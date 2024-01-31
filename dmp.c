#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>
#include <linux/average.h>

DECLARE_EWMA(size, 6, 4);

struct dmp_device {
    struct dm_dev *dev;
};

struct stats {
    unsigned long read_reqs;
    unsigned long write_reqs;
    unsigned long total_reqs;
    struct ewma_size read_avg_size;
    struct ewma_size write_avg_size;
    struct ewma_size total_avg_size;
};

static struct kobject *statistic;

static struct stats dmp_stats = {0, 0, 0, }; 

static ssize_t volumes_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "read:\n\treqs:%lu\n\tavg size:%lu\nwrite:\n\treqs:%lu\n\tavg size:%lu\ntotal:\n\treqs:%lu\n\tavg size:%lu\n",
            dmp_stats.read_reqs, (unsigned long) ewma_size_read(&dmp_stats.read_avg_size),
            dmp_stats.write_reqs, (unsigned long) ewma_size_read(&dmp_stats.write_avg_size),
            dmp_stats.total_reqs, (unsigned long) ewma_size_read(&dmp_stats.total_avg_size));
}

static int error_ctr(void){
    pr_info("out function dmp_ctr with ERROR");
    return -EINVAL;
}

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv){
    struct dmp_device *dd;

    pr_info("in function dmp_ctr\n");

    if(argc != 1){
        pr_info("in function dmp_ctr\n");
        ti->error = "Invalid argument count";
        return -EINVAL;
    }

    dd = kmalloc(sizeof(struct dmp_device), GFP_KERNEL);

    if(dd == NULL){
        pr_info("dd is null\n");
        ti->error = "dm_device: Cannot allocate linear context";
        return -ENOMEM;
    }

    if (dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dd->dev)){
        ti->error = "dm_device: Device lookup failed";
        kfree(dd);
        return error_ctr();
    }
    
    ti->private = dd;

    pr_info("out function dmp_ctr\n");
    return 0;
}

static void dmp_dtr(struct dm_target *ti){
    struct dmp_device *dd = (struct dmp_device *) ti->private;
    pr_info("in function dmp_ctr");
    dm_put_device(ti, dd->dev);
    kfree(dd);
    pr_info("out function dmp_ctr");
}


static int dmp_map(struct dm_target *ti, struct bio *bio){

    struct dmp_device *dd = (struct dmp_device *) ti->private;

    dmp_stats.total_reqs += 1;
    ewma_size_add(&dmp_stats.total_avg_size, 1);

    switch (bio_op(bio)){
        case REQ_OP_READ:
            dmp_stats.read_reqs += 1;
            ewma_size_add(&dmp_stats.read_avg_size, bio->bi_iter.bi_size);
            break;
        case REQ_OP_WRITE:
            dmp_stats.write_reqs += 1;
            ewma_size_add(&dmp_stats.write_avg_size, bio->bi_iter.bi_size);
            break;
        default:
            return DM_MAPIO_KILL;
    }

    bio_set_dev(bio, dd->dev->bdev);
    submit_bio(bio);

    return DM_MAPIO_SUBMITTED;
}

static struct target_type dmp_target = {
    .name = "dmp",
    .version = {0,0,1},
    .module = THIS_MODULE,
    .ctr = dmp_ctr,
    .dtr = dmp_dtr,
    .map = dmp_map,
};

static struct kobj_attribute volume_attr = 
        __ATTR(volumes, 0644, volumes_show, NULL);

static int init_dmp_target(void) {
    int res = dm_register_target(&dmp_target);
    if(res < 0) {
        pr_info("Error in registering targert\n");
        return res;
    }

    statistic = kobject_create_and_add("stat", &THIS_MODULE->mkobj.kobj);
    if (!statistic) {
        pr_info("Error in create kobject\n");
        dm_unregister_target(&dmp_target);
        return -ENOMEM;
    }

    res = sysfs_create_file(statistic, &volume_attr.attr);
    if(res < 0) {
        pr_info("Error to create sysfs file\n");
        dm_unregister_target(&dmp_target);
        kobject_put(statistic);
        return res;
    }

    ewma_size_init(&dmp_stats.read_avg_size);
    ewma_size_init(&dmp_stats.write_avg_size);
    ewma_size_init(&dmp_stats.total_avg_size);
    
    return 0;
}

static void cleanup_dmp_target(void){
    dm_unregister_target(&dmp_target);
    kobject_put(statistic);
}

module_init(init_dmp_target);
module_exit(cleanup_dmp_target);
MODULE_LICENSE("GPL");

