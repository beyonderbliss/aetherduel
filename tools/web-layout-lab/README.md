# PrimaNyx Keyboard Layout Lab

This lab is dedicated to the custom Android text-entry keyboard. The old card-view experiment is no longer the active lab UI; the completed native card-view implementation and its historical artifacts are left untouched.

## Shared viewport model

The Android keyboard is an Android AlertDialog window with:
- width: MATCH_PARENT
- height: WRAP_CONTENT
- gravity: BOTTOM

It is not a fixed 2712×1220 phone canvas. Its height is content-driven by panel padding, the preview field, row heights, and row gaps. The browser lab renders the keyboard at the available page width and computes the same content height from the shared dp geometry.

The source of truth is layout-spec/keyboard.json. All dimensions are Android dp. In the browser preview, CSS pixels represent those logical dp values at a 1:1 layout scale; the browser page supplies the available viewport width.

## Editing

1. Open the Cloudflare Pages deployment for this branch.
2. Change geometry and palette controls; the preview updates immediately.
3. Switch between Letters and Symbols to inspect both key sets.
4. Copy the JSON and apply accepted values to layout-spec/keyboard.json.
5. Reload the page to verify the committed specification.

## Android integration

The Android build workflow reads the same JSON file when it injects TextEntry.java into the pinned Android wrapper. Panel padding, preview dimensions, row height/gap, center gap, key margins, corner radii, colors, and text sizing are taken from that shared spec. This connects the lab and native keyboard instead of maintaining a second set of hand-copied geometry values.

The Android build remains manual through workflow_dispatch. Editing the lab does not trigger an APK build.

## Scope

- This lab edits the custom text-entry keyboard only.
- It does not modify the completed card-view module.
- Browser preview is a geometry tool; Android rendering and system window insets remain native-owned.
