#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${AETHERDUEL_BUILD_DIR:-$ROOT_DIR/.build}"
WRAPPER_DIR="${AETHERDUEL_ANDROID_WRAPPER_DIR:-$BUILD_DIR/android-wrapper}"
WRAPPER_REF="36517642ce8151d9e6e5fb1acb801dfa7a8ef1f5"
WRAPPER_REPO="https://github.com/edo9300/edopro-android.git"
NDK_VERSION="21.4.7075529"
APPLICATION_ID="io.github.beyonderbliss.aetherduel"
# Force the Android wrapper to treat each build as a new asset revision.
BUILD_VERSION_CODE="$(date +%s)"

fail() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
command -v git >/dev/null || fail "git is required"
command -v python3 >/dev/null || fail "python3 is required"
command -v curl >/dev/null || fail "curl is required to fetch pinned runtime assets"
command -v java >/dev/null || fail "Java is required"

SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-}}"
[[ -n "$SDK_ROOT" && -d "$SDK_ROOT" ]] || fail "Set ANDROID_SDK_ROOT (or ANDROID_HOME) to an installed Android SDK"
export ANDROID_SDK_ROOT="$SDK_ROOT"
export ANDROID_HOME="$SDK_ROOT"
export PATH="$SDK_ROOT/ndk/$NDK_VERSION:$PATH"
if command -v ccache >/dev/null; then export NDK_CCACHE=ccache; fi
command -v ndk-build >/dev/null || fail "ndk-build is missing; install Android NDK $NDK_VERSION under $SDK_ROOT/ndk"

mkdir -p "$BUILD_DIR"
if [[ ! -d "$WRAPPER_DIR/.git" ]]; then
  if [[ -e "$WRAPPER_DIR" ]]; then
    fail "$WRAPPER_DIR exists but is not the expected Git checkout; move it aside or set AETHERDUEL_ANDROID_WRAPPER_DIR"
  fi
  git clone --filter=blob:none --no-checkout "$WRAPPER_REPO" "$WRAPPER_DIR"
fi

git -C "$WRAPPER_DIR" remote set-url origin "$WRAPPER_REPO"
git -C "$WRAPPER_DIR" fetch --depth=1 origin "$WRAPPER_REF"
git -C "$WRAPPER_DIR" checkout --detach --force FETCH_HEAD
ACTUAL_REF="$(git -C "$WRAPPER_DIR" rev-parse HEAD)"
[[ "$ACTUAL_REF" == "$WRAPPER_REF" ]] || fail "Android wrapper pin mismatch: expected $WRAPPER_REF, got $ACTUAL_REF"

# The wrapper's upstream EDOPro submodule must never replace AetherDuel.
rm -rf "$WRAPPER_DIR/deps/edopro"
mkdir -p "$WRAPPER_DIR/deps"
ln -s "$ROOT_DIR" "$WRAPPER_DIR/deps/edopro"

# Package the smallest useful runtime set from a pinned ProjectIgnis distribution.
# AssetCopy requires populated index.txt/filelist.txt; empty lists silently copy nothing.
ASSETS="$WRAPPER_DIR/src/main/assets"
DEFAULTS="$ASSETS/defaults"
DISTRIBUTION_REF="54a6e2395c532648ff762540e9615319fac4f51b"
DISTRIBUTION_RAW="https://raw.githubusercontent.com/ProjectIgnis/Distribution/$DISTRIBUTION_REF"

mkdir -p "$DEFAULTS/config" "$DEFAULTS/fonts" "$ASSETS/update"

# Clone ProjectIgnis/Distribution secara shallow untuk mendapatkan seluruh aset textures, config, dan fonts secara utuh
DIST_TMP="$BUILD_DIR/Distribution"
if [[ ! -d "$DIST_TMP" ]]; then
  git clone --depth=1 https://github.com/ProjectIgnis/Distribution.git "$DIST_TMP"
else
  git -C "$DIST_TMP" pull origin master || true
fi

mkdir -p "$DEFAULTS/config" "$DEFAULTS/fonts" "$DEFAULTS/textures" "$ASSETS/update"

# Salin seluruh isi folder secara utuh ke defaults
cp -r "$DIST_TMP/config/." "$DEFAULTS/config/"
cp -r "$DIST_TMP/fonts/." "$DEFAULTS/fonts/"
cp -r "$DIST_TMP/textures/." "$DEFAULTS/textures/"

# Generate index.txt dan filelist.txt secara otomatis berdasarkan isi folder defaults
python3 - "$DEFAULTS" "$ASSETS" <<'PY'
from pathlib import Path
import sys

defaults_dir = Path(sys.argv[1])
assets_dir = Path(sys.argv[2])

dirs = set()
files = []

for p in defaults_dir.rglob("*"):
    if p.is_file():
        rel = p.relative_to(defaults_dir)
        for parent in rel.parents:
            if str(parent) != ".":
                dirs.add(parent.as_posix())
        files.append(rel.as_posix())

sorted_dirs = sorted(list(dirs))
sorted_files = sorted(files)

# Tulis ke index.txt (hanya direktori)
(assets_dir / "index.txt").write_text("\n".join(sorted_dirs) + "\n")

# Tulis ke filelist.txt (direktori lalu file-file di dalamnya)
filelist_items = sorted_dirs + sorted_files
(assets_dir / "filelist.txt").write_text("\n".join(filelist_items) + "\n")

print(f"Generated asset manifests: {len(sorted_dirs)} directories, {len(sorted_files)} files.")
PY
: > "$ASSETS/indexu.txt"
: > "$ASSETS/filelistu.txt"


cd "$WRAPPER_DIR" && ./gradlew --no-daemon --stacktrace assembleDebug
)

printf '\nAPK output:\n%s\n' "$WRAPPER_DIR/build/outputs/apk/debug/"
