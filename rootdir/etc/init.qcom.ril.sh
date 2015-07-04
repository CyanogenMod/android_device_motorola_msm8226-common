#!/system/bin/sh
export PATH=/system/xbin:$PATH

multisim=`getprop persist.radio.multisim.config`

if [ "$multisim" = "dsds" ] || [ "$multisim" = "dsda" ]; then
    stop ril-daemon1
    start ril-daemon1
fi
