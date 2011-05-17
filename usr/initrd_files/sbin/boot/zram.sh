#zram/zcache settings - kodos


source /sbin/boot/zram.conf.default
ZRAM_CONF_VERSION_CURRENT=$ZRAM_CONF_VERSION
ZRAM_CONF_VERSION=0

mount -o remount,rw /system
mount -o remount,rw /

if [ ! -e /system/etc/zram.conf ]; then
 cp /sbin/boot/zram.conf.default /system/etc/zram.conf
fi;

source /system/etc/zram.conf

if [ ! "$ZRAM_CONF_VERSION" == "$ZRAM_CONF_VERSION_CURRENT" ]; then
 cp /sbin/boot/zram.conf.default /system/etc/zram.conf
 source /system/etc/zram.conf
fi;

mount -o remount,ro /system
mount -o remount,ro /

if [ $ZRAM_ENABLED == "1" ]; then
 /sbin/insmod /lib/modules/zram.ko num_devices=$ZRAM_NUM_DEVICES
 /sbin/zramconfig /dev/block/zram0 --disksize_kb $ZRAM_DISKSIZE_KB
 /sbin/zramconfig /dev/block/zram0 --init
 /sbin/mkswap /dev/block/zram0
 /sbin/swapon /dev/block/zram0
 setprop persist.tweak.vm.swappiness disabled
 echo $ZRAM_SWAPPINESS > /proc/sys/vm/swappiness
fi;

for i in `ls /sys/kernel/mm/zcache/`; do
 echo $ZCACHE_MEMLIMIT > /sys/kernel/mm/zcache/$i/memlimit
done;
