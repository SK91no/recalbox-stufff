#!/bin/bash

# the aim of this script
# is to make things happend in this way :
# udev
# mounting /recalbox/share
# mounting other devices
#
# because:
# 1) there are 2 drivers for ntfs ; not the 2 are possible at the same time
# 2) for ntfs, if the device is not correctly removed, we've to try to fix (ntfsfix) or fallback in case of error, and the device must not be mounted
# 3) for some others actions, such as /dev/mmcblk0p3 fs growing, it must not be mounted

write_device_info() {
  set |
	  grep -E '^DEVNAME=|^ID_FS_USAGE=|^ID_FS_UUID=|^ID_FS_TYPE=|^ID_FS_LABEL=|^ACTION=|^XMOUNTS_TYPE=' |
	  sed -e s+'^'+'export '+ > /var/run/3mounts.delay/$(basename "$DEVNAME")
}

write_dynamic_samba_share() {
  local SHARE
  SHARE=${1//[^a-zA-Z0-9_\- ()]/ }
  cat <<EOF
[$SHARE]
comment = $2
path = $3
writeable = yes
guest ok = yes
create mask = 0644
directory mask = 0755
force user = root
veto files = /._*/.DS_Store/
delete veto files = yes
EOF
}

regenerate_dynamic_samba_shares() {
  local mountpoint
  local file
  rm /tmp/dynamic-share.conf
  find /var/run/3mounts.delay -type f | while read -r file; do
    /usr/bin/recallog -s 3mounts -t INFO "processing $file for samba share"
    (
      source "$file"
      mountpoint=$(grep -E "$DEVNAME\b" /proc/mounts| cut -d " " -f 2 | head -n1)
      if [ -n "$mountpoint" ]; then
        /usr/bin/recallog -s 3mounts -t INFO "mountpoint is $mountpoint"
        LABEL=${ID_FS_LABEL:-$ID_MODEL}
        LABEL=${LABEL:-$DEVNAME}
        write_dynamic_samba_share "${XMOUNTS_TYPE^} (${LABEL})" "Partition ${LABEL}" "$mountpoint" >>/tmp/dynamic-share.conf
      else
        /usr/bin/recallog -s 3mounts -t ERROR "something went wrong. $DEVNAME is not mounted"
      fi
    )
  done
}

/usr/bin/recallog -s 3mounts -t INFO "ACTION=$ACTION DEVNAME=$DEVNAME TYPE=$XMOUNTS_TYPE"
if [ "$ACTION" = add ] && mount | grep -q -E "^$DEVNAME\b"; then
    /usr/bin/recallog -s 3mounts -t WARNING "$DEVNAME already mounted, skipping..."
    exit 1
fi

if [ -f /tmp/disable.automount ]; then
    /usr/bin/recallog -s 3mounts -t WARNING "automount is disabled, skipping..."
    exit 0
fi

# handle remove, remove old file
if [ "$ACTION" = remove ] && [ -f /var/run/3mounts.delay/"$(basename "$DEVNAME")" ]; then
  rm /var/run/3mounts.delay/"$(basename "$DEVNAME")"
  3mounts remove
  /usr/bin/recallog -s 3mounts -t INFO "$DEVNAME removed"
  REGENERATE=true
fi

# sanitize
[ ! -d /var/run/3mounts.delay ] && mkdir -p /var/run/3mounts.delay

# save the context for later user
# it will be played by the S11share script after the mounting of /recalbox/share
# and used by this script to regenerate samba shares
if [ "$ACTION" = add ] && [ -n "$ID_FS_TYPE" ]; then
  write_device_info
  /usr/bin/recallog -s 3mounts -t INFO "$DEVNAME saved for future use"
  # if share is already mounted, just use the basic 3mounts
  if test -e /var/run/recalbox.share.mounted; then
    /usr/bin/3mounts "$1"
    ret=$?
    if [ $ret -eq 0 ]; then
      /usr/bin/recallog -s 3mounts -t INFO "$DEVNAME processed and mounted"
    else
      /usr/bin/recallog -s 3mounts -t ERROR "$DEVNAME not mounted, check error in syslog"
    fi
  fi
  REGENERATE=true
fi

if [ "$REGENERATE" = true ]; then
  /usr/bin/recallog -s 3mounts -t INFO "regenerate dynamic samba shares"
  regenerate_dynamic_samba_shares
fi
exit "$ret"

