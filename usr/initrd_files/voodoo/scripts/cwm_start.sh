#!/bin/sh
# mostly replicate updater-script behavior from CWM as update.zip
set -x
export PATH=$PATH:/bin

exec > /voodoo/logs/cwm_start_log.txt 2>&1

# froyo make /sdcard a symlink to /mnt/sdcard, which confuses CWM
rm /sdcard
mkdir /sdcard

ln -sf busybox /sbin/[
ln -sf busybox /sbin/[[
ln -sf recovery /sbin/amend
ln -sf busybox /sbin/ash
ln -sf busybox /sbin/awk
ln -sf busybox /sbin/basename
ln -sf busybox /sbin/bbconfig
ln -sf busybox /sbin/bunzip2
#ln -sf recovery /sbin/busybox
ln -sf busybox /sbin/bzcat
ln -sf busybox /sbin/bzip2
ln -sf busybox /sbin/cal
ln -sf busybox /sbin/cat
ln -sf busybox /sbin/catv
ln -sf busybox /sbin/chgrp
ln -sf busybox /sbin/chmod
ln -sf busybox /sbin/chown
ln -sf busybox /sbin/chroot
ln -sf busybox /sbin/cksum
ln -sf busybox /sbin/clear
ln -sf busybox /sbin/cmp
ln -sf busybox /sbin/cp
ln -sf busybox /sbin/cpio
ln -sf busybox /sbin/cut
ln -sf busybox /sbin/date
ln -sf busybox /sbin/dc
ln -sf busybox /sbin/dd
ln -sf busybox /sbin/depmod
ln -sf busybox /sbin/devmem
ln -sf busybox /sbin/df
ln -sf busybox /sbin/diff
ln -sf busybox /sbin/dirname
ln -sf busybox /sbin/dmesg
ln -sf busybox /sbin/dos2unix
ln -sf busybox /sbin/du
ln -sf recovery /sbin/dump_image
ln -sf busybox /sbin/echo
ln -sf busybox /sbin/egrep
ln -sf busybox /sbin/env
ln -sf recovery /sbin/erase_image
ln -sf busybox /sbin/expr
ln -sf busybox /sbin/false
ln -sf busybox /sbin/fdisk
ln -sf busybox /sbin/fgrep
ln -sf busybox /sbin/find
ln -sf recovery /sbin/flash_image
ln -sf busybox /sbin/fold
ln -sf busybox /sbin/free
ln -sf busybox /sbin/freeramdisk
ln -sf busybox /sbin/fuser
ln -sf busybox /sbin/getopt
ln -sf busybox /sbin/grep
ln -sf busybox /sbin/gunzip
ln -sf busybox /sbin/gzip
ln -sf busybox /sbin/head
ln -sf busybox /sbin/hexdump
ln -sf busybox /sbin/id
ln -sf busybox /sbin/insmod
ln -sf busybox /sbin/install
ln -sf busybox /sbin/kill
ln -sf busybox /sbin/killall
ln -sf busybox /sbin/killall5
ln -sf busybox /sbin/length
ln -sf busybox /sbin/less
ln -sf busybox /sbin/ln
ln -sf busybox /sbin/losetup
ln -sf busybox /sbin/ls
ln -sf busybox /sbin/lsmod
ln -sf busybox /sbin/lspci
ln -sf busybox /sbin/lsusb
ln -sf busybox /sbin/lzop
ln -sf busybox /sbin/lzopcat
ln -sf busybox /sbin/md5sum
ln -sf busybox /sbin/mkdir
ln -sf busybox /sbin/mke2fs
ln -sf busybox /sbin/mkfifo
ln -sf busybox /sbin/mkfs.ext2
ln -sf busybox /sbin/mknod
ln -sf busybox /sbin/mkswap
ln -sf busybox /sbin/mktemp
ln -sf recovery /sbin/mkyaffs2image
ln -sf busybox /sbin/modprobe
ln -sf busybox /sbin/more
#ln -sf busybox /sbin/mount
ln -sf busybox /sbin/mountpoint
ln -sf busybox /sbin/mv
ln -sf recovery /sbin/nandroid
ln -sf busybox /sbin/nice
ln -sf busybox /sbin/nohup
ln -sf busybox /sbin/od
ln -sf busybox /sbin/patch
ln -sf busybox /sbin/pgrep
ln -sf busybox /sbin/pidof
ln -sf busybox /sbin/pkill
ln -sf busybox /sbin/printenv
ln -sf busybox /sbin/printf
ln -sf busybox /sbin/ps
ln -sf busybox /sbin/pwd
ln -sf busybox /sbin/rdev
ln -sf busybox /sbin/readlink
ln -sf busybox /sbin/realpath
ln -sf recovery /sbin/reboot
ln -sf busybox /sbin/renice
ln -sf busybox /sbin/reset
ln -sf busybox /sbin/rm
ln -sf busybox /sbin/rmdir
ln -sf busybox /sbin/rmmod
ln -sf busybox /sbin/run-parts
ln -sf busybox /sbin/sed
ln -sf busybox /sbin/seq
ln -sf busybox /sbin/setsid
ln -sf busybox /sbin/sh
ln -sf busybox /sbin/sha1sum
ln -sf busybox /sbin/sha256sum
ln -sf busybox /sbin/sha512sum
ln -sf busybox /sbin/sleep
ln -sf busybox /sbin/sort
ln -sf busybox /sbin/split
ln -sf busybox /sbin/stat
ln -sf busybox /sbin/strings
ln -sf busybox /sbin/stty
ln -sf busybox /sbin/swapoff
ln -sf busybox /sbin/swapon
ln -sf busybox /sbin/sync
ln -sf busybox /sbin/sysctl
ln -sf busybox /sbin/tac
ln -sf busybox /sbin/tail
ln -sf busybox /sbin/tar
ln -sf busybox /sbin/tee
ln -sf busybox /sbin/test
ln -sf busybox /sbin/time
ln -sf busybox /sbin/top
ln -sf busybox /sbin/touch
ln -sf busybox /sbin/tr
ln -sf busybox /sbin/true
ln -sf busybox /sbin/tty
ln -sf busybox /sbin/umount
ln -sf busybox /sbin/uname
ln -sf busybox /sbin/uniq
ln -sf busybox /sbin/unix2dos
ln -sf busybox /sbin/unlzop
ln -sf recovery /sbin/unyaffs
ln -sf busybox /sbin/unzip
ln -sf busybox /sbin/uptime
ln -sf busybox /sbin/usleep
ln -sf busybox /sbin/uudecode
ln -sf busybox /sbin/uuencode
ln -sf busybox /sbin/watch
ln -sf busybox /sbin/wc
ln -sf busybox /sbin/which
ln -sf busybox /sbin/whoami
ln -sf busybox /sbin/xargs
ln -sf busybox /sbin/yes
ln -sf busybox /sbin/zcat


# also shorter
echo '#!/sbin/sh
set -x
exec >> /voodoo/logs/cwm_postrecoveryboot_log.txt 2>&1
rm /etc
mkdir -p /etc
mkdir -p /datadata
chmod 4777 /sbin/su
umount /efs
umount /dbdata
umount /data

# succeed to mount the sdcard by default even with broken fstab
mount -t vfat -o rw,nosuid,nodev,noexec,uid=1000,gid=1015,fmask=0002,dmask=0002,allow_utime=0020,iocharset=iso8859-1,shortname=mixed,utf8,errors=remount-ro "`cat /voodoo/run/sdcard_device`" /sdcard
' > /sbin/postrecoveryboot.sh


# run the actual recovery
exec /sbin/recovery
