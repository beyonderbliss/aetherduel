# Android SDK 36 — Full-Width Landscape + Camera Cutout Safe Area

**Status: VERIFIED ON DEVICE — STABLE CHECKPOINT**

## Checkpoint

- Stable branch: `stable/sdk-36-cutout-safe-area-full-width`
- Source branch: `experiment/sdk-36-right-edge-optimization`
- Source commit: `356918a0cc11e174582a2b752acae2662f362224`
- Android build workflow run: [#156](https://github.com/beyonderbliss/aetherduel/actions/runs/41630422771)
- Workflow result: **success**
- Artifact: `aetherduel-android-arm64-debug` (artifact ID `13228252077`)
- Artifact SHA-256: `d6e9e2ad32a711c598ae67618dc84447fc431466255aa09265e60da3d4bc3d36`
- Previous rollback/reference branch: `baseline/sdk-36-cutout-safe-area`
- `master` was not modified.

## Verified device result

The user tested the APK produced by the source commit above and confirmed:

- Landscape fullscreen is active.
- The left camera cutout is protected by the safe area with precise alignment.
- The content extends across the full available width on the right, reaching the curved screen edge.
- The former black right-side margin is gone.
- No image distortion or unwanted cropping was observed.

This is the accepted behavior and the reference result for future Android window/layout changes.

## Root cause

The generated Android manifest inherited this legacy metadata entry from the pinned Android wrapper:

```xml
<meta-data
    android:name="android.max_aspect"
    android:value="2.1" />
```

The device landscape display measured 2712 × 1220 pixels. The 2.1 aspect-ratio cap limited the app's effective width to:

```
1220 × 2.1 = 2562 px
2712 - 2562 = 150 px unused
```

That restriction—not the SDL renderer's cutout handling—caused the unused horizontal display area and visible black margin. Earlier attempts to adjust DecorView padding or cutout mode did not solve that underlying width cap.

## Fix

The Android build workflow now removes the legacy `android.max_aspect=2.1` metadata from the generated wrapper manifest before building the APK. It also asserts that no `android.max_aspect` entry remains.

The successful workflow commit uses this exact-match pattern:

```python
pattern = r'\s*<meta-data\s+android:name="android\.max_aspect"\s+android:value="2\.1"\s*/>'
updated, count = re.subn(pattern, "", text, count=1)
assert count == 1, "Expected exactly one legacy android.max_aspect=2.1 entry"
assert "android.max_aspect" not in updated, "Legacy aspect-ratio cap remains"
manifest.write_text(updated)
```

The target SDK remains **36**. The solution does not rely on downgrading the app to SDK 28.

## Rollback and change control

- Use `stable/sdk-36-cutout-safe-area-full-width` as the known-good full-width + cutout-safe checkpoint.
- Keep `baseline/sdk-36-cutout-safe-area` unchanged as the earlier SDK 36 rollback/reference point (cutout-safe, but with the former right-side margin).
- Do not move or merge either checkpoint into `master` without explicit approval.
- Do not reintroduce the legacy aspect-ratio cap.
- Treat changes to fullscreen flags, cutout policy, native window sizing, SDL surface geometry, or renderer scaling as separate changes. Re-test both the left cutout and right curved edge on-device before replacing this checkpoint.

## Validation provenance

The workflow run completed successfully for commit `356918a0cc11e174582a2b752acae2662f362224`, and its ARM64 debug APK is the artifact associated with that exact commit. Final visual/behavioral acceptance was supplied by the user's on-device test.
