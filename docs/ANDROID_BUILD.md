# AetherDuel Android diagnostic build

## Purpose

This is a lightweight Android build for rapid source/UI iteration. It is not a complete playable EDOPro distribution.

- Uses the AetherDuel repository as the native EDOPro source.
- Checks out the Android wrapper at pinned commit `36517642ce8151d9e6e5fb1acb801dfa7a8ef1f5`.
- Builds ARM64 only.
- Disables WindBot in the generated wrapper checkout; it does not delete WindBot source from AetherDuel.
- Packages a minimal runtime set from pinned ProjectIgnis Distribution commit `54a6e2395c532648ff762540e9615319fac4f51b`: `config/strings.conf`, `config/configs.json`, `config/system.conf`, and `fonts/NotoSansJP-Regular.otf`. It generates the Android asset-copy manifests so these files are actually copied into the selected EDOPro folder. It does not download the full distribution or run `ci/download-assets.sh` / `ci/touch-assets.sh`.
- Builds a debug APK with `assembleDebug`.
- Keeps the generated wrapper and build output under `.build/android-wrapper/`, which is ignored by Git.

Because the full ProjectIgnis distribution is intentionally omitted, this APK is for startup/build diagnostics and UI iteration. It has the strings and base font needed to get past initial runtime setup, but it is not expected to support a complete duel or normal card/database functionality.

## Automatic build

The GitHub Actions workflow runs on every push, pull request, or manual dispatch. After a successful run, download the APK from the run's **Artifacts** section. The artifact name includes the source commit SHA.

The workflow installs Java 17, Android SDK platform 36, and Android NDK r21d. Gradle dependencies and native compiler outputs are cached between runs to reduce repeated setup and compilation time.

## Local build

A local build requires:

- Java 17
- Android SDK with platform 36 and build-tools 36.0.0
- Android NDK 21.4.7075529
- `ndk-build`, `git`, and `python3` on `PATH`

Set `ANDROID_SDK_ROOT` (or `ANDROID_HOME`) to the SDK directory, then run:

```bash
bash scripts/build-android.sh
```

The script downloads the pinned Android wrapper once into `.build/android-wrapper/`, reconnects its `deps/edopro` path to the current AetherDuel checkout, applies diagnostic-only WindBot/ABI settings to that generated wrapper checkout, and builds the APK.

Do not edit files inside `.build/android-wrapper/` as permanent project changes. Put source changes in the AetherDuel repository; the wrapper is regenerated from the pinned upstream commit.
