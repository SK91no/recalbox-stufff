#!/bin/bash

mousepoll=$(cat /sys/module/usbhid/parameters/mousepoll) 

if [[ $1 != $mousepoll ]];then
	/sbin/modprobe -r usbhid && /sbin/modprobe usbhid mousepoll=$1
fi
