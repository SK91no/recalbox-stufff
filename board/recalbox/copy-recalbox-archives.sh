#!/bin/bash -e

# PWD = buildroot dir
# BASE_DIR = output/ dir
# BUILD_DIR = output/build
# HOST_DIR = output/host
# BINARIES_DIR = output/images
# TARGET_DIR = output/target

# List of files not upgraded (typically user modified files)
EXCLUDED_FILE_LIST=(boot.lst config.ini recalbox-user-config.txt recalbox-boot.conf crt/recalbox-crt-config.txt crt/recalbox-crt-options.cfg)

# $1 boot directory
generate_boot_file_list() {
  pushd "$1" >/dev/null
  find . -type f -printf '%P\n' | \
		grep -v -E "$(IFS="|";  echo "^(${EXCLUDED_FILE_LIST[*]})$")"
  popd >/dev/null
}

compute_md5() {
	# create boot.md5
	pushd "$1" >/dev/null || return 1
	find . ! -name boot.md5 -type f -exec md5sum {} \; | \
    grep -v -E "$(IFS="|"; echo "(${EXCLUDED_FILE_LIST[*]})")" || return 1
	popd >/dev/null
}

generate_recalbox_tar_xz() {
	local source_dir="$1"
	local dest_file="$2"
	# recalbox.tar.xz (formerly boot.tar.xz)
	# Pipe tar into xz explicitly so we can pass --threads=0; `tar -cJf`
	# would invoke xz single-threaded.
	(set -o pipefail; tar -C "$source_dir" -cf - . | xz -T0 -9 > "$dest_file")
}

generate_recalbox_img() {
	local root_path="$1"
	local input_path="$2"
	local output_path="$3"
	local tmp_path="$4"
	# clean temp dir
	rm -rf "${tmp_path}" || return 1

	genimage --rootpath="${root_path}" --inputpath="${input_path}" --outputpath="${output_path}" --config="${input_path}/genimage.cfg" --tmppath="${tmp_path}" || return 1
	rm -f "${output_path}/boot.vfat" || return 1
	rm -f "${input_path}/rootfs.tar" || return 1
	rm -f "${input_path}/rootfs.squashfs" || return 1
	sync
}

# prepare genimage.cfg, replace @files section with list of files
# $1=genimage source file
prepare_genimage_cfg() {
	local genimage_cfg_source="$1"
	FILES=$(find "${BINARIES_DIR}/boot-data" -type f | sed -e s+"^${BINARIES_DIR}/boot-data/\(.*\)$"+"file \1 \{ image = 'boot-data/\1' }"+ | tr '\n' '@')
	sed -e s+'@files'+"${FILES}"+ "${genimage_cfg_source}" | tr '@' '\n' > "${BINARIES_DIR}/genimage.cfg" || exit 1
}

RECALBOX_BINARIES_DIR="${BINARIES_DIR}/recalbox"

[[ -d "${RECALBOX_BINARIES_DIR}" ]] && rm -rf "${RECALBOX_BINARIES_DIR}"
mkdir -p "${RECALBOX_BINARIES_DIR}"

[[ -d "${BINARIES_DIR}/boot-data" ]] || mkdir -p "${BINARIES_DIR}/boot-data"

# XU4, RPI1, RPI2 or RPI3
RECALBOX_TARGET=$(grep -E "^BR2_PACKAGE_RECALBOX_TARGET_[A-Z_0-9]*=y$" "${BR2_CONFIG}" | sed -e s+'^BR2_PACKAGE_RECALBOX_TARGET_\([A-Z_0-9]*\)=y$'+'\1'+)

RECALBOX_TARGET_LOWER=$(echo "${RECALBOX_TARGET}" | tr '[:upper:]' '[:lower:]')

RECALBOX_IMG="${RECALBOX_BINARIES_DIR}/recalbox-${RECALBOX_TARGET_LOWER}.img"

echo -e "\n----- Generating images/recalbox files -----\n"

