/sbin/insmod /lib/modules/zram.ko num_devices=2
/sbin/zramconfig /dev/block/zram0 --disksize_kb 131072
#/sbin/zramconfig /dev/block/zram0 --disksize_kb 262144
/sbin/zramconfig /dev/block/zram0 --init
/sbin/mkswap /dev/block/zram0
/sbin/swapon /dev/block/zram0
setprop persist.tweak.vm.swappiness disabled
echo "60" > /proc/sys/vm/swappiness
