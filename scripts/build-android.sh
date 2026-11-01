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
download_asset() {
  local source_path="$1"
  local destination="$2"
  curl --fail --silent --show-error --location --retry 5 --connect-timeout 30 \
    "$DISTRIBUTION_RAW/$source_path" -o "$destination"
  [[ -s "$destination" ]] || fail "Required runtime asset is missing or empty: $source_path"
}

download_asset "config/strings.conf" "$DEFAULTS/config/strings.conf"
download_asset "config/configs.json" "$DEFAULTS/config/configs.json"
download_asset "config/system.conf" "$DEFAULTS/config/system.conf"
download_asset "fonts/NotoSansJP-Regular.otf" "$DEFAULTS/fonts/NotoSansJP-Regular.otf"
download_asset "fonts/NotoSansJP-Regular-OFL.txt" "$DEFAULTS/fonts/NotoSansJP-Regular-OFL.txt"

# index.txt lists directories; filelist.txt lists directories and files in copy order.
printf 'config\nfonts\n' > "$ASSETS/index.txt"
printf 'config\nconfig/strings.conf\nconfig/configs.json\nconfig/system.conf\nfonts\nfonts/NotoSansJP-Regular.otf\nfonts/NotoSansJP-Regular-OFL.txt\n' > "$ASSETS/filelist.txt"
: > "$ASSETS/indexu.txt"
: > "$ASSETS/filelistu.txt"

python3 - "$WRAPPER_DIR" "$APPLICATION_ID" "$BUILD_VERSION_CODE" <<'PY'
from pathlib import Path
import re
import sys

root = Path(sys.argv[1])
application_id = sys.argv[2]
version_code = sys.argv[3]

gradle = root / "build.gradle"
text = gradle.read_text()
dependency = '\timplementation(files("libs/libWindbot.aar"))\n'
assert text.count(dependency) == 1, "Expected exactly one WindBot AAR dependency"
text = text.replace(dependency, "")
text, count = re.subn(
    r"abiFilters 'armeabi-v7a', 'arm64-v8a', 'x86'.*",
    "abiFilters 'arm64-v8a'",
    text,
    count=1,
)
assert count == 1, "Could not restrict Gradle ABI filters to ARM64"
text, count = re.subn(
    r'applicationId "io\.github\.edo9300\.edopro"',
    f'applicationId "{application_id}"',
    text,
    count=1,
)
assert count == 1, "Could not set the AetherDuel application ID"
text, count = re.subn(r"versionCode \d+", f"versionCode {version_code}", text, count=1)
assert count == 1, "Could not set the per-build asset revision"
gradle.write_text(text)

main = root / "src/main/java/io.github.edo9300.edopro/MainActivity.java"
text = main.read_text()
text, count = re.subn(r"^import libwindbot\.windbot\.WindBot;\n", "", text, count=1, flags=re.M)
assert count == 1, "Could not remove WindBot import from MainActivity"
old = """\t\tvar use_windbot = true;
\t\tif (Build.VERSION.SDK_INT <= Build.VERSION_CODES.ICE_CREAM_SANDWICH_MR1) {
\t\t\tuse_windbot = false;
\t\t} else {
\t\t\ttry {
\t\t\t\t/*
\t\t\t\t * windbot loading might fail, for whatever reason,
\t\t\t\t * disable it if that's the case
\t\t\t\t */
\t\t\t\tWindBot.initAndroid(working_directory + "/WindBot");
\t\t\t} catch (Exception e) {
\t\t\t\tuse_windbot = false;
\t\t\t}
\t\t}
"""
assert text.count(old) == 1, "Could not uniquely locate WindBot initialization"
text = text.replace(old, "\t\tvar use_windbot = false;\n")
assert "WindBot" not in text, "Unexpected WindBot reference remains in MainActivity"
main.write_text(text)

native = root / "src/main/java/io.github.edo9300.edopro/EpNativeActivity.java"
text = native.read_text()
text, count = re.subn(r"^import libwindbot\.windbot\.WindBot;\n", "", text, count=1, flags=re.M)
assert count == 1, "Could not remove WindBot import from EpNativeActivity"
for action, method in [
    ("RUN_WINDBOT", "runAndroid"),
    ("ATTACH_WINDBOT_DATABASE", "addDatabase"),
]:
    pattern = (
        r'\n\t\t\t\tcase "' + action + r'": \{.*?'
        + re.escape("WindBot." + method) + r'\(args\);'
        + r'.*?\n\t\t\t\t\}'
    )
    text, count = re.subn(pattern, "", text, count=1, flags=re.S)
    assert count == 1, "Could not remove " + action + " receiver"
assert "WindBot." not in text, "WindBot runtime call remains in EpNativeActivity"
native.write_text(text)

app_mk = root / "jni/Application.mk"
text = app_mk.read_text()
text, count = re.subn(r"^APP_ABI :=.*$", "APP_ABI := arm64-v8a", text, count=1, flags=re.M)
assert count == 1, "Could not restrict native ABI to ARM64"
text, count = re.subn(r"^APP_OPTIM :=.*$", "APP_OPTIM := debug", text, count=1, flags=re.M)
assert count == 1, "Could not set native debug optimization"
app_mk.write_text(text)
PY

if grep -R -nE 'libwindbot|WindBot\.(initAndroid|runAndroid|addDatabase)' \
  "$WRAPPER_DIR/build.gradle" "$WRAPPER_DIR/src/main/java"; then
  fail "WindBot references remain after diagnostic isolation"
fi

printf 'sdk.dir=%s\nndk.dir=%s/ndk/%s\n' "$SDK_ROOT" "$SDK_ROOT" "$NDK_VERSION" > "$WRAPPER_DIR/local.properties"

printf '\n== AetherDuel Android build configuration ==\n'
printf 'Source: %s\nWrapper: %s (%s)\nABI: arm64-v8a\nWindBot: disabled for this diagnostic build\nFull distribution assets: skipped\n' "$ROOT_DIR" "$WRAPPER_REPO" "$WRAPPER_REF"

export PATH="$SDK_ROOT/ndk/$NDK_VERSION:$PATH"
ndk-build -C "$WRAPPER_DIR" -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
(
  cd "$WRAPPER_DIR"
  ./gradlew --no-daemon --stacktrace assembleDebug
)

printf '\nAPK output:\n%s\n' "$WRAPPER_DIR/build/outputs/apk/debug/"