case "${RECALBOX_TARGET}" in
    RPI1|RPI3|RPI4|RPI4_64|RPI5_64|RPIZERO2LEGACY|RPIZERO2)
	# /boot
	echo "generating boot"
	[ -d "${BINARIES_DIR}/rpi-firmware" ] && cp -rf "${BINARIES_DIR}/rpi-firmware/." "${BINARIES_DIR}/boot-data/"
	cp -f "${BINARIES_DIR}/"*.dtb "${BINARIES_DIR}/boot-data"
	cp -r "${BINARIES_DIR}/overlays" "${BINARIES_DIR}/boot-data/" || exit 1
	rm -rf "${BINARIES_DIR}/boot-data/boot" || exit 1
	mkdir -p "${BINARIES_DIR}/boot-data/boot" || exit 1
  [ -f "${BINARIES_DIR}/zImage" ] && KERNEL=zImage || KERNEL=Image 
	cp "${BINARIES_DIR}/${KERNEL}" "${BINARIES_DIR}/boot-data/boot/linux" || exit 1
	cp "${BINARIES_DIR}/initrd.gz" "${BINARIES_DIR}/boot-data/boot" || exit 1
	cp "${BINARIES_DIR}/rootfs.squashfs" "${BINARIES_DIR}/boot-data/boot/recalbox" || exit 1

	GENIMAGE_CFG="${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/rpi/genimage.cfg"
	;;

	ODROIDXU4)
	rm -rf "${BINARIES_DIR}/boot-data/boot" || exit 1
	mkdir -p "${BINARIES_DIR}/boot-data/boot" || exit 1

	# /boot
	echo "generating boot"
	cp "${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/odroidxu4/boot.ini" "${BINARIES_DIR}/boot-data/boot.ini" || exit 1
	cp "${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/odroidxu4/config.ini" "${BINARIES_DIR}/boot-data/config.ini" || exit 1
	cp "${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/odroidxu4/boot.ini" "${BINARIES_DIR}/boot-data/boot.ini.sample" || exit 1
	cp "${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/odroidxu4/config.ini" "${BINARIES_DIR}/boot-data/config.ini.sample" || exit 1
	mkdir -p "${BINARIES_DIR}/boot-data/overlays"
	for overlay in ads7846 hktft32 hktft35 hktft-cs-ogst \
	               i2c0 i2c1 spi0 sx865x-i2c1 uart0; do
		cp "${BINARIES_DIR}/${overlay}.dtb" "${BINARIES_DIR}/boot-data/overlays/${overlay}.dtbo" || exit 1
	done
	cp "${BINARIES_DIR}/exynos5422-odroidxu4.dtb" "${BINARIES_DIR}/boot-data/boot" || exit 1
	cp "${BINARIES_DIR}/uInitrd" "${BINARIES_DIR}/boot-data/boot/" || exit 1
	cp "${BINARIES_DIR}/zImage" "${BINARIES_DIR}/boot-data/boot/linux" || exit 1
	cp "${BINARIES_DIR}/rootfs.squashfs" "${BINARIES_DIR}/boot-data/boot/recalbox" || exit 1

  EXCLUDED_FILE_LIST+=(boot.ini)
	GENIMAGE_CFG="${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/odroidxu4/genimage.cfg"
	;;

	ODROIDGO2)
	rm -rf "${BINARIES_DIR}/boot-data/boot" || exit 1
	mkdir -p "${BINARIES_DIR}/boot-data/boot" || exit 1

	# /boot
	echo "generating boot"
	cp "${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/odroidgo2/boot.ini" "${BINARIES_DIR}/boot-data/boot.ini" || exit 1
	cp "${BINARIES_DIR}/rk3326-odroidgo2-linux-v11.dtb" "${BINARIES_DIR}/boot-data" || exit 1
	cp "${BINARIES_DIR}/rk3326-odroidgo2-linux.dtb" "${BINARIES_DIR}/boot-data" || exit 1
	cp "${BINARIES_DIR}/rk3326-odroidgo3-linux.dtb" "${BINARIES_DIR}/boot-data" || exit 1
	cp "${BINARIES_DIR}/rk3326-rg351p-linux.dtb" "${BINARIES_DIR}/boot-data" || exit 1
  cp "${BINARIES_DIR}/rk3326-rg351v-linux.dtb" "${BINARIES_DIR}/boot-data" || exit 1
  cp "${BINARIES_DIR}/rk3326-rg351v-linux-oc.dtb" "${BINARIES_DIR}/boot-data" || exit 1
  cp "${BINARIES_DIR}/rk3326-rg351v-linux-timing_fix.dtb" "${BINARIES_DIR}/boot-data" || exit 1
  cp "${BINARIES_DIR}/rk3326-rg351v-linux-oc-timing_fix.dtb" "${BINARIES_DIR}/boot-data" || exit 1
  cp "${BINARIES_DIR}/rk3326-rg351mp-linux.dtb" "${BINARIES_DIR}/boot-data" || exit 1
  cp "${BINARIES_DIR}/rk3326-rg351mp-linux-oc.dtb" "${BINARIES_DIR}/boot-data" || exit 1
  cp "${BINARIES_DIR}/rk3326-rg351mp-linux-oc-timing_fix.dtb" "${BINARIES_DIR}/boot-data" || exit 1
  cp "${BINARIES_DIR}/rk3326-rg351mp-linux-timing_fix.dtb" "${BINARIES_DIR}/boot-data" || exit 1
	cp "${BINARIES_DIR}/uInitrd" "${BINARIES_DIR}/boot-data/boot/" || exit 1
	cp "${BINARIES_DIR}/Image" "${BINARIES_DIR}/boot-data/boot/linux" || exit 1
	cp "${BINARIES_DIR}/rootfs.squashfs" "${BINARIES_DIR}/boot-data/boot/recalbox" || exit 1

	GENIMAGE_CFG="${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/odroidgo2/genimage.cfg"
	;;

	RG353X)
	rm -rf "${BINARIES_DIR}/boot-data/boot" || exit 1
	mkdir -p "${BINARIES_DIR}/boot-data/boot" || exit 1
	mkdir -p "${BINARIES_DIR}/boot-data/boot/extlinux" || exit 1
	mkdir -p "${BINARIES_DIR}/boot-data/boot/dtb/rockchip" || exit 1

	# /boot
	echo "generating boot"
	cp "${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/anbernic/rg353x/extlinux.conf" "${BINARIES_DIR}/boot-data/boot/extlinux/extlinux.conf" || exit 1
	cp "${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/anbernic/boot-640x480.ppm" "${BINARIES_DIR}/boot-data/boot.ppm" || exit 1
	for dtb in rk3566-anbernic-rg353ps.dtb rk3566-anbernic-rg353vs.dtb   rk3566-anbernic-rg503.dtb \
             rk3566-anbernic-rg353p.dtb  rk3566-anbernic-rg353v.dtb    rk3566-powkiddy-rk2023.dtb \
             rk3566-anbernic-rg-arc-d.dtb rk3566-anbernic-rg-arc-s.dtb \
             rk3566-powkiddy-rgb30.dtb   rk3566-powkiddy-rgb10max3.dtb; do
	  cp "${BINARIES_DIR}/${dtb}" "${BINARIES_DIR}/boot-data/boot/dtb/rockchip/" || exit 1
	done
	cp "${BINARIES_DIR}/initrd.gz" "${BINARIES_DIR}/boot-data/boot/initrd.gz" || exit 1
	cp "${BINARIES_DIR}/Image" "${BINARIES_DIR}/boot-data/boot/linux" || exit 1
	cp "${BINARIES_DIR}/rootfs.squashfs" "${BINARIES_DIR}/boot-data/boot/recalbox" || exit 1
	# split uboot, genimage does not allow flashing image bigger than partition
	dd if="${BINARIES_DIR}/u-boot-rockchip.bin" of="${BINARIES_DIR}/u-boot-rockchip-idbloader.bin" bs=512 count=16320 || exit 1
	dd if="${BINARIES_DIR}/u-boot-rockchip.bin" of="${BINARIES_DIR}/u-boot-rockchip-uboot.bin" bs=512 skip=16320 || exit 1
	mkdir -p "${BINARIES_DIR}/boot-data/boot/bootloader/64" || exit 1
	cp "${BINARIES_DIR}/u-boot-rockchip.bin" "${BINARIES_DIR}/boot-data/boot/bootloader/64/" || exit 1

	GENIMAGE_CFG="${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/anbernic/rg353x/genimage.cfg"
	;;

	X86_64)
	# /boot
	rm -rf ${BINARIES_DIR}/boot-data/boot || exit 1
	mkdir -p ${BINARIES_DIR}/boot-data/boot/grub || exit 1
	cp "${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/grub2/grub.cfg" ${BINARIES_DIR}/boot-data/boot/grub/grub.cfg || exit 1
	cp "${BINARIES_DIR}/bzImage" "${BINARIES_DIR}/boot-data/boot/linux" || exit 1
	cp "${BINARIES_DIR}/initrd.gz" "${BINARIES_DIR}/boot-data/boot" || exit 1
	cp "${BINARIES_DIR}/rootfs.squashfs" "${BINARIES_DIR}/boot-data/boot/recalbox" || exit 1
	mkdir -p "${BINARIES_DIR}/boot-data/EFI/BOOT" || exit 1
	cp "${BINARIES_DIR}/efi-part/EFI/BOOT/bootia32.efi" "${BINARIES_DIR}/boot-data/EFI/BOOT" || exit 1
	cp "${BINARIES_DIR}/efi-part/EFI/BOOT/bootx64.efi" "${BINARIES_DIR}/boot-data/EFI/BOOT" || exit 1
	cp "${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/grub2/grub.cfg" "${BINARIES_DIR}/boot-data/EFI/BOOT" || exit 1

	# copy grub
	cp "${TARGET_DIR}/lib/grub/i386-pc/boot.img" "${BINARIES_DIR}/" || exit 1

	GENIMAGE_CFG="${BR2_EXTERNAL_RECALBOX_PATH}/board/recalbox/x86/genimage.cfg"
	;;

	*)
	echo "Outch. Unknown target ${RECALBOX_TARGET} (see copy-recalbox-archives.sh)" >&2
	bash
	exit 1
