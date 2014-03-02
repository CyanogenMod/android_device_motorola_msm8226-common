#!/sbin/bbx sh
#
# Check if this is a GPE device and copy in the right fstab
#

FSTYPE=`/sbin/bbx blkid /dev/block/mmcblk0p36 | /sbin/bbx cut -d ' ' -f3 | /sbin/bbx cut -d '"' -f2`

if [ "$FSTYPE" == "ext4" ]
then
  /sbin/bbx cp -f /gpe-fstab.qcom /etc/recovery.fstab
  /sbin/bbx mv -f /gpe-fstab.qcom /fstab.qcom
else
  /sbin/bbx rm /gpe-fstab.qcom
fi
