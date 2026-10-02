#!/bin/bash
INIT_SCRIPT=$(basename "$0")

if [ ! "$1" ];then
    echo -e "usage : recalbox-config.sh [command] [args]\nWith command in\n\toverscan [enable|disable]\n\toverclock [none|high|turbo|extrem]\n\taudio [hdmi|jack|auto|string]\n\tlsaudio\n\tcanupdate\n\tupdate\n\twifi [enable|disable] ssid key\n\tstorage [current|list|INTERNAL|ANYEXTERNAL|RAM|DEV UUID]\n\tsetRootPassword [password]\n\tgetRootPassword"
    exit 1
fi
configFile="/boot/recalbox-user-config.txt"
storageFile="/boot/recalbox-boot.conf"
command="$1"
mode="$2"
extra1="$3"
extra2="$4"
extra3="$5"
extra4="$6"
postMode="${@:3}"
arch=`cat /recalbox/recalbox.arch`

preBootConfig() {
    mount -o remount,rw /boot
}

postBootConfig() {
    mount -o remount,ro /boot
}

function getSubNum () {
    # $1 : version number. ex 4.0.2
    # $2 : separator. ex : "."
    # $3 : the position you need. ex : 2
    result=`echo $1 | cut -d "$2" -f "$3" 2>/dev/null`
    [[ -z $result ]] && echo "0" || echo $result
}

function isNewer () {
    # compare $1 and $2
    # if $1 >= $2 return 0
    # else return 1
    newVersion="$1"
    oldVersion="$2"

    for i in 1 2 3 ; do
        oldNum=`getSubNum $oldVersion "." "$i"`
        newNum=`getSubNum $newVersion "." "$i"`
        [[ $newNum -eq $oldNum ]] && continue
        [[ $newNum -gt $oldNum ]] && return 0
        [[ $newNum -lt $oldNum ]] && return 1
    done
    return 1
}


log=/recalbox/share/system/logs/recalbox.log
systemsetting="recalbox_settings"

recallog -s "${INIT_SCRIPT}" -t "CONFIG"  "---- recalbox-config.sh ----"

if [ "$command" == "gpiocontrollers" ];then
    command="module"
    mode="load"
    extra1="mk_arcade_joystick_rpi"
    extra2="map=1,2"
fi

if [ "$command" == "module" ];then
    modulename="$extra1"
    map="$extra2 $extra3 $extra4"
    # remove in all cases
    rmmod /lib/modules/`uname -r`/updates/${modulename}.ko 2>&1 | recallog -s "${INIT_SCRIPT}" -t "MODULES"

    if [ "$mode" == "load" ];then
        echo "`logtime` : loading module $modulename args = $map" 2>&1 | recallog -s "${INIT_SCRIPT}" -t "MODULES"
        insmod /lib/modules/`uname -r`/updates/${modulename}.ko $map 2>&1 | recallog -s "${INIT_SCRIPT}" -t "MODULES"
    [ "$?" ] || exit 1
    fi
    exit 0
fi

if [ "$command" == "wifi" ]; then
    ssid="$3"
    psk="$4"

    if [[ "$mode" == "enable" ]]; then
        recallog -s "${INIT_SCRIPT}" -t "WIFI" "(re)configure wifi"
        if [[ $ssid != "" && $psk != "" ]];then
          ssid=$(echo "$ssid" | sed -e 's/\\/\\\\/g; s/\//\\\//g; s/&/\\\&/g')
          psk=$(echo "$psk" | sed -e 's/\\/\\\\/g; s/\//\\\//g; s/&/\\\&/g')
          sed -i "s|^wifi.ssid=.*|wifi.ssid=${ssid}|g" /recalbox/share/system/recalbox.conf
          sed -i "s|^wifi.key=.*|wifi.key=${psk}|g" /recalbox/share/system/recalbox.conf
        fi
        $systemsetting -command save -key wifi.enabled -value 1
        sleep 1
        /etc/init.d/S09wifi restart
        sleep 4 # wait a bit before returning to ES
        exit $?
    fi
    if [[ "$mode" == "disable" ]]; then
        recallog -s "${INIT_SCRIPT}" -t "WIFI" "disable wifi"
        /etc/init.d/S09wifi stop
        exit $?
    fi
    if [[ "$mode" == "list" ]]; then
        wpa_cli -i wlan0 scan > /dev/null || exit 1
        sleep 5 # wait a bit until some results come in
        wpa_cli -i wlan0 scan_results | tail -n +2 | sed -r 's/^([^ \t]*[ \t]*){4}(.*)$/\2/' | awk '!a[$0]++' || exit 1
        exit 0
    fi
    if [[ "$mode" == "wps" ]]; then
        wpa_cli wps_pbc || exit 1
        exit 0
    fi
    if [[ "$mode" == "waitip" ]]; then
        try=0
        until [ ! $try -lt 60 ]
        do
            # Wait for an IP
            if [ `ifconfig wlan0 | grep -c "inet addr"` -gt 0 ]; then
                break;
            fi
            sleep 1
            try=`expr $try + 1`
        done
        exit 0
    fi
    if [[ "$mode" == "save" ]]; then
        wpa_cli set update_config 1 > /dev/null || exit 1
        mount -o remount,rw /
        wpa_cli save_config || exit 1 # Let OK/FAIL be captured by the caller
        mount -o remount,ro /
        exit 0
    fi
fi

if [[ "$command" == "hcitoolscan" ]]; then
    scanningDaemon=test-discovery
    alternate="`$systemsetting  -command load -key controllers.bluetooth.alternate`"
    [[ $alternate == 1 ]] && scanningDaemon=btDaemon
    /recalbox/scripts/bluetooth/"$scanningDaemon" & ( PID=$! ; sleep 15 ; kill -15 $PID)
    PYTHONIOENCODING=UTF-8 /recalbox/scripts/bluetooth/eslist.sh
    exit 0
