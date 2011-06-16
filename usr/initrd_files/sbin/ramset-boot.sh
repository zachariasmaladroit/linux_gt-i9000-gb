#!/bin/busybox sh

/bin/busybox sh /sbin/ramset.sh

read sync < /data/sync_fifo_ramset
/bin/rm /data/sync_fifo_ramset
