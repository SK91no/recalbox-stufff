# Supermodel performance tuning (Raspberry Pi 5 / aarch64)

Build-side optimizations layered on stock Supermodel to speed up Sega Model 3 emulation
on the Pi5. Measured ~+16-20% at faithful PowerPC clock (`ppc-frequency=100`): Scud Race
and Daytona 2 hold 60 fps at arcade-accurate speed (baseline was ~54 fps).

## What is applied (`supermodel.mk`)

- **LTO** (`-flto=auto -fuse-linker-plugin`) + tuned flags (`SUPERMODEL_EXTRA_FLAGS`:
  `-funroll-loops -fno-semantic-interposition -fipa-pta`).
- **PGO** (profile-guided optimization): profile stored in `pgo/*.gcda`, applied with
  `-fprofile-use`. `-Wno-error=coverage-mismatch` makes a stale profile degrade to
  no-PGO on the affected files instead of breaking the build.
- **Patch 0005**: legacy3d decodes/uploads textures as RGBA8 (`GL_UNSIGNED_BYTE`) instead
  of `GL_FLOAT`. The texture sheet storage is already `GL_RGBA8`, so this is 4x less upload
  bandwidth with no driver conversion, and lossless (sources are <=8-bit).
- **Patch 0006**: opt-in benchmark/profiling harness (see below). Zero overhead when unused.

A/B measured (Daytona 2, dense scene): PowerPC time -14 to -18%.

## Regenerating the PGO profile (do this after bumping `SUPERMODEL_VERSION`)

    ARCH=rpi5_64 ./package/supermodel/pgo-regen.sh

Builds an instrumented binary, runs each game's **attract mode** on the Pi5, accumulates
the profile, stores it in `pgo/`, and rebuilds. It deploys Recalbox's factory NVRAM from
`/recalbox/share_upgrade/saves/model3/NVRAM` so networked games (Daytona 2, Scud, ...) boot
standalone with 0 credits and self-run their 3D demo. No save states needed.
Env: `PI_IP`, `PI_PW`, `GAMES`, `FRAMES`.

## Benchmark harness (patch 0006, opt-in)

- `-bench=N` — run N frames (throttle/vsync forced off) then quit; prints per-frame stats
  (mean/median/p99 ms, fps, internal ppc/render/sync/snd breakdown) and a process CPU-time probe.
- `-bench-warmup=N`, `-bench-out=file.csv`, `-bench-drive` (hold full throttle for driving games).
- `-show-fps` also writes one line/second to `/tmp/supermodel-fps.log`.

Must run through `dwl` on the Pi5: Model 3 needs a desktop-GL context, which V3D exposes
only via the Wayland EGL platform (this is why games launch under a dwl compositor).

## Bottleneck analysis (reference)

- **CPU-bound on the PowerPC.** The 603 core (`Src/CPU/PowerPC/ppc603.c`) is a pure
  interpreter (no dynarec); ~15 ms/frame at `ppc=100` — the dominant cost.
- **Rendering (legacy3d) is CPU-bound too**: scene-graph traversal + per-frame reassembly
  of dynamic PolyRAM models (~10-16 ms in dense scenes). GPU fill is only ~2 ms at 640x480.

## Explored and rejected

- **New3D renderer**: runs on the Pi5 (via Mesa version override) but ~2.7x slower than
  legacy3d on V3D.
- **Direct KMS**: not possible without a GLES port — Supermodel needs desktop GL, which V3D
  provides only through the Wayland EGL platform (hence dwl).
- **Dynamic model cache (content hash)**: no gain — PolyRAM models are regenerated every
  frame, so the hash always misses (plus VBO-flush frame spikes).
- **Vertex format compression, per-model `glGetIntegerv` removal**: negligible.

## Bigger levers (future, dedicated projects)

- **PowerPC dynarec** (e.g. MAME `drcuml` + aarch64 backend): ~2-5x CPU. Largest remaining win.
- **GLES renderer port**: more efficient, and would enable direct KMS.
