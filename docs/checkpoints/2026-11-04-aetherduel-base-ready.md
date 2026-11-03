# AetherDuel Base Checkpoint — 2026-11-04

**Branch:** `integrate/verified-card-cutout-spacing`  
**Baseline before this checkpoint:** `fb549e7b0e8980618cf4d5b0cce1bc232708bf52`

This checkpoint records the user-verified Android base before beginning the mouse-scrollbar to touch-scrolling investigation.

## Verified state

- **Fullscreen Card Viewer:** working on the clean/original EDOPro base.
- **Android 15 camera cutout and display width:** left cutout is protected precisely; the right side fills to the curved screen edge without the previous black margin. No observed artwork distortion or cropping.
- **Duel action button spacing:** 10 units; user-confirmed as the preferred spacing.
- **Deck editor button spacing:** 10 units; user-confirmed.
- **Quick Settings shortcut:** compact Setting shortcut is used instead of the removed Quick Settings tab/scrollbar.
- **Shortcut lifecycle:** shortcut is hidden when returning to the main menu.
- **Android UI Scale:** default/migrated to 220%; user has tested the resulting build and confirmed the patch works.

## Known boundaries

- This checkpoint does not include a change to FPS/VSync behavior. FPS investigation is deferred until the card assets are integrated and preview responsiveness can be tested under representative conditions.
- The thousands of card assets are not yet integrated into this base.
- No touch-scrolling implementation is included yet.

## Next task

Investigate and implement touch scrolling in place of desktop-style mouse scrollbar interaction, with scope limited to the scrolling/input mechanism.

Preserve the verified layout, cutout/window policy, fullscreen artwork rendering, spacing, shortcut lifecycle, and 220% Android UI Scale behavior. Do not modify FPS/VSync or artwork loading as part of the touch-scrolling task.

## Rollback point

Use the commit created for this checkpoint on `integrate/verified-card-cutout-spacing` as the rollback point before touch-scrolling changes.
