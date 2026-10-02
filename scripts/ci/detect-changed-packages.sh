#!/usr/bin/env bash
#
# Print, one per line, the set of buildroot packages impacted by the current
# git diff against the MR target branch (or master when running outside a MR).
#
# Mapping rules:
#   package/<pkg>/...                          -> <pkg>
#   custom/package/<pkg>/...                   -> <pkg>
#   buildroot/package/<pkg>/...                -> <pkg>
#   projects/<sub>/...                         -> any package whose .mk references projects/<sub>
#   board/recalbox/**/kernel_patches/**        -> linux
#   board/recalbox/**/kernel-defconfig.config  -> linux
#
# Used by the package-validation stage in .gitlab-ci.yml.

set -euo pipefail

TARGET_BRANCH="${CI_MERGE_REQUEST_TARGET_BRANCH_NAME:-master}"
PROJECT_URL="${CI_MERGE_REQUEST_PROJECT_URL:-}"
IN_MR="${CI_MERGE_REQUEST_IID:-}"

# Diff base: GitLab's MR merge-base.
BASE="${CI_MERGE_REQUEST_DIFF_BASE_SHA:-}"

# Fetch the base commit if the shallow clone lacks it.
if [ -n "$BASE" ] && ! git cat-file -e "${BASE}^{commit}" 2>/dev/null; then
  git fetch -q --depth=2 origin "$BASE" 2>/dev/null \
    || git fetch -q --deepen=500 origin 2>/dev/null \
    || git fetch -q --unshallow origin 2>/dev/null || true
fi

# Fall back to the target branch tip when no MR base is available.
if [ -z "$BASE" ] || ! git cat-file -e "${BASE}^{commit}" 2>/dev/null; then
  if [ -n "$PROJECT_URL" ]; then
    git remote show target >/dev/null 2>&1 || git remote add target "$PROJECT_URL" >/dev/null 2>&1 || true
    git fetch -q target "$TARGET_BRANCH" 2>/dev/null || true
    BASE="target/${TARGET_BRANCH}"
  else
    BASE="origin/${TARGET_BRANCH}"
  fi
fi

# Fail in an MR if no base resolves; tolerate HEAD~1 elsewhere.
if ! git rev-parse --verify -q "${BASE}^{commit}" >/dev/null 2>&1; then
  if [ -n "$IN_MR" ]; then
    echo "ERROR: cannot resolve a diff base (CI_MERGE_REQUEST_DIFF_BASE_SHA / ${TARGET_BRANCH})." >&2
    exit 2
  fi
  echo "WARN: $BASE not found, falling back to HEAD~1" >&2
  BASE="HEAD~1"
fi

CHANGED=$(git diff --name-only "$BASE" HEAD)

# An empty diff in an MR means a broken base: fail instead of skipping.
if [ -z "$CHANGED" ] && [ -n "$IN_MR" ]; then
  echo "ERROR: empty diff against base '$BASE' in MR pipeline — refusing to skip validation silently." >&2
  exit 2
fi

PKGS=""

while IFS= read -r f; do
  [ -z "$f" ] && continue
  case "$f" in
    package/*/*)
      PKGS+=$'\n'$(echo "$f" | sed -E 's#^package/([^/]+)/.*#\1#')
      ;;
    custom/package/*/*)
      PKGS+=$'\n'$(echo "$f" | sed -E 's#^custom/package/([^/]+)/.*#\1#')
      ;;
    buildroot/package/*/*)
      PKGS+=$'\n'$(echo "$f" | sed -E 's#^buildroot/package/([^/]+)/.*#\1#')
      ;;
    projects/*/*)
      sub=$(echo "$f" | cut -d/ -f1-2)
      refs=$(find package/ -name '*.mk' -exec grep -l "$sub" {} + 2>/dev/null | sed -E 's#^package/([^/]+)/.*#\1#' | sort -u || true)
      if [ -n "$refs" ]; then
        PKGS+=$'\n'"$refs"
      fi
      ;;
  esac
  # Kernel sources live under board/recalbox/ at varying depths (common dir,
  # per-arch dir, per-board sub-arch). Any patch or defconfig touch must
  # rebuild linux so OOT modules link against fresh vmlinux/Module.symvers.
  if [[ "$f" == board/recalbox/* ]] && [[ "$f" == *kernel_patches/* || "$f" == *kernel-defconfig.config ]]; then
    PKGS+=$'\nlinux'
  fi
done <<< "$CHANGED"

echo "$PKGS" | sed '/^$/d' | sort -u
