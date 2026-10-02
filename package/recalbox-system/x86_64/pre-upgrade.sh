#!/bin/bash

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

## Move cfg config for cdi-2015
mkdir -p /recalbox/share/system/configs/cdi/cdi2015
mv /recalbox/share/saves/cdi/cdi2015/cfg /recalbox/share/system/configs/cdi2015/