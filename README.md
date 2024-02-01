## Device mapper proxy

### Описание
Данный репозиторий содержит код модуля ядра под ОС linux, который создает виртуальные блочные устройства поверх существующего на базе device mapper и следит за статистикой выполняемых операция на устройстве.

Поддерживаемые статистические данные:
- Количество запросов на запись
- Количество запросов на чтение
- Средний размер блока на запись
- Средний размер блока на чтение
- Общее кол-во запросов
- Средний размер блока 

### Структура проекта
- Makefile - скрипт сборки для make
- dmp.c - исходный код модуля
- test.sh - скрипт автоматической установки и тестирования

### Сборка и установка

Модуль был протестирован на Ubuntu 22.04.3 LTS.
Для сборки небходим gcc и make.
Установка данных пакетов в Ubuntu:
```bash
sudo apt -y install build-essential
```
Далее небходимо склонировать данный репозиторий:
```bash
git clone https://github.com/Dlexeyn/Device_mapper_proxy_module.git &&
cd Device_mapper_proxy_module
```
Используйте make для сборки модуля:
```bash
make
```
С помощью insmod загрузите скомпилированный модуль ядра:
```bash
sudo insmod dmp.ko
```
Проверим, что модуль загружен:
```bash
lsmod | grep dmp
```
### Тестирование модуля
Создадим тестовое блочное устройство:
```bash
sudo dmsetup create zero1 --table "0 $size zero"
```
где _$size_ - размер устройства

Создадим наше dmp устройство:

```bash
sudo dmsetup create dmp1 --table "0 $size dmp /dev/mapper/zero1
```
где _$size_ - размер /dev/mapper/zero1

Проверим, что наши устройства создались:
```bash
ls -al /dev/mapper/*
```
```console
foo@bar:~$ ls -al /dev/mapper/*
lrwxrwxrwx 1 root root       7 Feb  1 10:35 /dev/mapper/dmp1 -> ../dm-2
lrwxrwxrwx 1 root root       7 Feb  1 10:35 /dev/mapper/zero1 -> ../dm-1
```

Проведем операции на запись и чтение с помощью dd:
```bash
sudo dd if=/dev/random of=/dev/mapper/dmp1 bs=4k count=1
sudo dd of=/dev/null if=/dev/mapper/dmp1 bs=4k count=1
```

Посмотрим статистику через sysfs:
```bash
cat /sys/module/dmp/stat/volumes
```
```console
read:
        reqs:90
        avg size:4274
write:
        reqs:2
        avg size:2048
total:
        reqs:92
        avg size:4182
```
### Удаление модуля
Прежде чем выгрузить модуль, необходимо удалить созданные устройства с помощью dmsetup:
```bash
sudo dmsetup remove dmp1
```
```bash
sudo dmsetup remove zero1
```
Чтобы выгрузить модуль, необходимо использовать rmmod:
```bash
sudo rmmod dmp
```