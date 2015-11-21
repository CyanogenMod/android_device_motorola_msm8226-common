#!/system/bin/sh

DUMP=/cache/pds-dump.img

if [ ! -f $DUMP ]; then
    # Make a copy of pds in /cache
    /system/bin/dd if=/dev/block/platform/msm_sdcc.1/by-name/pds of=$DUMP-temp
    # Prevent init from using a partially created file
    mv $DUMP-temp $DUMP
fi
