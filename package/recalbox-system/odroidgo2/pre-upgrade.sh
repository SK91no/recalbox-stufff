#!/bin/bash

RA_CORE_OPTIONS=/recalbox/share/system/configs/retroarch/cores/retroarch-core-options.cfg

# remove old es_input.cfg as the joypad name has changed upstrem
rm /recalbox/share/system/.emulationstation/es_input.cfg

# change retroarchcustom.cfg.odroidgo2 to fix shaders and core freeze
sed -i -E 's/^(\s*video_driver\s*=).*/\1 "gl"/' /recalbox/share/system/configs/retroarch/retroarchcustom.cfg.odroidgo2

# enable Vulkan GPU render (#1563)
sed -i -E 's/^\s*(duckstation_GPU.Renderer\s*=\s*).*/\1"Vulkan"/' "$RA_CORE_OPTIONS"

# change lr-mupen64plus-nx core config
sed -i -E '/^(mupen64plus-43screensize|mupen64plus-EnableCopyColorToRDRAM|mupen64plus-ThreadedRenderer)/d' "$RA_CORE_OPTIONS"
cat >>"$RA_CORE_OPTIONS" <<EOF
mupen64plus-43screensize = "320x240"
mupen64plus-EnableCopyColorToRDRAM = "Off"
mupen64plus-ThreadedRenderer = "True"
EOF

# Bugfix for 7.1.1 GoS erroneous uboot
OLDBOOT=$(head -n 1 /boot/boot.ini)
if [ "$OLDBOOT" = 'odroidgoa-uboot-config' ]; then
  mount -o remount,rw /boot
  sed -i -E '1 s/^.*$/'"$OLDBOOT"'/' /boot/update/boot.ini
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

# Remove all .nvmem and .nvmem2 files from saves
rm -rf /recalbox/share/saves/atomiswave/reicast/*.nvmem
rm -rf /recalbox/share/saves/atomiswave/reicast/*.nvmem2

# Remove shaders files
rm -rf /recalbox/share/shaders/crt-pi.glslp