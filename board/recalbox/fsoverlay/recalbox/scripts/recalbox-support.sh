#!/bin/bash


GTMP="/tmp"
DHOME="/recalbox/share/system"

# to be callable by any external tool
if test $# -eq 1
then
    REPORTNAME=$(basename "$1" | sed -e s+'^\([^.]*\)\..*$'+'\1'+)
    OUTPUTFILE=$1
else
    REPORTNAME="recalbox-support-"$(date +%Y%m%d%H%M%S)
    OUTPUTFILE="/recalbox/share/saves/${REPORTNAME}.tar.gz"
fi

TMPDIR="${GTMP}/${REPORTNAME}"

f_cp() {
    test -e "$1" && cp "$1" "$2"
}

d_cp() {
    test -d "$1" && cp -pr "$1" "$2"
}

MASK="******"

# Hide the value of the given keys, keeping the key itself readable.
# An empty value is left empty: support needs to tell "unset" from "set".
filter_keys() {
    local file="$1"; shift
    test -f "${file}" || return 0
    local key args=()
    for key in "$@"
    do
        args+=(-e "s/^\([[:space:]]*[;#]\{0,1\}[[:space:]]*${key}[[:space:]]*=[[:space:]]*\)..*$/\1${MASK}/")
    done
    test ${#args[@]} -eq 0 || sed -i "${args[@]}" "${file}"
}

filter_expr() {
    local file="$1"; shift
    test -f "${file}" || return 0
    local expr args=()
    for expr in "$@"
    do
        args+=(-e "${expr}")
    done
    test ${#args[@]} -eq 0 || sed -i "${args[@]}" "${file}"
}

# Secrets and personal data of recalbox.conf, shared with its /boot/recalbox-backup.conf copy.
RECALBOX_CONF_KEYS=(
    "system\.default_password" "patron\.privatekey"
    "global\.retroachievements\.username" "global\.retroachievements\.password"
    "scraper\.screenscraper\.user" "scraper\.screenscraper\.password"
    "global\.translate\.apikey" "global\.translate\.url"
    "global\.netplay\.nickname" "netplay\.password\.[0-9][0-9]*"
    "system\.hostname"
    "wifi[0-9]*\.ssid" "wifi[0-9]*\.key"
    "wifi[0-9]*\.ip" "wifi[0-9]*\.netmask" "wifi[0-9]*\.gateway" "wifi[0-9]*\.nameservers"
)

# Emulator configurations: the keys we write ourselves, then generic names for the
# cores writing their own ini file.
EMULATOR_CONF_KEYS=(
    "cheevos_username" "cheevos_password" "cheevos_token"
    "netplay_password" "netplay_spectate_password" "netplay_nickname"
    "youtube_stream_key" "twitch_stream_key" "facebook_stream_key"
    "Nickname" "IndexName" "IndexPassword" "HostCode"
    "[Uu]sername" "[Pp]assword" "[Pp]asswd" "[Pp]assphrase" "[Tt]oken" "ApiToken"
    "[Aa]pi[Kk]ey" "api_key" "[Ss]ecret" "[Pp]rivate[Kk]ey" "private_key" "psk" "PSK"
)

# Secrets buried in free text: logs, command lines, command outputs.
FREE_TEXT_EXPR=(
    's|\(://\)[^/:@]*:[^/@]*@|\1***:***@|g'
    's/\([Bb]earer[[:space:]][[:space:]]*\)[A-Za-z0-9._~+/=-][A-Za-z0-9._~+/=-]*/\1'"${MASK}"'/g'
    's/\(-netplay_\(player\|viewer\)password[[:space:]][[:space:]]*\)"[^"]*"/\1"'"${MASK}"'"/g'
    's/\([?&]\(ssid\|sspassword\|devpassword\|devid\|apikey\|api_key\|token\|password\|user\)=\)[^&"[:space:]]*/\1'"${MASK}"'/g'
    's/\([[:space:],:]\([Pp]assword\|[Pp]asswd\|[Uu]sername\|[Pp]assphrase\|[Tt]oken\|credentials\|domain\)=\)[^,[:space:])]\{1,\}/\1'"${MASK}"'/g'
    # connman service identifiers carry the ssid, hex encoded.
    's/\(wifi_[0-9a-f]\{6,\}_\)[0-9a-fA-F]\{2,\}\(_managed_\)/\1'"${MASK}"'\2/g'
)


if mkdir "${TMPDIR}" && mkdir "${TMPDIR}/"{system,joysticks,lirc,kodi}
then
    if ! cd "${TMPDIR}"
    then
	echo "Change directory failed" >&2
	exit 1
    fi
else
    echo "Reporting directory creation failed" >&2
    exit 1
fi

# SYSTEM
DSYSTEM="${TMPDIR}/system"
dmesg                   > "${DSYSTEM}/dmesg.txt"
lsmod                   > "${DSYSTEM}/lsmod.txt"
ps aux                  > "${DSYSTEM}/ps.txt"
df -h                   > "${DSYSTEM}/df.txt"
netstat -tuan           > "${DSYSTEM}/netstat.txt"
lsusb -v                > "${DSYSTEM}/lsusb.txt" 2>/dev/null
tvservice -m CEA        > "${DSYSTEM}/tvservice-CEA.txt"
tvservice -m DMT        > "${DSYSTEM}/tvservice-DMT.txt"
tvservice -s            > "${DSYSTEM}/tvservice-status.txt"
ifconfig -a             > "${DSYSTEM}/ifconfig.txt"
lspci                   > "${DSYSTEM}/lspci.txt"
amixer                  > "${DSYSTEM}/amixer.txt"
aplay -l                > "${DSYSTEM}/aplay-l.txt"
DISPLAY=:0.0 glxinfo    > "${DSYSTEM}/glxinfo.txt"
DISPLAY=:0.0 xrandr     > "${DSYSTEM}/xrandr.txt"
DISPLAY=:0.0 xrandr -q  > "${DSYSTEM}/xrandr-q.txt"
connmanctl technologies > "${DSYSTEM}/connman-technologies.txt"
connmanctl services     > "${DSYSTEM}/connman-services.txt"
mount                   > "${DSYSTEM}/mount.txt"
blkid                   > "${DSYSTEM}/blkid.txt"
hciconfig -a            > "${DSYSTEM}/hciconfig.txt"
free -h                 > "${DSYSTEM}/free.txt"
for file in /sys/class/drm/card*/edid; do
    echo "$file ->"
    base64 "$file"
    echo -n "status: "
    cat "${file//edid/status}"
    echo -n "enabled?: "
    cat "${file//edid/enabled}"
    echo "modes: "
    cat "${file//edid/modes}"
    echo "----------------------------------------------------------------------------"
done > "${DSYSTEM}/edid.txt"

# Emulator command lines carry the netplay passwords, and connman reports every network around.
filter_expr "${DSYSTEM}/ps.txt" "${FREE_TEXT_EXPR[@]}"
filter_expr "${DSYSTEM}/connman-services.txt" \
    "s/^\(.\{3\}\).*[[:space:]]\(wifi_[0-9a-f]*_\)[0-9a-fA-F]*\(_managed_.*\)$/\1 ${MASK} \2${MASK}\3/"
filter_keys "${DSYSTEM}/connman-technologies.txt" "Tethering\.\{0,1\}Identifier" "Tethering\.\{0,1\}Passphrase"

mkdir "${DSYSTEM}/boot"
f_cp /recalbox/recalbox.version                                   "${DSYSTEM}"
f_cp /recalbox/recalbox.arch                                      "${DSYSTEM}"
f_cp /recalbox/share/system/recalbox.conf                         "${DSYSTEM}"
d_cp /recalbox/share/system/logs                                  "${DSYSTEM}"
f_cp /var/log/messages                                            "${DSYSTEM}"
f_cp /var/log/recalbox-bluetooth.log                              "${DSYSTEM}"
f_cp /recalbox/share/system/.emulationstation/systemlist.xml      "${DSYSTEM}"
f_cp /recalbox/share/system/logs/es_log.txt                       "${DSYSTEM}"
f_cp /recalbox/share/system/.emulationstation/es_input.cfg        "${DSYSTEM}"
f_cp /boot/config.txt                                             "${DSYSTEM}/boot"
f_cp /boot/recalbox-user-config.txt                               "${DSYSTEM}/boot"
f_cp /boot/recalbox-boot.conf                                     "${DSYSTEM}/boot"
f_cp /boot/hardware.log                                           "${DSYSTEM}/boot"
d_cp /boot/bootlogs                                               "${DSYSTEM}/boot"
f_cp /boot/update.log                                             "${DSYSTEM}/boot"
f_cp /boot/boot/bl-update.log                                     "${DSYSTEM}/boot"
d_cp /boot/crt                                                    "${DSYSTEM}/boot"
f_cp /boot/recalbox-backup.conf                                   "${DSYSTEM}/boot"
f_cp /var/log/Xorg.0.log                                          "${DSYSTEM}"
f_cp /recalbox/share_init/system/.emulationstation/systemlist.xml "${DSYSTEM}/share_init_systemlist.xml"
f_cp /recalbox/share/system/.emulationstation/systemlist.xml      "${DSYSTEM}/share_systemlist.xml"
find /recalbox/share/roms/ -type f \( ! -iname "*.txt" ! -iname "*.xml" ! -iname "*.png" ! -iname "*.jpg" ! -iname "*.dat" \) ! -path "*/data/*" | wc -l > "${DSYSTEM}/approxnbroms.txt"

# Configuration and logs filters. They only ever run on the copies under ${TMPDIR}.
filter_keys "${DSYSTEM}/recalbox.conf"             "${RECALBOX_CONF_KEYS[@]}"
filter_keys "${DSYSTEM}/boot/recalbox-backup.conf" "${RECALBOX_CONF_KEYS[@]}"
# Network share credentials live inside the sharenetwork_* values.
filter_expr "${DSYSTEM}/boot/recalbox-boot.conf" "${FREE_TEXT_EXPR[@]}"
filter_expr "${DSYSTEM}/es_log.txt"              "${FREE_TEXT_EXPR[@]}"
filter_expr "${DSYSTEM}/messages"                "${FREE_TEXT_EXPR[@]}"
filter_expr "${DSYSTEM}/mount.txt"               "${FREE_TEXT_EXPR[@]}"
for LOG in "${DSYSTEM}"/logs/*
do
    filter_expr "${LOG}" "${FREE_TEXT_EXPR[@]}"
done

# Themes
ls -1 /recalbox/share/themes                               > "${DSYSTEM}/share_themes.txt"
ls -1 /recalbox/share/system/.emulationstation/themes      > "${DSYSTEM}/share_es_themes.txt"
ls -1 /recalbox/share_init/system/.emulationstation/themes > "${DSYSTEM}/share_init_es_themes.txt"

# Recalbox intros
ls -l /recalbox/system/resources/splash/ | tail -n +2 | awk '{print $9, "-", $5}' > "${DSYSTEM}/recalbox_intros.txt"

# Update logs
d_cp /recalbox/share/system/upgrade "${DSYSTEM}"
find "${DSYSTEM}/upgrade" -type f ! -name "*upgrade*" | xargs rm
for LOG in "${DSYSTEM}"/upgrade/*
do
    filter_expr "${LOG}" "${FREE_TEXT_EXPR[@]}"
done

# Emulators configs
test -d "/recalbox/share/system/configs" && rsync -a "/recalbox/share/system/configs" "${TMPDIR}" --exclude "ppsspp/PSP/SAVEDATA" --exclude "ppsspp/PSP/PPSSPP_STATE" --exclude "retroarch/overlays"
test -d "/recalbox/share/system/.config" && rsync -a "/recalbox/share/system/.config" "${TMPDIR}"

# Text extensions only: sed must never run on a binary, and busybox grep cannot detect one.
# Extend this list when a new configuration file lands in the archive.
find "${TMPDIR}/configs" "${TMPDIR}/.config" -type f \
     \( -name "*.cfg" -o -name "*.ini" -o -name "*.conf" -o -name "*.opt" -o -name "*.json" \) 2>/dev/null |
while IFS= read -r CONF
do
    filter_keys "${CONF}" "${EMULATOR_CONF_KEYS[@]}"
    case "${CONF}" in
        # Moonlight stores its client TLS private key and certificate there.
        *Moonlight.conf) filter_keys "${CONF}" "key" "certificate" "uuid" ;;
    esac
done

# bluetooth
DBT="${TMPDIR}/bluetooth"
mkdir "${DBT}"
rfkill list > "${DBT}/rfkill.txt" 2>&1
{
    echo "=========== list ============";    timeout 5 bluetoothctl -- list
    echo "=========== show ============";    timeout 5 bluetoothctl -- show
    echo "=========== devices ============"; timeout 5 bluetoothctl -- devices
} > "${DBT}/bluetoothctl.txt" 2>&1
f_cp /var/log/recalbox-bluetooth.log   "${DBT}"
f_cp /var/log/recalbox-bluetooth.log.1 "${DBT}"
f_cp /etc/bluetooth/main.conf          "${DBT}"
f_cp /etc/bluetooth/input.conf         "${DBT}"
f_cp /etc/modprobe.d/bluetooth.conf    "${DBT}/modprobe-bluetooth.conf"
# bluez registry: structure and flags matter for support, link keys never leave the box
# bluez5_utils is built with --localstatedir=/overlay/.configs/
for BTSTORE in /overlay/.configs/lib/bluetooth /var/lib/bluetooth
do
    test -d "${BTSTORE}" || continue
    find "${BTSTORE}" > "${DBT}/bluez-store-tree.txt" 2>&1
    find "${BTSTORE}" -name info 2>/dev/null | while IFS= read -r INFO
    do
        DEST="${DBT}/store-$(echo "${INFO}" | sed -e "s|${BTSTORE}/||" -e 's|/|_|g').txt"
        sed -e "s/^Key=.*/Key=${MASK}/" "${INFO}" > "${DEST}"
    done
    break
done

# wifi: why an association never happens is invisible without the supplicant's own
# words. The scan seeing networks but the link never coming up is read here.
DWIFI="${TMPDIR}/wifi"
mkdir "${DWIFI}"
iw reg get > "${DWIFI}/iw-reg.txt" 2>&1
iw dev    > "${DWIFI}/iw-dev.txt" 2>&1
for IF in $(iw dev 2>/dev/null | awk '/Interface/{print $2}'); do
    {
        echo "=========== link ============";    iw dev "${IF}" link
        echo "=========== info ============";    iw dev "${IF}" info
    } > "${DWIFI}/iw-${IF}.txt" 2>&1
    {
        echo "=========== status ============";        timeout 5 wpa_cli -i "${IF}" status
        echo "=========== list_networks ============"; timeout 5 wpa_cli -i "${IF}" list_networks
        echo "=========== scan_results ============";  timeout 5 wpa_cli -i "${IF}" scan_results
    } > "${DWIFI}/wpa_cli-${IF}.txt" 2>&1
done
f_cp /var/log/wpa_supplicant.log "${DWIFI}"
# The supplicant config carries the PSK in clear: it never leaves the box.
if test -f /overlay/.configs/wpa_supplicant.conf; then
    sed -e 's/\(psk=\).*/\1******/' \
        -e 's/\(sae_password=\).*/\1******/' \
        -e 's/\(wep_key[0-9]*=\).*/\1******/' \
        /overlay/.configs/wpa_supplicant.conf > "${DWIFI}/wpa_supplicant.conf"
fi
# The log names every network around, and share credentials can appear in URLs.
filter_expr "${DWIFI}/wpa_supplicant.log" "${FREE_TEXT_EXPR[@]}"

# joysticks
DJOYS="${TMPDIR}/joysticks"
find /dev/input > "${DJOYS}/inputs.txt"
for J in /dev/input/event*
do
    N=$(basename ${J})
    evtest --info "${J}"          > "${DJOYS}/evtest.${N}.txt"
    udevadm info -q all -n "${J}" > "${DJOYS}/udevadm.${N}.txt"
done
sdl2-jstest -l > "${DJOYS}/sdl2-jstest.txt"


# lirc
DLIRC="${TMPDIR}/lirc"
f_cp "${DHOME}/.config/lirc/lircd.conf" "${DLIRC}"

# kodi
DKODI="${TMPDIR}/kodi"
f_cp "${DHOME}/.kodi/userdata/Lircmap.xml"                                                   "${DKODI}"
f_cp "${DHOME}/.kodi/userdata/keymaps/recalbox.xml"                                          "${DKODI}"
d_cp "${DHOME}/.kodi/userdata/remotes"                                                       "${DKODI}"
d_cp "${DHOME}/.kodi/userdata/addon_data/peripheral.joystick/resources/buttonmaps/xml/linux" "${DKODI}"
test -e "${DHOME}/.kodi/temp/kodi.log" && tail -4000 "${DHOME}/.kodi/temp/kodi.log" > "${DKODI}/kodi.log"
# Kodi logs its sources, smb://user:password@host among them.
filter_expr "${DKODI}/kodi.log" "${FREE_TEXT_EXPR[@]}"

# pulseaudio
PA_CMDS="list-modules list-sinks list-sources list-clients list-cards list-samples stat dump"
for PA_CMD in $PA_CMDS; do
    echo "=========== $PA_CMD ============" >>"${DSYSTEM}/pulseaudio.txt"
    pacmd "$PA_CMD" >>"${DSYSTEM}/pulseaudio.txt" 2>&1
done

if ! (cd "${GTMP}" && tar cf -  "${REPORTNAME}" | gzip -c > "${OUTPUTFILE}")
then
    echo "Reporting zip creation failed" >&2
    exit 1
fi

rm -rf "${TMPDIR}"
echo "${OUTPUTFILE}"
exit 0
