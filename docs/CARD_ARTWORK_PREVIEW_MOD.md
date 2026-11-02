# Card Artwork Preview: Standard Mode and Fullscreen

Status: **initial visual goal achieved; layout still requires adjustment**

Branch at the time of the fix: `feat/card-view-original-clean`

## Goal

Show the card artwork in the standard card-information preview without cropping, while retaining the separate fullscreen artwork view.

The goal was not to change the source artwork or distort its aspect ratio. The standard preview's wrapper and child-image geometry needed to agree.

## Root cause identified

The standard preview had two conflicting geometry definitions:

- The generated layout specification used **zero wrapper padding** and an artwork origin of **(0, 0)**.
- The initialization code still created a larger wrapper using the old padding constants (10 px horizontal, 9 px vertical) and placed the child image at **(10, 9)**.

The resize path was already calculating the standard wrapper from the generated layout values. That meant the initial widget geometry did not match the geometry expected by the resize path.

This inconsistency was the confirmed target of the successful fix. The observed result after installing and testing the resulting APK: **standard artwork no longer cropped**. Fullscreen artwork continued to work.

## Minimal fix that worked

File: `gframe/game.cpp`, inside `Game::Initialize()`, in the `//img` initialization section.

Before:

```cpp
wCardImg = env->addStaticText(L"", Scale(1, 1, 1 + CARD_IMG_WRAPPER_WIDTH, 1 + CARD_IMG_WRAPPER_HEIGHT), true, false, 0, -1, true);
imgCard = AlignElementWithParent(env->addImage(Scale(10, 9, 10 + CARD_IMG_WIDTH, 9 + CARD_IMG_HEIGHT), wCardImg));
```

After:

```cpp
wCardImg = env->addStaticText(L"", Scale(1, 1, 1 + CARD_IMG_WIDTH, 1 + CARD_IMG_HEIGHT), true, false, 0, -1, true);
imgCard = AlignElementWithParent(env->addImage(Scale(0, 0, CARD_IMG_WIDTH, CARD_IMG_HEIGHT), wCardImg));
```

Fix commit: `265c4fa30616d6fe1e07e76e676a9a4314c6ab0d`

The patch changed only these two initialization expressions in `gframe/game.cpp`.

## Important: preserve these design decisions

1. Keep standard wrapper padding at **0 × 0** unless a future measured layout requirement demonstrates otherwise. Zero padding was intentional: the artwork should fit without an empty border/gap.
2. Keep the artwork child origin at **(0, 0)** so it matches the zero-padding wrapper.
3. Do not change the source card artwork dimensions or image aspect ratio to compensate for a container/layout mismatch.
4. Do not copy the entire layout implementation from another experimental branch just to fix standard preview. In particular, preserve the working fullscreen behavior.
5. Keep the generated layout specification and runtime initialization/resize geometry consistent. If the layout spec changes, inspect all three: initialization, standard resize, and child-image bounds.

## Relevant layout and resize details

The generated layout currently defines:

- Artwork size: **177 × 254**
- Standard panel origin: **(1, 1)**
- Wrapper padding: **(0, 0)**
- Artwork origin: **(0, 0)**
- Keep aspect ratio: **true**

The standard resize path in `ResizeCardinfoWindow(bool keep_ratio)` uses the generated artwork size and wrapper padding to calculate the wrapper dimensions. It then resizes the child image to the resized wrapper bounds. This path was deliberately left unchanged for the successful fix.

The old constants `CARD_IMG_WRAPPER_H_PADDING` and `CARD_IMG_WRAPPER_V_PADDING` may still exist elsewhere. Do not reintroduce them into standard-mode initialization without first reconciling them with the generated layout specification and resize calculations.

## Reproduction / verification checklist for a future port or mod

After applying the minimal fix to the equivalent initialization code:

- [ ] Build the Android ARM64 APK.
- [ ] Open a card in the standard preview.
- [ ] Check all four artwork edges; verify the right and bottom edges are visible.
- [ ] Toggle **Keep Card Picture Aspect Ratio** both on and off and inspect the standard preview.
- [ ] Open fullscreen artwork and verify it still displays correctly.
- [ ] Resize or change the relevant card-information layout if the app supports it, then repeat the checks.
- [ ] Test at the target device's normal orientation and display scaling.

A successful compile is not proof that the visual issue is fixed. The decisive acceptance test is the installed app showing the complete artwork in standard mode while fullscreen remains functional.

## If cropping returns

Do not immediately alter the artwork ratio or add arbitrary padding.

Check in this order:

1. Compare the generated layout values with the wrapper's initialization bounds.
2. Confirm the artwork child starts at the configured artwork origin (currently 0, 0).
3. Confirm the standard resize path uses the same wrapper dimensions and child bounds.
4. Inspect parent clipping, alignment, and coordinate scaling in the GUI framework (Irrlicht) if the geometry is consistent but cropping remains.
5. Change one cause at a time and verify standard and fullscreen modes independently.

## Current state and next phase

The initial objective—uncropped artwork in standard preview while retaining fullscreen artwork—has been achieved and manually confirmed on the Android app.

**This is not the final UI/layout state.** Further adjustment is expected. Treat this document as the record of the working baseline, not as a claim that all sizing, spacing, positioning, responsiveness, or visual polish is finished. When adjusting the layout, preserve the known-good two-line initialization fix and re-test both display modes.
