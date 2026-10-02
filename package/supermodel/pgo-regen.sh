#!/usr/bin/env bash
#
# pgo-regen.sh - Regenerate Supermodel's PGO profile after a version bump.
#
# When SUPERMODEL_VERSION changes in supermodel.mk, the stored profile in pgo/*.gcda goes
# partially stale (the build does NOT break -- -Wno-error=coverage-mismatch degrades the
# changed functions to no-PGO -- but the PGO gain is lost on them). This script re-records a
# complete profile by running each game in ATTRACT MODE (no save states needed):
#   1. instrumented build (-fprofile-generate, via SUPERMODEL_PGO_MODE=generate)
#   2. deploy Recalbox's factory NVRAM (share_upgrade: standalone, 0 credit -> games self-run
#      in attract, no "network board" error) and run each game ~long on the Pi5
#   3. pull the accumulated profile back into pgo/
#   4. production rebuild (-fprofile-use)
#
# Missing games are skipped. The user's own NVRAM is backed up to NVRAM.userbak and restored.
#
# Usage:   ARCH=rpi5_64 ./package/supermodel/pgo-regen.sh
# Env:     PI_IP (def 192.168.1.60)  PI_PW (def recalboxroot)  ARCH (def rpi5_64)
#          FRAMES (attract frames per game, def 3000)  GAMES ("rom1 rom2 ...")
set -euo pipefail

ARCH="${ARCH:-rpi5_64}"
PI_IP="${PI_IP:-192.168.1.60}"
PI_PW="${PI_PW:-recalboxroot}"
FRAMES="${FRAMES:-3000}"
GAMES="${GAMES:-daytona2 scud vf3 von2 fvipers2 spikeout lostwsga oceanhun srally2 lamachin harley skichamp swtrilgy dirtdvls getbass eca magtruck bassdx vs2 vs299}"

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
PKG_PGO="$REPO/package/supermodel/pgo"
DOCKER="$REPO/scripts/linux/recaldocker-notty.sh"
SSH_OPTS="-o StrictHostKeyChecking=no -o PreferredAuthentications=password -o PubkeyAuthentication=no -o ConnectTimeout=8"
export SSHPASS="$PI_PW"
RSH(){ sshpass -e ssh $SSH_OPTS "root@$PI_IP" "$@"; }
RCP(){ sshpass -e scp $SSH_OPTS "$@"; }

command -v sshpass >/dev/null || { echo "ERROR: sshpass is required"; exit 1; }
RSH 'echo ok' >/dev/null 2>&1 || { echo "ERROR: Pi5 $PI_IP unreachable"; exit 1; }

echo "==> [1/4] Instrumented build (-fprofile-generate)"
cd "$REPO"
ARCH="$ARCH" "$DOCKER" make SUPERMODEL_PGO_MODE=generate supermodel-dirclean >/dev/null 2>&1 || true
ARCH="$ARCH" "$DOCKER" make SUPERMODEL_PGO_MODE=generate supermodel 2>&1 | grep -iE "error:|Linking Super" | tail -3
BIN="$(ls "$REPO"/output/build/supermodel-*/bin/supermodel)"
[ -f "$BIN" ] || { echo "ERROR: instrumented binary not found"; exit 1; }

echo "==> [2/4] Profiling on the Pi5 (attract mode, factory NVRAM): $GAMES"
RCP "$BIN" "root@$PI_IP:/tmp/supermodel-bench" >/dev/null 2>&1
RUN_REMOTE="$(cat <<REMOTE
chmod +x /tmp/supermodel-bench; rm -rf /tmp/pgo; mkdir -p /tmp/pgo
NV=/recalbox/share/saves/model3/NVRAM; VAN=/recalbox/share_upgrade/saves/model3/NVRAM
# Back up user's NVRAM and deploy factory NVRAM (standalone, 0 credit -> attract)
[ -d "\$NV" ] && { rm -rf "\$NV.userbak"; cp -r "\$NV" "\$NV.userbak"; }
mkdir -p "\$NV"; cp -f "\$VAN"/*.nv "\$NV"/ 2>/dev/null
/etc/init.d/S31emulationstation stop >/dev/null 2>&1; sleep 2
pkill -x supermodel 2>/dev/null; pkill -x dwl 2>/dev/null; sleep 1
export XDG_RUNTIME_DIR=/run/user/0 HOME=/recalbox/share/system
export MESA_GLSL_VERSION_OVERRIDE=450 MESA_GL_VERSION_OVERRIDE=4.5
export GCOV_PREFIX=/tmp/pgo GCOV_PREFIX_STRIP=0
C="-fullscreen -res=640,480 -ss=1 -no-wide-screen -upscalemode=0 -no-throttle -no-vsync -crosshairs=0 -no-multi-texture -legacy3d -gpu-multi-threaded -sound -dsb -legacy-scsp -ppc-frequency=100"
for g in $GAMES; do
  rom=/recalbox/share/roms/model3/\$g.zip
  if [ ! -f "\$rom" ]; then echo "  skip (missing): \$g"; continue; fi
  # fresh factory NVRAM for this game each run so it boots to attract (no leftover credits)
  cp -f "\$VAN/\$g.nv" "\$NV/\$g.nv" 2>/dev/null || true
  echo "  profiling \$g (attract, $FRAMES frames)..."
  timeout 400 /usr/bin/dwl -s "/tmp/supermodel-bench \$rom \$C -bench=$FRAMES -bench-warmup=0 -bench-out=/dev/null" >/dev/null 2>&1 || echo "    (timeout/partial: \$g)"
done
# restore user's NVRAM
[ -d "\$NV.userbak" ] && { rm -f "\$NV"/*.nv; cp -f "\$NV.userbak"/*.nv "\$NV"/ 2>/dev/null; }
/etc/init.d/S31emulationstation start >/dev/null 2>&1
echo "  gcda: \$(find /tmp/pgo -name '*.gcda' | wc -l)"
cd /tmp/pgo && tar czf /tmp/pgo.tgz .
REMOTE
)"
RSH "$RUN_REMOTE"

echo "==> [3/4] Pulling profile back into package/supermodel/pgo/"
TMP="$(mktemp -d)"
RCP "root@$PI_IP:/tmp/pgo.tgz" "$TMP/pgo.tgz" >/dev/null 2>&1
tar xzf "$TMP/pgo.tgz" -C "$TMP"
SRC="$(dirname "$(find "$TMP" -name 'ppc.gcda' | head -1)")"
[ -d "$SRC" ] || { echo "ERROR: no .gcda pulled back"; rm -rf "$TMP"; exit 1; }
rm -f "$PKG_PGO"/*.gcda
cp "$SRC"/*.gcda "$PKG_PGO"/
rm -f "$PKG_PGO"/Main.gcda   # Main is not profiled (recompiled without profile, no impact)
echo "  stored: $(ls "$PKG_PGO"/*.gcda | wc -l) profile files"
rm -rf "$TMP"

echo "==> [4/4] Production rebuild (-fprofile-use)"
ARCH="$ARCH" "$DOCKER" make SUPERMODEL_PGO_MODE=use supermodel-dirclean >/dev/null 2>&1 || true
ARCH="$ARCH" "$DOCKER" make supermodel 2>&1 | grep -iE "error:|Linking Super" | tail -3

echo "==> Done. PGO profile regenerated and production binary rebuilt."
echo "    Remember to commit package/supermodel/pgo/*.gcda"
