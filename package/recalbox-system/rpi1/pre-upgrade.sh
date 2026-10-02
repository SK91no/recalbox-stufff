#!/bin/bash

mount -o remount,rw /boot

if [ ! -f /boot/recalbox-user-config.txt ]; then
  cat >/boot/recalbox-user-config.txt <<EOF
# Change to your needs

# uncomment if you get no picture on HDMI for a default "safe" mode
#hdmi_safe=1

disable_overscan=1

# uncomment to force a specific HDMI mode (this will force VGA)
#hdmi_group=1
#hdmi_mode=1

# Sound output. Set to 0 or comment for autodetect, 1 for DVI, 2 to force HDMI.
#hdmi_drive=2

config_hdmi_boost=0

# uncomment for composite PAL
#sdtv_mode=2

# uncomment for lirc-rpi
#dtoverlay=lirc-rpi
EOF
fi

# Remove old dats files from bios
rm -rf /recalbox/share/bios/fba_029671*.dat
rm -rf /recalbox/share/bios/fba/*.dat
rm -rf /recalbox/share/bios/FinalBurn*.dat
rm -rf /recalbox/share/bios/dc/*.dat.zip
rm -rf /recalbox/share/bios/dc/*barhi.zip
rm -rf /recalbox/share/bios/fbneo/FinalBurn*.dat
rm -rf /recalbox/share/bios/fbneo/light
rm -rf /recalbox/share/bios/mame/*.dat.zip
rm -rf /recalbox/share/bios/mame/*rhi.zip
rm -rf /recalbox/share/bios/mame2000/MAME\ 0.37b5\ XML.dat
rm -rf /recalbox/share/bios/mame2003/mame2003.xml
rm -rf /recalbox/share/bios/mame2003-plus/mame2003-plus.xml
rm -rf /recalbox/share/bios/mame2010/mame2010.xml
rm -rf /recalbox/share/bios/mame2015/mame2014-xml.zip
rm -rf /recalbox/share/bios/mame2015/mame2015-xml.zip

# Remove shaders files
rm -rf /recalbox/share/shaders/crt-pi.glslp