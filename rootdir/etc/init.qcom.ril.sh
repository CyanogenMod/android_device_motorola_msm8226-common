#!/system/bin/sh
export PATH=/system/xbin:$PATH

multisim=`toybox getprop persist.radio.multisim.config`

if [ "$multisim" = "dsds" ] || [ "$multisim" = "dsda" ]; then
    start ril-daemon1
fi
