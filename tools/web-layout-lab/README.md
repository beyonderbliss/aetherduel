# PrimaNyx Keyboard Layout Lab

The lab edits the split Android text-entry keyboard using a measured logical grid. It preserves the conventional staggered QWERTY arrangement while allowing the two halves and their center gap to be measured independently.

## Workspace and measurement

- Fine square grid: five minor subdivisions per logical cell, with a stronger major-cell line.
- Top and left rulers provide cell and row references.
- The center gap is adjustable from **4 to 8 logical cells**. The two key groups remain separate; SPACE remains the intentional bridge.
- A center-gap band and center axis make the split geometry visible.
- The current grid width is calculated as 20 key-area cells plus the selected center-gap cells.
- All keyboard geometry values remain Android dp where the native layout specification expects dp. The gap control is a logical-cell design control for the browser Lab.

## Editing

1. Open the Cloudflare Pages deployment built from this branch.
2. Choose Letters or Symbols.
3. Select a snap increment, then drag a key to reposition it. Drag its yellow corner handle to change its width.
4. Keys snap to the selected grid increment. Magnetic alignment attracts a dragged key edge to nearby key edges; yellow guides and a live coordinate readout show the alignment.
5. Adjust Center gap from 4 to 8 cells to compare the split spacing.
6. Select a key to edit Column X, Row Y, and Width numerically.
7. Copy the JSON to inspect or preserve the layout edits.

The editor upgrade changes only Lab measurement and interaction tooling; it does not rewrite the existing key positions, widths, or stagger.

## Android integration

The Android workflow reads `layout-spec/keyboard.json` while generating `TextEntry.java`. Native keyboard rendering remains unchanged by this Lab-only editor update. The center-gap cell control is a design-space control; do not interpret it as a native dp value until the final measured geometry is explicitly translated for Android.

The workflow remains manual through `workflow_dispatch`; editing the Lab does not trigger an APK build.

## Scope

- Keyboard Lab editor only; native keyboard renderer and completed card-view module are untouched.
- No APK build or deployment is triggered by editor changes.
- The browser preview does not emulate Android system-bar insets or device-specific window behavior.
