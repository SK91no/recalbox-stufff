#!/bin/bash

do_wget() {
  wget --quiet -r "$1" -O "$2"
  if [ "$?" -ne 0 ]; then
    echo "Something went wrong! Can't download $1 to $2"
    exit 1
  fi
}

# use this script to update argon40 scripts
[ -d scripts ] && rm -rf scripts
[ -d fonts ] && rm -rf fonts
mkdir scripts
pushd scripts
do_wget  http://download.argon40.com/scripts/argon-rpi-eeprom-config-psu.py    argonone-eepromconfig.py

# one
do_wget  http://download.argon40.com/scripts/argononed.py                      argononed.py

# Following downloads are set as pending because
# some script checks the presence of those files to
# determine on which board the system is running
# Pending scripts should be renamed by the case installer
# eon
do_wget  http://download.argon40.com/scripts/argoneond.py                      argoneond.py.pending
do_wget  http://download.argon40.com/scripts/argonrtc.py                       argonrtc.py.pending

# eon & eoled
do_wget  http://download.argon40.com/scripts/argoneonoled.py                   argoneonoled.py.pending
do_wget  http://download.argon40.com/scripts/argononeoled.py                   argononeoled.py.pending

do_wget  http://download.argon40.com/scripts/argonstatus.py                    argonstatus.py
do_wget  http://download.argon40.com/scripts/argonsysinfo.py                   argonsysinfo.py
do_wget  http://download.argon40.com/scripts/argonregister-v1.py               argonregister-v1.py
do_wget  http://download.argon40.com/scripts/argonregister.py                  argonregister.py
do_wget  http://download.argon40.com/scripts/argonpowerbutton-rpigpio.py       argonpowerbutton.py

# oled fonts
popd
mkdir fonts
pushd fonts
for binfile in font8x6 font16x12 font32x24 font64x48 font16x8 font24x16 font48x32 bgdefault bgram bgip bgtemp bgcpu bgraid bgstorage bgtime; do
  do_wget http://download.argon40.com/oled/${binfile}.bin ${binfile}.bin
done

echo "All done!"
