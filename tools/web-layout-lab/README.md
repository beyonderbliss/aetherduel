# PrimaNyx Keyboard Layout Lab

The lab edits the custom Android text-entry keyboard using a 24-column × 4-row grid. Every key has its own grid X/Y position and width in cells. The editor supports pointer/touch dragging, horizontal resizing from the lower-right handle, adjustable snap increments, and numeric position/width controls. It prevents keys from overlapping.

## Viewport and units

Android keyboard window:
- width: MATCH_PARENT
- height: WRAP_CONTENT
- gravity: BOTTOM

Geometry values use Android dp. The browser preview renders the same grid at the available page width; the grid proportions and key placements are shared with the Android build workflow. Row height and panel spacing remain dp values.

## Editing

1. Open the Cloudflare Pages deployment built from this branch.
2. Choose Letters or Symbols.
3. Drag a key to reposition it. Drag its yellow corner handle to change its width.
4. Change Snap to 1, 2, or 3 cells to control movement increments.
5. Select a key to edit its Column X, Row Y, and Width numerically.
6. Copy the JSON and update layout-spec/keyboard.json when you want to persist a layout.

The current editor intentionally keeps key height to one row; width and position are independently configurable. This maps to the current Android native keyboard row renderer.

## Android integration

The Android workflow reads layout-spec/keyboard.json while generating TextEntry.java. It creates each native keyboard row from the per-key X coordinate and width, inserting weighted spacer cells for empty grid columns. Changing a key's row or column moves it in the native keyboard too. Geometry and palette values are also applied from the shared specification.

The workflow remains manual through workflow_dispatch. Editing the lab does not trigger an APK build.

## Scope

- Keyboard editor only; the completed card-view module is untouched.
- No APK build or deployment is triggered by editor changes.
- The browser preview does not emulate Android system-bar insets or device-specific window behavior.
