#!/sbin/sh

FILES="keymaster.b00 keymaster.b01 keymaster.b02 keymaster.b03 keymaster.mdt"

ret=0
mkdir -p /firmware
mount -t ext4 /dev/block/platform/msm_sdcc.1/by-name/modem /firmware -o ro
for i in $FILES; do
    if [ ! -f /firmware/image/$i ]; then
        ret=1
        break;
    fi
done
umount /firmware

exit $ret
