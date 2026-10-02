#!/bin/bash
# Usage
# ARCH="rpi5_64" scripts/linux/recaldocker.sh
# ARCH="rpi5_64" DL="$PWD/../build/dl" HOST="$PWD/../build/host" scripts/linux/recaldocker.sh
# ARCH="rpi5_64" DL="$PWD/../build/dl" HOST="$PWD/../build/host" scripts/linux/recaldocker.sh make menuconfig
# ARCH="rpi5_64" DL="$PWD/../build/dl" HOST="$PWD/../build/host" scripts/linux/recaldocker.sh make recalbox-emulationstation2
# ARCH="rpi5_64" DL="$PWD/../build/dl" HOST="$PWD/../build/host" scripts/linux/recaldocker.sh make recalbox-emulationstation2-reconfigure
# ARCH="rpi5_64" DL="$PWD/../build/dl" HOST="$PWD/../build/host" scripts/linux/recaldocker.sh make recalbox-rpi5_64_defconfig

# Quick'n'dirty checks
if [[ -z "$ARCH" ]] ; then
  inferred_arch=$(basename $(pwd) | sed -nr 's/^recalbox-(.+)/\1/p')
  if [[ -f configs/recalbox-${inferred_arch}_defconfig ]]; then
    ARCH=${inferred_arch}
  else
    echo "You must set the ARCH variable (we could not infer it from directory name)." >&2
    exit 1
  fi
fi
DL="${DL:-$PWD/dl}"
HOST="${HOST:-$PWD/host}"

docker build -t "recalbox-dev" .

NPM_PREFIX_OUTPUT_PATH="`pwd`/output/build/.npm"
mkdir -p "$NPM_PREFIX_OUTPUT_PATH" "${DL}" "${HOST}"

docker run -ti --rm --security-opt seccomp=unconfined \
	-w="$(pwd)" \
	-v "$(pwd):$(pwd)" \
	-v "$NPM_PREFIX_OUTPUT_PATH:/.npm" \
	-v "${DL}:/share/dl" \
	-v "${HOST}:/share/host" \
	-e "ARCH=${ARCH}" \
	-e "PACKAGE=${PACKAGE}" \
	-e "RECALBOX_VERSION=${RECALBOX_VERSION:-development}" \
	-e "GITLAB_TOKEN_THEMES=${GITLAB_TOKEN_THEMES}" \
	-e "GITLAB_TOKEN_BEEBEM=${GITLAB_TOKEN_BEEBEM}" \
	-e "GITLAB_TOKEN_RECALBOX_CORES=${GITLAB_TOKEN_RECALBOX_CORES}" \
	-e "FORCE_UNSAFE_CONFIGURE=1" \
	--user="`id -u`:`id -g`" \
	"recalbox-dev" ${@}
