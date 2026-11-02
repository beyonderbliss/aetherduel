# Fullscreen Card Artwork Mod

Status: **implemented; preserve as a known-good behavior**

Branch: feat/card-view-original-clean

This document records the fullscreen card-artwork modification separately from the standard-preview crop fix. The current branch implementation is the source of truth; earlier experimental scaling approaches are historical only.

## Intended behavior

- Fullscreen is toggled by tapping/clicking the visible card artwork during a duel.
- Tapping/clicking the artwork again returns to standard preview.
- The artwork is flush against the left edge of the screen.
- It uses as much screen height as possible while preserving the card aspect ratio.
- The artwork wrapper is layered above the normal duel UI. Existing controls are overlaid by the artwork rather than individually redesigned.
- The existing card image and texture are reused; fullscreen does not add another image-loading path.

## Files changed

### 1. gframe/game.h — fullscreen state

A runtime state flag was added to Game:

~~~cpp
bool cardViewFullscreen = false;
~~~

It records which card-artwork layout is active. It is not a persisted user preference.

### 2. gframe/event_handler.cpp — tap/click toggle

Inside ClientField::OnEvent(), the mouse/touch input path checks that both the card wrapper and image are visible, then hit-tests the image's absolute screen bounds.

~~~cpp
if(mainGame->wCardImg->isVisible() &&
   mainGame->imgCard->isVisible() &&
   mainGame->imgCard->getAbsolutePosition().isPointInside(mousepos)) {
    mainGame->cardViewFullscreen = !mainGame->cardViewFullscreen;
    mainGame->ResizeCardinfoWindow(gGameConfig->keep_cardinfo_aspect_ratio);
    break;
}
~~~

This toggles the state and delegates all geometry changes to ResizeCardinfoWindow(). No extra fullscreen button or swipe recognizer is required.

The event-handler translation unit also needs the complete Irrlicht image interface:

~~~cpp
#include <IGUIImage.h>
~~~

This include was added in commit 6e798d9a797e0ccf3bd8d943314f7c9454cbe480 because the hit-test path uses the image interface.

### 3. gframe/game.cpp — fullscreen geometry

Game::ResizeCardinfoWindow(bool keep_ratio) contains a separate fullscreen branch before the standard layout path.

The fullscreen branch:

1. Reads the real viewport dimensions using driver->getScreenSize().
2. Gets the target card ratio from generated constants layout::kFullscreenAspectWidth and layout::kFullscreenAspectHeight.
3. Calculates card width as min(viewport width, viewport height multiplied by the target ratio).
4. Calculates card height from that width and the same ratio.
5. Places the wrapper at x = 0 and vertically centers it.
6. Places the image child at (0, 0) inside the wrapper, with the same width and height.
7. Brings the wrapper to the front of its parent, making the artwork overlay sibling duel UI.
8. Returns immediately so standard-mode sizing is not applied over fullscreen geometry.

Core geometry:

~~~cpp
const float aspect = static_cast<float>(layout::kFullscreenAspectWidth) /
    layout::kFullscreenAspectHeight;
const auto viewport = driver->getScreenSize();
const irr::s32 cardWidth = std::min<irr::s32>(
    viewport.Width,
    static_cast<irr::s32>(viewport.Height * aspect));
const irr::s32 cardHeight = static_cast<irr::s32>(cardWidth / aspect);
const irr::s32 top =
    (static_cast<irr::s32>(viewport.Height) - cardHeight) / 2;

wCardImg->setRelativePosition(
    irr::core::recti(0, top, cardWidth, top + cardHeight));
imgCard->setRelativePosition(
    irr::core::recti(0, 0, cardWidth, cardHeight));
wCardImg->getParent()->bringToFront(wCardImg);
~~~

When fullscreen is inactive, the standard branch resizes the wrapper/image using the standard layout and sends the wrapper behind sibling UI elements.

Fullscreen is reset when the duel is no longer active:

~~~cpp
if(!dInfo.isInDuel)
    cardViewFullscreen = false;
~~~

## Z-order correction

Irrlicht's z-order methods are invoked through the parent element, not directly on the child wrapper. The current implementation is:

~~~cpp
wCardImg->getParent()->bringToFront(wCardImg); // fullscreen
wCardImg->getParent()->sendToBack(wCardImg);   // standard preview
~~~

This correction is recorded in commit 6dc93d294c4ce09ceb149709d27fdc7c3b1e6e04.

## Relevant implementation history

These commits explain how the current implementation evolved:

- 4d408e6052ce7dcfcf4b0e45bfdd521acaa533ac — introduced the left-side fullscreen layout and an early dynamic texture-scaling/front-layer implementation.
- 8754c28ec177f2cea311f7233284165c0090c86b — refined the image-scaling code and removed unsuitable direct front-order calls.
- 11541dcdb5ed666b0178d31228ec45a2e85d7928 — added a public getImage() getter for an intermediate texture-inspection approach.
- 6e798d9a797e0ccf3bd8d943314f7c9454cbe480 — included the complete IGUIImage interface needed by the hit test.
- 6dc93d294c4ce09ceb149709d27fdc7c3b1e6e04 — corrected z-order calls to use the wrapper's parent.

Important: the earlier manual texture scaling/getImage() approach is historical. The current fullscreen path uses the generated aspect-ratio constants and sizes the existing image element to the fullscreen wrapper bounds. Do not reintroduce the intermediate manual setImageScale() logic without a specific measured need and a separate visual test.

## Constraints to preserve

1. Keep the artwork aligned to the left screen edge.
2. Preserve the card aspect ratio; do not stretch the artwork to fill the entire viewport width.
3. Use actual viewport dimensions rather than a hard-coded screen height.
4. Keep the wrapper in front only in fullscreen and behind sibling UI in standard mode.
5. Keep tap/click on the artwork as the toggle unless the interaction design is deliberately changed.
6. Reuse the existing image/texture path; do not modify texture loading as part of a layout-only change.
7. Keep fullscreen restricted to the active duel lifecycle and clear its state when leaving the duel.
8. Do not disturb the standard preview initialization fix documented in CARD_ARTWORK_PREVIEW_MOD.md.

## Verification checklist

- [ ] In a duel, tap the visible card artwork; fullscreen opens.
- [ ] The artwork begins at the left edge.
- [ ] The image uses the available height while preserving its aspect ratio.
- [ ] The artwork overlays the normal duel UI as intended.
- [ ] Tap the artwork again; standard preview returns.
- [ ] Leave the duel; fullscreen state is cleared.
- [ ] Repeat after supported orientation, viewport, or display-scale changes.

A successful build alone does not verify the visual behavior. Confirm the installed Android ARM64 app manually.

## Current status

Fullscreen artwork is working and must be preserved while the remaining card-view layout is adjusted. The standard-preview crop fix is a separate, minimal initialization change; neither fix should be used as a reason to rewrite the other mode.
