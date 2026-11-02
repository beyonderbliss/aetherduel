# AetherDuel Card Layout Lab

This is the first shared-layout proof of concept. It deliberately starts with the
fullscreen card artwork rectangle only.

## Source of truth

- `layout-spec/card-view.json` is read by the browser preview.
- `scripts/generate_card_view_header.py` generates
  `gframe/card_view_layout.generated.h`.
- `game.cpp` consumes the generated constants in
  `Game::ResizeCardinfoWindow()`.

The generated header is checked in so the Android native build does not need to
run Python or read a JSON file at application runtime.

## Regenerate and verify

From the repository root:

```sh
python3 tools/web-layout-lab/scripts/generate_card_view_header.py
python3 tools/web-layout-lab/scripts/generate_card_view_header.py --check
```

## Preview

Serve this directory as a static site (for example, with Cloudflare Pages using
`tools/web-layout-lab` as the build output directory). Open `index.html`
through HTTP; browser module loading and JSON fetch are not supported from
`file://` URLs.

The preview uses the same aspect ratio and origin as the native fullscreen
card rectangle. Load an artwork image locally to inspect the geometry without
adding test images to the repository.

## Deliberate limits of this first slice

- The existing native standard card layout remains untouched.
- Texture loading, card selection, visibility state, panel background and
  surrounding UI remain native-owned.
- This does not claim pixel-identical rendering. It proves shared geometry
  consumption only.
- No Android workflow or build trigger is changed by this proof of concept.
