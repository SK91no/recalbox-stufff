#!/usr/bin/env bash
#
# Write "<package> <key>" lines to ${OUTPUTDIR}/.pkg-cache-keys, one per
# package of $PKG_CACHE_LIST that this arch actually selects.
#
# The key covers the whole dependency closure (see pkg-cache-key.py), which is
# what a defconfig grep cannot see: an option a package only receives because
# another one selected it. Two projects sharing a runner then agreed on a key
# while disagreeing on the artefact, and the archive of one landed in the other.
#
# Everything is computed inside the build image: the runner is Alpine and has
# no python, and only the resolved .config — not the defconfig — carries those
# selects. The buildroot SHA is read on the host, where git owns the checkout.
#
# Failing here is never fatal: the key file is left empty, every package misses
# its cache and gets rebuilt. Slow, never wrong.
#
# Usage: OUTPUTDIR=... WORKDIR=... ARCH=... PKG_CACHE_LIST=... \
#          scripts/ci/compute-pkg-cache-keys.sh <build image>

set -uo pipefail

IMAGE="${1:?usage: compute-pkg-cache-keys.sh <build image>}"
: "${WORKDIR:?the checkout to mount}" "${OUTPUTDIR:?where the keys are written}"
: "${ARCH:?the arch whose defconfig is resolved}" "${PKG_CACHE_LIST:?the packages to key}"
DOCKER="${DOCKER:-docker}"

KEYS="${OUTPUTDIR}/.pkg-cache-keys"
LOG="${OUTPUTDIR}/.pkg-cache-keys.log"

mkdir -p "${OUTPUTDIR}"
: > "${KEYS}"

# git inside the container trips on "dubious ownership" of the mounted checkout.
BUILDROOT_SHA=$(git -C "${WORKDIR}" ls-tree HEAD buildroot | awk '{print $3}')
if [ -z "${BUILDROOT_SHA}" ]; then
  echo ">>> Package cache keys: no buildroot submodule SHA — every package will MISS"
  exit 0
fi

if ! "${DOCKER}" run --rm --tmpfs /tmp:exec --security-opt seccomp=unconfined \
    -v "${WORKDIR}:/work" -v "${OUTPUTDIR}:/work/output" \
    -e "ARCH=${ARCH}" -e "BUILDROOT_SHA=${BUILDROOT_SHA}" -e "PKG_CACHE_LIST=${PKG_CACHE_LIST}" \
    -e "FORCE_UNSAFE_CONFIGURE=1" \
    -e GIT_CONFIG_COUNT=1 -e GIT_CONFIG_KEY_0=safe.directory -e GIT_CONFIG_VALUE_0='*' \
    "${IMAGE}" \
    sh -c '( cd buildroot && git reset --hard HEAD && git clean -fdx ) >/dev/null 2>&1 \
        || echo ">>> buildroot could not be reset — mergeToBR refuses a dirty tree" \
      ; make "recalbox-${ARCH}_defconfig" \
      && make -s show-info 2>/dev/null > output/.pkg-graph.json \
      && python3 scripts/ci/pkg-cache-key.py \
           --graph output/.pkg-graph.json --config output/.config \
           --buildroot-sha "${BUILDROOT_SHA}" --packages "${PKG_CACHE_LIST}" --tree . \
           > output/.pkg-cache-keys' > "${LOG}" 2>&1; then
  echo ">>> Package cache keys FAILED — every package will MISS. Last lines:"
  tail -20 "${LOG}"
  : > "${KEYS}"
  exit 0
fi

echo ">>> Package cache keys computed for $(wc -l < "${KEYS}") selected packages"
