#!/bin/sh
# Names the package that broke the build and keeps its state. Always exits 0.
set -u

LOG="${1:-build.log}"
[ -f "${LOG}" ] || { echo ">>> No ${LOG} to diagnose"; exit 0; }

FAILED_DIR=$(grep -aoE '/work/output/build/[^/]+/\.stamp_[a-z_]+' "${LOG}" 2>/dev/null \
             | tail -1 | sed -E 's#.*/build/([^/]+)/\.stamp_.*#\1#')
if [ -n "${FAILED_DIR}" ]; then
    FAILED_PKG=$(echo "${FAILED_DIR}" | sed -E 's/-[^-]+$//')
else
    FAILED_PKG=$(grep -aE '>>> [^ ]+ ' "${LOG}" 2>/dev/null | tail -1 | awk '{print $2}')
fi

echo ">>> Build FAILED — culprit: ${FAILED_PKG:-unknown}"

if [ -n "${FAILED_DIR}" ] && [ -d "${OUTPUTDIR:-}/build/${FAILED_DIR}" ]; then
    find "${OUTPUTDIR}/build/${FAILED_DIR}" -maxdepth 4 -type f -size -10M \
         \( -name 'config.log' -o -name 'config.status' -o -name 'meson-log.txt' \
            -o -name 'CMakeOutput.log' -o -name 'CMakeError.log' -o -name '.stamp_*' \) \
         > /tmp/failed-state.lst 2>/dev/null
    if [ -s /tmp/failed-state.lst ]; then
        tar -czf "failed-${FAILED_PKG}-state.tar.gz" --files-from=/tmp/failed-state.lst 2>/dev/null \
            && echo ">>> Kept $(wc -l < /tmp/failed-state.lst) diagnostic files in failed-${FAILED_PKG}-state.tar.gz"
    fi
fi

START=$(grep -naF ">>> ${FAILED_PKG} " "${LOG}" 2>/dev/null | head -1 | cut -d: -f1)
if [ -n "${START}" ]; then
    sed -n "${START},\$p" "${LOG}" > "failed-${FAILED_PKG}.log"
    echo "=== Last 200 lines of '${FAILED_PKG}' (full log in the failed-${FAILED_PKG}.log artifact) ==="
    tail -200 "failed-${FAILED_PKG}.log"
else
    echo "=== Could not isolate the package; last 200 lines of ${LOG} ==="
    tail -200 "${LOG}"
fi

exit 0
