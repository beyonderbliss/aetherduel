# Standard Card Artwork Scaling (KCPAR ON)

Status: **5% enlargement visually verified on Android**

Branch: `feat/card-view-original-clean`

## Purpose

Record the working formula used to enlarge the standard card artwork when **Keep Card Picture Aspect Ratio (KCPAR)** is enabled. This makes future size adjustments deliberate instead of guessing at width-only offsets.

The objective is to reduce the visual gap between the bottom of the standard card artwork and the top of the Card Info tab panel by enlarging the card itself.

Do **not** move or resize the Card Info panel to achieve this.

## Current verified setting

File: `gframe/game.cpp`

Function: `Game::ResizeCardinfoWindow(bool keep_ratio)`

Current scale factor:

```cpp
constexpr float standardArtworkScale = 1.05f;
```

The user tested the resulting Android APK and confirmed:
- The standard artwork becomes slightly larger.
- The gap above the Card Info panel is reduced.
- The visual result is acceptable for now; keep 1.05 until a new explicit adjustment is requested.

Implementation:

```cpp
if(keep_ratio && layout::kStandardKeepAspectRatio) {
    // Enlarge the standard artwork uniformly; leave KCPAR-off and fullscreen unchanged.
    constexpr float standardArtworkScale = 1.05f;
    const irr::s32 enlargedWidth = static_cast<irr::s32>(std::round(rect.getWidth() * standardArtworkScale));
    const irr::s32 enlargedHeight = static_cast<irr::s32>(std::round(rect.getHeight() * standardArtworkScale));
    rect.LowerRightCorner.X = rect.UpperLeftCorner.X + enlargedWidth;
    rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + enlargedHeight;
}
```

## Formula

First, the existing resize logic calculates the base rectangle while preserving the original artwork aspect ratio:

```
baseWidth  = rect.getWidth()
baseHeight = rect.getHeight()
```

Then apply one shared scale factor to both dimensions:

```
newWidth  = round(baseWidth  * standardArtworkScale)
newHeight = round(baseHeight * standardArtworkScale)
```

The top-left corner remains unchanged. Only the lower-right corner is extended:

```
newRight  = rect.left + newWidth
newBottom = rect.top  + newHeight
```

Because width and height use the **same factor**, the aspect ratio is preserved apart from unavoidable integer-pixel rounding.

### How to tune it later

Change only `standardArtworkScale`:

| Factor | Effect |
|---|---|
| `1.00f` | No additional enlargement; base KCPAR sizing |
| `1.02f` | 2% larger |
| `1.05f` | Current verified setting: 5% larger |
| `1.07f` | 7% larger |
| `1.10f` | 10% larger |

Use a value below 1.00 to reduce the artwork proportionally, for example `0.98f` for 2% smaller.

The percentage is relative to the **already-resized base rectangle**, not directly to the raw 177 × 254 layout dimensions. The base rectangle can vary with the device's window scaling and the `ResizeWithCappedWidth()` calculation.

## Scope and invariants

The scaling block is guarded by:

```cpp
keep_ratio && layout::kStandardKeepAspectRatio
```

Therefore:
- KCPAR ON, standard mode: apply the extra uniform scale.
- KCPAR OFF: do not apply the extra scale; retain the existing `Resize()` behavior.
- Fullscreen: returns before reaching this block; fullscreen sizing and its inset remain independent.
- Card Info panel: do not move or resize it as part of this adjustment.
- Artwork aspect ratio: preserve it; never change width and height by different percentages.

The wrapper and child image must continue to use matching final dimensions. The existing child-image positioning code after this block synchronizes `imgCard` to the final `rect`; do not remove or bypass it.

## Why the previous +12 experiment was ineffective

The earlier experiment increased the requested width and also recalculated the target aspect ratio from that wider width. This did not reliably enlarge both final dimensions; depending on the `ResizeWithCappedWidth()` branch and device scaling, the resulting height could remain effectively unchanged.

The current method first calculates the normal base rectangle, then scales its **final width and height together**. This is the verified approach.

## Future adjustment and verification

When changing the factor:
1. Change only `standardArtworkScale`.
2. Keep the KCPAR guard intact.
3. Do not modify the Card Info panel's position or dimensions.
4. Do not modify the fullscreen branch.
5. Build the Android ARM64 APK and inspect the installed app.

Check:
- Card is visibly larger/smaller as expected.
- Gap between card bottom and Card Info panel is reduced/increased accordingly.
- Artwork is not cropped or stretched.
- No collision with nearby UI or physical screen edges.
- KCPAR OFF remains unchanged.
- Fullscreen remains unchanged.

A larger factor does not guarantee a particular pixel gap on every device. Confirm the actual visual result before accepting a new value.

## Change history

- **1.05f** — current setting; manually verified by the user on Android. Commit: `aab7d568c3ef50c39546ab3dd81116a511f7d4c0`.
