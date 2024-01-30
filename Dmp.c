#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/device-mapper.h>

#define DM_NAME = "dmp"

struct dmp_device
{
    struct dm_dev *dev;
    sector_t start;
};

static struct dm_device *dd;

static int error_ctr(void){
    pr_info("out function dmp_ctr with ERROR");
    return -EINVAL;
}

static int dmp_ctr(struct dm_target *ti, unsigned int argc, char **argv){
    unsigned long long start;

    pr_info("in function dmp_ctr\n");

    if(argc != 2){
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

    if (sscanf(argv[1], "%llu", &start) != 1){
        ti->error = "dm_device: Ivalid device sector";
        kfree(dd);
        return error_ctr();
    }

    dd->start = (sector_t) start;

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
    pr_info("in function dmp_ctr");
    dm_put_device(ti, dd->dev);
    kfree(dd);
    pr_info("out function dmp_ctr");
}


static int dmp_map(struct dm_target *ti, struct bio *bio){
    switch (bio_op(bio)){
        case REQ_OP_READ:
            break;
        case REQ_OP_WRITE:
            break;
        default:
            return DM_MAPIO_KILL;
    }

    bio_endio(bio);

    return DM_MAPIO_SUBMITTED;
}

static struct target_type dmp_target = {
    .name = "device_mapper_proxy",
    .version = {0,0,1},
    .module = THIS_MODULE,
    .ctr = dmp_ctr,
    .dtr = dmp_dtr,
    .map = dmp_map,
};

static int init_dmp_target(void){
    int res = dm_register_target(&dmp_target);
    if(res < 0){
        pr_info("Error in registering targert\n");
    }
    return 0;
}

static void cleanup_dmp_target(void){
    dm_unregister_target(&dmp_target);
}

module_init(init_dmp_target);
module_exit(cleanup_dmp_target);
MODULE_LICENSE("GPL");

