# diagnose-failure.sh — rationale

## Naming the right package

The previous heuristic took the last `>>> <pkg> <ver> <step>` banner in
`build.log`. Under `make -j`, that line belongs to whichever package printed
last, not to the one that broke. On three real failures it accused `wsd`,
`advancemame` and `..`.

The make error line does not lie:

```
make[1]: *** [package/pkg-generic.mk:266: /work/output/build/ffmpeg-6.1.2/.stamp_configured] Error 1
```

The script extracts `ffmpeg-6.1.2`, then `ffmpeg` — build directories are
`<pkg>-<version>` and a version never contains a dash. The banner heuristic
stays as a fallback for failures outside a package rule.

Checked against the two real logs of 2026-08-02: `advancemame` → **ffmpeg** (the
actual culprit), and `wsd` → **wsd** (the old heuristic happened to be right).

## Keeping the state

`.cleanup-output` wipes `output/` in `after_script`, including on failure. That
is why the ffmpeg/x265 failure of 2026-08-02 could not be explained: nothing was
left to look at. The script therefore archives the failing package's
`config.log`, `config.status`, `meson-log.txt`, CMake logs and `.stamp_*` files
into `failed-<pkg>-state.tar.gz` before the cleanup runs.

## What it does not do

It never decides the job's exit code: it always returns 0 and the caller runs
`exit "$rc"`. A failing `script:` step stops GitLab from running the following
ones, so the diagnosis has to live in the same step as the failure — which is
why this is a called script rather than a `!reference` anchor.
