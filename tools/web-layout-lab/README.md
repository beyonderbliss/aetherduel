# PrimaNyx Keyboard Layout Lab

The lab edits the split Android text-entry keyboard using a 24-column × 4-row logical editing grid. Each key has an editable X/Y position and width. The preview keeps the two key groups separated by a center spacer, supports pointer/touch dragging, horizontal resizing, adjustable snap increments, and numeric position/width controls.

## Viewport and units

Android keyboard window:
- width: MATCH_PARENT
- height: WRAP_CONTENT
- gravity: BOTTOM

Geometry values use Android dp. The browser preview renders the shared layout specification at the available page width. The split pitch is calculated from the available width after subtracting the fixed center spacer. Row height, row gap, key margin, and panel spacing remain dp values.

## Editing

1. Open the Cloudflare Pages deployment built from this branch.
2. Choose Letters or Symbols.
3. Drag a key to reposition it. Drag its yellow corner handle to change its width.
4. Change Snap to 1, 2, or 3 cells to control movement increments.
5. Adjust Center gap to compare thumb reach in landscape. The selected value is stored as `keys.centerGap` in the shared JSON.
6. Select a key to edit its Column X, Row Y, and Width numerically.
7. Copy the JSON and update `layout-spec/keyboard.json` when you want to persist key edits.

The ordinary keycaps use a shared 1.8-unit width on a 2-unit pitch. The inner columns are aligned vertically (T/G/V on the left and Y/G/V on the right). DONE is exactly two standard keycap widths. SPACE is an intentional bridge: it fills the remaining bottom-row span and includes the center region.

## Android integration

The Android workflow reads `layout-spec/keyboard.json` while generating `TextEntry.java`. The first three rows are rendered as two independent weighted groups with a fixed center spacer in dp; group key widths and pitches are derived independently from the shared split pitch. The bottom row uses left and right groups plus a flexible SPACE key that bridges the center region. The center gap is no longer represented as a large weighted column count.

Row gap and horizontal key margin each have one source of truth under `keys`. The workflow remains manual through `workflow_dispatch`; editing the lab does not trigger an APK build.

## Scope

- Keyboard editor and native keyboard renderer only; the completed card-view module is untouched.
- No APK build or deployment is triggered by editor changes.
- The browser preview does not emulate Android system-bar insets or device-specific window behavior.
