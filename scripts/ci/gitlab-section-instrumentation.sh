#!/bin/sh
# Buildroot instrumentation script — wrap each (package, step) into a GitLab CI
# collapsible section. When a build fails, the broken step's section is never
# closed, so GitLab leaves exactly that section expanded in the job view —
# you see what broke without scrolling through a 100k-line build.log.
#
# Buildroot calls this (via BR2_INSTRUMENTATION_SCRIPTS, see
# package/pkg-generic.mk:step_user) with:
#   $1 = "start" | "end"
#   $2 = step name (extract, patch, configure, build, install-target, ...)
#   $3 = package name
#
# Enable in CI by passing BR2_INSTRUMENTATION_SCRIPTS=<path-to-this-script>
# to `make`. Outside GitLab (no CI env), it stays mostly quiet.
#
# Sequential top-level make only: under -jN the sections interleave and GitLab hides the log.

phase="$1"
step="$2"
pkg="$3"

# Wrap every meaningful step. patch/extract emit a lot of "Applying ..."
# lines, so folding them away is exactly the point. Skip the fixup/internal
# sub-steps that have no useful output.
case "$step" in
  download|extract|patch|configure|build|install-staging|install-target|install-host|install-images) ;;
  *) exit 0 ;;
esac

# GitLab section ids must match [a-zA-Z0-9_]. Map everything else to '_'.
sid="pkg_$(printf '%s_%s' "$pkg" "$step" | tr -c 'a-zA-Z0-9_' '_')"
ts="$(date +%s)"
esc="$(printf '\033')"   # portable ESC for /bin/sh printf

case "$phase" in
  start)
    printf '%s[0Ksection_start:%s:%s[collapsed=true]\r%s[0K>>> %s :: %s\n' \
           "$esc" "$ts" "$sid" "$esc" "$pkg" "$step"
    ;;
  end)
    printf '%s[0Ksection_end:%s:%s\r%s[0K\n' \
           "$esc" "$ts" "$sid" "$esc"
    ;;
esac
exit 0
