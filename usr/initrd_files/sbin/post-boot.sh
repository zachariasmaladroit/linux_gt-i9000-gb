#!/bin/sh

/voodoo/bin/remount_system_rw

# create busybox symlinks
DIR=`pwd`
if test -e /system/xbin/busybox; then
 cd /system/xbin
 for i in $(/system/xbin/busybox --list)
 do
  ln -s busybox $i
 done
 echo '/system/xbin/busybox ls -l $*' > /system/xbin/ll
 /system/xbin/busybox chmod +x /system/xbin/ll
 cd $DIR
fi

# delete "considered harmful" scripts
cd /system/etc/init.d
for i in `grep -l lowmemorykiller *` S90scheduler S99finish S98system_tweak 89system_tweak S98screenstate_scaling 90screenstate_scaling S97ramscript S97rambooster; do
 if test -e $i; then rm $i;fi;
done
cd $DIR

/voodoo/bin/remount_system_ro



# run the init.d scripts first, so that if anything in them conflicts with our
# settings, ours win ;)
/bin/sh /voodoo/scripts/run-parts.sh /system/etc/init.d

# clean up old cf-root tweak properties
/system/xbin/busybox rm /data/property/persist.tweak*


# disable iostats to reduce overhead
STL=`ls -d /sys/block/stl*`;
BML=`ls -d /sys/block/bml*`;
MMC=`ls -d /sys/block/mmc*`;
for k in $STL $BML $MMC;
do
 echo "0" > $k/queue/iostats
done


echo 0 > /proc/sys/vm/page-cluster
