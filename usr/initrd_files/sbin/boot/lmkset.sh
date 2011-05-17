# LMKset - kodos
#
# This script sets android's lowmemorykill thresholds and adj values, updating both
# the /sys knobs used by the kernel, and the android system properties (i.e. 
# getprop/setprop) used by the android framework, keeping them in synch with each other.
#
# Presets are defined/selected in /etc/lmk.conf

source /sbin/boot/lmk.conf.default
LMK_CONF_VERSION_CURRENT=$LMK_CONF_VERSION
LMK_CONF_VERSION=0

mount -o remount,rw /system
mount -o remount,rw /

if [ ! -e /system/etc/lmk.conf ]; then
 cp /sbin/boot/lmk.conf.default /system/etc/lmk.conf
fi;

source /system/etc/lmk.conf

if [ ! "$LMK_CONF_VERSION" == "$LMK_CONF_VERSION_CURRENT" ]; then
 rm /data/property/persist.lmkset.preset
 cp /sbin/boot/lmk.conf.default /system/etc/lmk.conf
 source /system/etc/lmk.conf
fi;

mount -o remount,ro /system
mount -o remount,ro /


eval export set LMKSET_ADJ_1=\${"PRESET_${PRESET}_ADJ_1"}
eval export set LMKSET_ADJ_2=\${"PRESET_${PRESET}_ADJ_2"}
eval export set LMKSET_ADJ_3=\${"PRESET_${PRESET}_ADJ_3"}
eval export set LMKSET_ADJ_4=\${"PRESET_${PRESET}_ADJ_4"}
eval export set LMKSET_ADJ_5=\${"PRESET_${PRESET}_ADJ_5"}
eval export set LMKSET_ADJ_6=\${"PRESET_${PRESET}_ADJ_6"}
eval export set LMKSET_ADJ_7=\${"PRESET_${PRESET}_ADJ_7"}
eval export set LMKSET_ADJ_8=\${"PRESET_${PRESET}_ADJ_8"}
eval export set LMKSET_ADJ_9=\${"PRESET_${PRESET}_ADJ_9"}

eval export set LMKSET_MEM_1=\${"PRESET_${PRESET}_MEM_1"}
eval export set LMKSET_MEM_2=\${"PRESET_${PRESET}_MEM_2"}
eval export set LMKSET_MEM_3=\${"PRESET_${PRESET}_MEM_3"}
eval export set LMKSET_MEM_4=\${"PRESET_${PRESET}_MEM_4"}
eval export set LMKSET_MEM_5=\${"PRESET_${PRESET}_MEM_5"}
eval export set LMKSET_MEM_6=\${"PRESET_${PRESET}_MEM_6"}
eval export set LMKSET_MEM_7=\${"PRESET_${PRESET}_MEM_7"}
eval export set LMKSET_MEM_8=\${"PRESET_${PRESET}_MEM_8"}
eval export set LMKSET_MEM_9=\${"PRESET_${PRESET}_MEM_9"}

eval export set LMKSET_ADJ=\${"PRESET_${PRESET}_ADJ"}
eval export set LMKSET_MINFREE=\${"PRESET_${PRESET}_MINFREE"}

setprop ro.FOREGROUND_APP_ADJ $LMKSET_ADJ_1
setprop ro.VISIBLE_APP_ADJ $LMKSET_ADJ_2
setprop ro.PERCEPTIBLE_APP_ADJ $LMKSET_ADJ_3
setprop ro.HEAVY_WEIGHT_APP_ADJ $LMKSET_ADJ_4
setprop ro.SECONDARY_SERVER_ADJ $LMKSET_ADJ_5
setprop ro.BACKUP_APP_ADJ $LMKSET_ADJ_6
setprop ro.HOME_APP_ADJ $LMKSET_ADJ_7
setprop ro.HIDDEN_APP_MIN_ADJ $LMKSET_ADJ_8
setprop ro.EMPTY_APP_ADJ $LMKSET_ADJ_9

setprop ro.FOREGROUND_APP_MEM $LMKSET_MEM_1
setprop ro.VISIBLE_APP_MEM $LMKSET_MEM_2
setprop ro.PERCEPTIBLE_APP_MEM $LMKSET_MEM_3
setprop ro.HEAVY_WEIGHT_APP_MEM $LMKSET_MEM_4
setprop ro.SECONDARY_SERVER_MEM $LMKSET_MEM_5
setprop ro.BACKUP_APP_MEM $LMKSET_MEM_6
setprop ro.HOME_APP_MEM $LMKSET_MEM_7
setprop ro.HIDDEN_APP_MEM $LMKSET_MEM_8
setprop ro.EMPTY_APP_MEM $LMKSET_MEM_9

echo $LMKSET_ADJ > /sys/module/lowmemorykiller/parameters/adj
echo "1" > /proc/sys/vm/overcommit_memory 1
echo "4" /proc/sys/vm/min_free_order_shift   
echo $LMKSET_MINFREE > /sys/module/lowmemorykiller/parameters/minfree
