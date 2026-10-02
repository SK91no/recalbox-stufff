#!/bin/bash

global_interface=/var/run/wpa_supplicant_global
config_file=/recalbox/share/system/recalbox.conf
config_file_boot=/boot/recalbox-backup.conf
wpa_file=/overlay/.configs/wpa_supplicant.conf
INIT_SCRIPT=$(basename "$0")
system_setting="recalbox_settings"

# if /recalbox/share is not yet mounted
if ! [ -f "$config_file" ]; then
  # use the boot version of the file
  config_file="$config_file_boot"
fi

mask2cidr() {
    nbits=0
    IFS=.
    for dec in $1 ; do
        case $dec in
            255) let nbits+=8;;
            254) let nbits+=7;;
            252) let nbits+=6;;
            248) let nbits+=5;;
            240) let nbits+=4;;
            224) let nbits+=3;;
            192) let nbits+=2;;
            128) let nbits+=1;;
            0);;
            *) echo "Error: $dec is not recognised"; exit 1
        esac
    done
    echo "$nbits"
}

wpa_supplicant_command() {
  wpa_cli -g "${global_interface}" "$@" >/dev/null
}

# Function to create wifi profiles based on user settings
rb_wifi_configure() {
  [ "$1" = "1" ] && X="" || X="$1"
  local interface="$2"
  settings_ssid=$("$system_setting" -command load -key "wifi${X}.ssid" -source "$config_file")
  settings_key=$("$system_setting" -command load -key "wifi${X}.key" -source "$config_file")
  settings_ip=$("$system_setting" -command load -key "wifi${X}.ip" -source "$config_file")
  settings_gateway=$("$system_setting" -command load -key "wifi${X}.gateway" -source "$config_file")
  settings_netmask=$("$system_setting" -command load -key "wifi${X}.netmask" -source "$config_file")
  settings_nameservers=$("$system_setting" -command load -key "wifi${X}.nameservers" -source "$config_file")
  settings_priority=$("$system_setting" -command load -key "wifi${X}.priority" -source "$config_file" -default "$((6-$1))")

  # setup wpa_supplicant network
  if [[ "$settings_ssid" != "" ]] ;then

    echo "Configuring wifi for SSID: $settings_ssid"
    network=$(wpa_cli -i "$interface" add_network)
    echo "new wifi network: $network"
    echo -n "Setting network ssid:"
    wpa_cli -i "$interface" set_network "$network" ssid "\"$settings_ssid\"" || exit 1
    if [ -n "$settings_key" ]; then
        # Connect to protected wifi
        echo -n "Setting network psk:"
        if [ ${#settings_key} -lt 64 ]; then # compatible with encoded psk
            settings_key="\"$settings_key\""
        fi
        wpa_cli -i "$interface" set_network "$network" psk "$settings_key" || exit 1
        echo -n "setting network key_mgmt:"
        wpa_cli -i "$interface" set_network "$network" key_mgmt WPA-PSK WPA-EAP WPA-PSK-SHA256 NONE SAE || exit 1
        echo -n "Setting network sae_password:"
        wpa_cli -i "$interface" set_network "$network" sae_password "$settings_key" || exit 1
        echo -n "Setting network ieee80211w:"
        wpa_cli -i "$interface" set_network "$network" ieee80211w 1 || exit 1
    else
        # Connect to open ssid
        echo -n "Setting network key_mgmt:"
        wpa_cli -i "$interface" set_network "$network" key_mgmt NONE || exit 1
    fi
    [ -n "$settings_priority" ] && wpa_cli -i "$interface" set_network "$network" priority "$settings_priority"
    echo -n "Setting network scan_ssid:"
    wpa_cli -i "$interface" set_network "$network" scan_ssid 1 || exit 1
    echo -n "Enabling network:"
    wpa_cli -i "$interface" enable_network "$network" || exit 1

    # static ip configuration in dhcpcd.conf
    mount -o remount,rw /
    sed -i "/\b\($interface\|static\)\b/d" /etc/dhcpcd.conf
    if [[ "$settings_ip" != "" ]] && \
      [[ "$settings_gateway" != "" ]] && \
      [[ "$settings_netmask" != "" ]] && \
      [[ "$settings_nameservers" != "" ]]; then
      echo "Static ip configuration"
      settings_netmask=$(mask2cidr "$settings_netmask")
      echo "Static ip_address=$settings_ip/$settings_netmask"
      echo "Static routers=$settings_gateway"
      echo "Static domain_name_servers=$settings_nameservers"
      {
        echo "interface $interface"
        echo "static ip_address=$settings_ip/$settings_netmask"
        echo "static routers=$settings_gateway"
        echo "static domain_name_servers=$settings_nameservers"
      } >> /etc/dhcpcd.conf
    fi
    mount -o remount,ro /
  fi
}

# function to drop all of the wpa_suppliment networks
cleanup_interface() {
  # clear wpa_supplicant configuration and rebuild
  local interface="$1"
  # limit any infinite loop to 10 max
  for i in {1..10}; do

    # get the last network id
    netid=$(wpa_cli -i "$interface" list_networks | tail -n1 | cut -d '	' -f 1)
    netid=$(echo "$netid" | tr " " "\n")

    # exit at the header record
    if [[ $netid == network* ]]; then
      break
    fi

    # remove network id
    echo -n "Removing network:"
    wpa_cli -i "$interface" remove_network "$netid"
  done
}

configure_interface() {
  local interface="$1"
  echo -n "Enabling update_config:"
  wpa_cli -i "$interface" set update_config 1

  settings_region=$("$system_setting" -command load -key "wifi.region" -source "$config_file")
  echo -n "Set country:"
  wpa_cli -i "$interface" set country "$settings_region"

  cleanup_interface "$interface"

  settings_connect=$("$system_setting" -command load -key "wifi.connect" -source "$config_file" -default "1")
  # iterate through all network ...
  if [[ "$settings_connect" == "1" ]]; then
    for i in {1..3}; do
      rb_wifi_configure "$i" "$interface"
    done
  fi

  # write wpa_supplicant configuration
  echo -n "Saving wifi configuration:"
  wpa_cli -i "$interface" save_config

  [[ "$settings_connect" == "1" ]] && ensure_connected "$interface"
}

# brcmfmac's scheduled scan intermittently fails right after boot: wpa is left
# enabled on the network but stuck in SCANNING, never associating. A normal scan
# works and a reconnect associates — the very recovery a manual wifi off/on does.
# Nudge until the link is up or the budget runs out; a no-op once connected.
ensure_connected() {
  local interface="$1"
  # nothing to wait for when no network is configured
  wpa_cli -i "$interface" list_networks 2>/dev/null | grep -q '^[0-9]' || return 0
  local try
  for try in $(seq 1 10); do
    [ "$(wpa_cli -i "$interface" status 2>/dev/null | sed -n 's/^wpa_state=//p')" = "COMPLETED" ] && return 0
    wpa_cli -i "$interface" scan >/dev/null 2>&1
    wpa_cli -i "$interface" reconnect >/dev/null 2>&1
    sleep 3
  done
}

# start wpa_supplicant
enable_wifi() {
  local interface
  pgrep wpa_supplicant >/dev/null || wpa_supplicant -B -M -i wlan* -c "$wpa_file" -Dnl80211,wext -u -f /var/log/wpa_supplicant.log -g "${global_interface}"
  while ! wpa_supplicant_command interface; do
    sleep 0.2
  done
  # enable wifi for plugged interface
  wpa_cli -g /var/run/wpa_supplicant_global interface | sed '1,/^Available interface/d' | while read -r interface; do
    configure_interface "$interface"
  done
}

# stop wpa_supplicant
disable_wifi() {
  wpa_supplicant_command terminate
  local COUNT=50
  while pgrep wpa_supplicant >/dev/null; do
    sleep 0.2;
    COUNT=$((COUNT-1))
    if [ "$COUNT" -eq 0 ]; then
      pkill -9 wpa_supplicant
      break
    fi
  done
}

set_default_config() {
  [ ! -s "${wpa_file}" ] && echo "# WPA config file" >"${wpa_file}"
  sed -i -e '/^ap_scan=/{h;s/=.*/=1/};${x;/^$/{s//ap_scan=1/;H};x}' "${wpa_file}"
  sed -i -e '/^ctrl_interface=/{h;s#=.*#=/var/run/wpa_supplicant#};${x;/^$/{s##ctrl_interface=/var/run/wpa_supplicant#;H};x}' "${wpa_file}"
}
