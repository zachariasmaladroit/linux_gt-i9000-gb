#!/sbin/ext/busybox sh

/sbin/ext/busybox sh /sbin/boot/lmkset.sh

read sync < /data/sync_fifo_lmkset
rm /data/sync_fifo_lmkset
