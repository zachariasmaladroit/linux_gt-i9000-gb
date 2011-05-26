#Not all volumes are finished mounting by the time zram.sh runs,
#so they don't end up getting their memlimit set. This waits 60
#seconds for all mounts to finish, then sets their memlimit.

sleep 60

for i in `ls /sys/kernel/mm/zcache/`; do
 echo $RAMSET_ZCACHE_MEMLIMIT > /sys/kernel/mm/zcache/$i/memlimit
done;

