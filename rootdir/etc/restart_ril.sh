#!/system/bin/sh
export PATH=/system/xbin:$PATH

multisim=`getprop persist.radio.multisim.config`

if [ "$multisim" = "dsds" ] || [ "$multisim" = "dsda" ]; then
    restart ril-daemon
    restart ril-daemon1
elif [ "$multisim" = "tsts" ]; then
    restart ril-daemon
    restart ril-daemon1
    restart ril-daemon2
else
    restart ril-daemon
fi

