#!/bin/bash

export DISPLAY=:0

CABINET=$(recalbox_settings -command load -key pinball.cabinet)
SCREEN1=$(recalbox_settings -command load -key pinball.screen1)
SCREEN2=$(recalbox_settings -command load -key pinball.screen2)
SCREEN2_RES=$(recalbox_settings -command load -key pinball.screen2.resolution)

if [[ -z $SCREEN2_RES ]]; then
	SCREEN2_RES_CMD="--auto"
else
	SCREEN2_RES_CMD="--mode $SCREEN2_RES"
fi


if [[ "$CABINET" -eq 1 ]] && [[ ! -z $SCREEN1 ]] && [[ ! -z $SCREEN2 ]]; then
	
	echo "Pinball : Dualscreen configuration"
	
	if [[ $(xrandr | grep $SCREEN1 | awk  '{print $2}') == disconnected ]];then
		echo "Pinball : No screen 1 disconnected"
		xrandr --output $SCREEN1 --off
		exit 0
	fi
	
	RES=$(xrandr | grep $SCREEN1 | sed 's/.* \([0-9]*x[0-9]*\)+.*/\1/')
	
	if [[ -z "$SCREEN1" ]]; then
		echo "Pinball : No screen 1 configured"
		exit 0
	fi
	if [[ -z "$SCREEN2" ]]; then
		echo "Pinball : No screen 2 configured"
		exit 0
	fi
	echo "Pinball : turn on second screen"
	xrandr --output $SCREEN2 $SCREEN2_RES_CMD
	xrandr --output $SCREEN1 --primary -r $RES --output $SCREEN2 --right-of $SCREEN1
fi
