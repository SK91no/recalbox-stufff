#!/bin/bash

function switchToVec {
  if ! videoOnVec; then
    dtoverlay -r recalboxrgbdual2-video
    echo 1 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-rgblanking/value
    echo 1 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-composite/value
    compositestd="0"
    if grep -q "options.dual2.compositestandard = ." /boot/crt/recalbox-crt-options.cfg; then
      compositestd=$(grep "options.dual2.compositestandard = ." /boot/crt/recalbox-crt-options.cfg | sed 's/\(.*\)\(.\)/\2/')
    fi
    dtoverlay recalboxrgbdual2-video vec tv_mode="${compositestd}"
    mpv --vo=drm --drm-mode=help
  fi
}

function switchToDpi {
  if ! videoOnDpi; then
    dtoverlay -r recalboxrgbdual2-video
    echo 0 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-rgblanking/value
    echo 0 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-composite/value
    dtoverlay recalboxrgbdual2-video dpi
    mpv --vo=drm --drm-mode=help
  fi
}

function switchToVideoDefault {
  if grep -q "1" /sys/devices/platform/recalboxrgbdual2/dipswitch-composite/value; then
    switchToDpi
  else
    switchToVec
  fi
}

function switchToRatio43 {
  echo 0 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-widescreen/value
}

function switchToRatio169 {
  echo 1 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-widescreen/value
}
function switchToRatioDefault {
  if grep -q "options.video.widescreen = 1" "/boot/crt/recalbox-crt-options.cfg";then
    echo 1 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-widescreen/value
  else
    echo 0 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-widescreen/value
  fi
}

function maybeMount {
  if [ ! -d /config/device-tree ];then
    mount -o remount,rw / && mkdir -p /config && mount -t configfs none /config && mount -o remount,ro /
  fi
}

function findConnectedConnectors() {
  grep -l ^connected /sys/class/drm/card*/status | sed 's/.*card\([0-9]\+\)-\([^\/]\+\).*/\2/'
}

function videoOnVec {
  local connector="$(findConnectedConnectors | grep Composite)"
  if [[ -z "${connector}" ]]; then return 1; fi
}

function videoOnDpi {
  local connector="$(findConnectedConnectors | grep VGA)"
  if [[ -z "${connector}" ]]; then return 1; fi
}

function disableScart {
  echo 1 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-scart-disable/value
}

function enableScart {
  echo 0 >  /sys/devices/platform/recalboxrgbdual2/rgbd2-scart-disable/value
}

function initScartVideo {
  switchToRatio43
  disableScart
  sleep 0.5
  enableScart
  switchToVideoDefault
  switchToRatioDefault
}

maybeMount

if [[ "${1}" == "video" ]]; then
  if [[ "${2}" == "vec" ]]; then
    switchToVec
  elif [[ "${2}" == "dpi" ]]; then
    switchToDpi
  else
    switchToVideoDefault
  fi
elif [[ "${1}" == "ratio" ]]; then
   if [[ "${2}" == "43" ]]; then
      switchToRatio43
    elif [[ "${2}" == "169" ]]; then
      switchToRatio169
    else
      switchToRatioDefault
    fi
elif [[ "${1}" == "init" ]]; then
  initScartVideo
fi