fi

if [[ "$command" == "hiddpair" ]]; then
    name="${*:4}"
    mac1="$3"
    mac=$(echo "$mac1" | grep -oEi "([0-9A-F]{2}[:-]){5}([0-9A-F]{2})" | tr '[:lower:]' '[:upper:]')
    macLowerCase=$(echo "$mac" | tr '[:upper:]' '[:lower:]')
    if [ "$?" != "0" ]; then
        exit 1
    fi
    recallog -s "${INIT_SCRIPT}" -t "BLUETOOTH" "Unpairing and removing BT device $mac"
    /recalbox/scripts/bluetooth/test-device remove "$mac"

    recallog -s "${INIT_SCRIPT}" -t "BLUETOOTH" "pairing $name $mac"
    echo "$name" | grep -i "8Bitdo\|other"
    if [ "$?" -eq "0" ]; then
        recallog -s "${INIT_SCRIPT}" -t "BLUETOOTH" "8Bitdo detected"
        if ! grep -q -i "$mac" /run/udev/rules.d/99-8bitdo.rules; then
            recallog -s "${INIT_SCRIPT}" -t "BLUETOOTH" "adding rule for $mac"
            echo "SUBSYSTEM==\"input\", ATTRS{uniq}==\"$macLowerCase\", MODE=\"0666\", ENV{ID_INPUT_JOYSTICK}=\"1\"" >> "/run/udev/rules.d/99-8bitdo.rules"
        fi
    fi

    /recalbox/scripts/bluetooth/recalpair "$mac" "$name"
    hcitool con | grep "$mac1"
    if [[ $? == "0" ]]; then
        connected=0
        recallog -s "${INIT_SCRIPT}" -t "BLUETOOTH" "bluetooth : $mac1 connected !"
        /recalbox/scripts/bluetooth/test-device trusted "$mac" yes
    else
        connected=1
        recallog -s "${INIT_SCRIPT}" -t "BLUETOOTH" "bluetooth : $mac1 failed connection"
    fi
    exit $connected
fi

if [[ "$command" == "storage" ]]; then
    if [[ "$mode" == "current" ]]; then
        if test -e $storageFile
        then
            SHAREDEVICE=`cat ${storageFile} | grep "^sharedevice=" | head -n1 | cut -d'=' -f2`
            [[ "$?" -ne "0" || "$SHAREDEVICE" == "" ]] && SHAREDEVICE=INTERNAL
            echo "$SHAREDEVICE"
        else
            echo "INTERNAL"
        fi
        exit 0
    fi
    if [[ "$mode" == "list" ]]; then
        echo "INTERNAL"
        echo "ANYEXTERNAL"
        echo "RAM"
        (blkid | grep -vE '^/dev/mmcblk' | grep ': LABEL="'
         blkid | grep -vE '^/dev/mmcblk' | grep -v ': LABEL="' | sed -e s+':'+': LABEL="NO_NAME"'+
        ) | sed -e s+'^[^:]*: LABEL="\([^"]*\)" UUID="\([^"]*\)" TYPE="[^"]*"$'+'DEV \2 \1'+
        exit 0
    fi
    if [[ "${mode}" == "INTERNAL" || "${mode}" == "ANYEXTERNAL" || "${mode}" == "RAM" || "${mode}" == "DEV" || "${mode}" == "NETWORK" ]]; then
        preBootConfig
        if [[ "${mode}" == "INTERNAL" || "${mode}" == "ANYEXTERNAL" || "${mode}" == "RAM" ]]; then
            if grep -qE "^sharedevice=" "${storageFile}"
            then
                sed -i "s|^sharedevice=.*|sharedevice=${mode}|g" "${storageFile}"
            else
                echo "sharedevice=${mode}" >> "${storageFile}"
            fi
        fi
        if [[ "${mode}" == "DEV" ]]; then
            if grep -qE "^sharedevice=" "${storageFile}"
            then
                sed -i "s|^sharedevice=.*|sharedevice=${mode} $extra1|g" "${storageFile}"
            else
               echo "sharedevice=${mode} ${extra1}" >> "${storageFile}"
            fi
        fi
    postBootConfig
    exit 0
    fi
fi

if [[ "$command" == "forgetBT" ]]; then
    for mac in $(bluetoothctl devices | awk '{print $2}') ; do
        recallog -s "${INIT_SCRIPT}" -t "BLUETOOTH" "Unpairing and removing BT device $mac"
        /recalbox/scripts/bluetooth/test-device remove "$mac"
    done
    exit 0
fi

if [[ "$command" == "getEmulatorDefaults" ]]; then
    emulator="$mode"
    python -c "import json; import sys; sys.path.append('/usr/lib/python3.9/site-packages/configgen'); import emulatorlauncher; print(json.dumps(emulatorlauncher.getDefaultEmulator('$emulator').config))"
    exit 0
fi

if [[ "$command" == "configbackup" ]]; then
    #Backup recalbox.conf to /boot partition
    preBootConfig
    cp /recalbox/share/system/recalbox.conf /boot/recalbox-backup.conf
    sed -i '1s/^/#THIS IS A BACKUP OF RECALBOX.CONF\n#PLEASE DO NOT MAKE ANY CHANGE HERE !!!\n\n\n/' /boot/recalbox-backup.conf
    postBootConfig
    recallog -s "${INIT_SCRIPT}" -t "BACKUP" "recalbox.conf saved to /boot partition"
    exit 0
fi

echo "Unknown command $command"
recallog -s "${INIT_SCRIPT}" -t "CONFIG" -e "recalbox-config.sh: unknown command $command"

exit 10