esac

# copy pre-upgrade
[[ -f ${BINARIES_DIR}/pre-upgrade.sh ]] && \
	cp "${BINARIES_DIR}/pre-upgrade.sh" "${BINARIES_DIR}/boot-data/pre-upgrade.sh"

#
# AT THIS STAGE - NO NEW FILES MAY BE COPIED!
#

# generate boot.md5
echo "Generating boot.md5"
compute_md5 "${BINARIES_DIR}/boot-data" > "${BINARIES_DIR}/boot-data/boot.md5" ||
	{ echo "ERROR: unable to compute md5" && exit 1; }

# generate boot.lst
echo "Generating boot.lst"
generate_boot_file_list "${BINARIES_DIR}/boot-data/" >"${BINARIES_DIR}/boot-data/boot.lst"

# generate recalbox.tar.xz (formerly boot.tar.xz) — runs in background so it
# overlaps with genimage below (genimage is mostly I/O-bound, tar.xz is
# CPU-bound, the two don't fight for the same resource). We wait on it
# before the CPU-heavy .img.xz compression so they don't both fight for
# cores.
echo "Generating tar (in background)"
generate_recalbox_tar_xz "${BINARIES_DIR}/boot-data/" "${RECALBOX_BINARIES_DIR}/recalbox-${RECALBOX_TARGET_LOWER}.tar.xz" &
TAR_XZ_PID=$!

