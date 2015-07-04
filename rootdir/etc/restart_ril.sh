#!/system/bin/sh
export PATH=/system/xbin:$PATH

multisim=`getprop persist.radio.multisim.config`

if [ "$multisim" = "dsds" ] || [ "$multisim" = "dsda" ]; then
    stop ril-daemon
    stop ril-daemon1
    start ril-daemon
    start ril-daemon1
elif [ "$multisim" = "tsts" ]; then
    stop ril-daemon
    stop ril-daemon1
    stop ril-daemon2
    start ril-daemon
    start ril-daemon1
    start ril-daemon2
else
    stop ril-daemon
    start ril-daemon
fi

