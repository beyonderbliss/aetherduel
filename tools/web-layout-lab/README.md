# AetherDuel Card Layout Lab

The layout lab is a shared-geometry workspace for testing EDOPro card-view
layouts in a mobile browser while keeping the native Irrlicht/C++ implementation
as the final consumer.

## Source of truth

- `layout-spec/card-view.json` contains Standard and Fullscreen geometry.
- `index.html` loads that JSON and provides an interactive browser preview.
- `scripts/generate_card_view_header.py` generates
  `gframe/card_view_layout.generated.h`.
- `gframe/game.cpp` consumes the generated constants for the native card view.

The generated header is checked in so the Android native build does not need to
run Python or parse JSON at application runtime.

## Standard mode model

The initial Standard geometry is sourced from EDOPro:

- Artwork: 177 × 254 native logical units (`CARD_IMG_WIDTH` /
  `CARD_IMG_HEIGHT` in `gframe/image_manager.h`).
- Wrapper padding: 10 horizontal and 9 vertical.
- Wrapper size: 197 × 272 logical units.
- Panel origin: (1, 1).
- Artwork child origin: (10, 9).
- Resize reference: 1024 × 640.

The browser preview models the native resize transforms:

- `Resize()` scales X against viewport width / 1024 and Y against viewport
  height / 640.
- `ResizeWithCappedWidth()` applies the same scaling, then caps the wrapper
  width when it exceeds the configured wrapper aspect ratio. It does not expand
  the width or reduce the height when the viewport is narrow.
- The artwork child is aligned with `EGUIA_SCALE`, so the preview scales its
  local rectangle with the wrapper.

The Standard controls let experiments change panel origin, wrapper padding,
artwork child origin, and the keep-ratio setting. These changes are local to
the current browser session until the resulting JSON is copied back into the
shared spec.

## Fullscreen mode model

Fullscreen retains the 177:254 artwork ratio and viewport-height sizing used by
the native fullscreen branch. The browser preview uses `contain` so the entire
card remains visible in Chrome portrait and landscape. This is a browser
presentation choice; do not treat its centering/fit behavior as proof of
pixel-identical native fullscreen positioning.

## Experiment workflow

1. Open the Cloudflare Pages deployment.
2. Choose Standard or Fullscreen and load a local card artwork image.
3. Adjust the available numeric controls and inspect portrait/landscape.
4. Copy the resulting JSON when an experiment is worth keeping.
5. Apply accepted values to `layout-spec/card-view.json`.
6. Regenerate and check the C++ header:
   ```sh
   python3 tools/web-layout-lab/scripts/generate_card_view_header.py
   python3 tools/web-layout-lab/scripts/generate_card_view_header.py --check
   ```
7. Run the layout validation workflow. A full Android/native application build
   remains a separate final validation and is not triggered by this lab branch.

## Deliberate limits

- Browser preview is a geometry/debugging tool, not a replacement for the
  Irrlicht renderer.
- Texture loading, card selection, visibility state, panel background, and
  surrounding UI remain native-owned.
- The generated header is compile-checked independently. That is not the same
  as compiling the complete Android application.
- Do not merge this branch or change the production build path until the
  native layout has been validated and explicitly approved.