# prepare genimage.cfg
echo "Preparing genimage.cfg"
[[ -z "${GENIMAGE_CFG}" ]] &&
	{ kill "${TAR_XZ_PID}" 2>/dev/null; echo "ERROR: GENIMAGE_CFG is empty, check board customization" && exit 1; }
prepare_genimage_cfg "${GENIMAGE_CFG}" ||
	{ kill "${TAR_XZ_PID}" 2>/dev/null; echo "ERROR : unable to prepare genimage.cfg" && exit 1 ; }

# generate recalbox.img (runs in parallel with the tar.xz above)
echo "Generating image"
generate_recalbox_img "${TARGET_DIR}" "${BINARIES_DIR}" "${RECALBOX_BINARIES_DIR}" "${BUILD_DIR}/genimage.tmp" ||
	{ kill "${TAR_XZ_PID}" 2>/dev/null; echo "ERROR : unable to create recalbox.img" && exit 1 ; }

# wait for the tar.xz to finish before attacking img.xz (both are CPU-bound)
wait "${TAR_XZ_PID}" ||
	{ echo "ERROR : unable to create recalbox.tar.xz" && exit 1 ; }

# Compress the generated .img
echo "Compressing image"
if mv -f "${RECALBOX_BINARIES_DIR}/recalbox.img" "${RECALBOX_IMG}" ; then
	echo "Compressing ${RECALBOX_IMG} ... "
	xz -9 -e --threads=0 "${RECALBOX_IMG}" || exit 1
else
	echo "ERROR: Couldn't move recalbox.img or compress it"
	exit 1
fi

# Computing hash sums to make have an update that can be dropped on a running Recalbox
echo "Computing sha1 sums ..."
(cd "${RECALBOX_BINARIES_DIR}" && for file in * ; do sha1sum "${file}" > "${file}.sha1"; done)
