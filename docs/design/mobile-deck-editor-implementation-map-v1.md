# Mobile Deck Editor — Native Implementation Map v1

- **Status:** Read-only implementation map
- **Target branch:** `feature/mobile-deck-editor`
- **Geometry contract:** `docs/design/mobile-deck-editor-geometry-v1.md`
- **Code snapshot audited:** branch HEAD `30bb043647060cd11ebf2251f1b2b6c616600e17`
- **Baseline checkpoint:** `38480b1db0a9f0a4357b591efa9abc4be083138f`
- **Date:** 2026-11-04
- **Scope:** Native Deck Editor only; no source code changed while preparing this map.

## 1. Purpose

This document maps the frozen geometry specification to the existing native C++ implementation. It is the implementation map to use before making the first C++ patch.

The mapping identifies:
- existing functions that must be adapted;
- existing backend functions that must be preserved;
- new layout/gesture responsibilities that do not currently exist;
- protected code paths that must not be modified as part of the Deck Editor redesign.

Line numbers below refer to the audited code snapshot above. They are navigation hints; function names and surrounding responsibilities are the durable references.

## 2. Existing native ownership

| File | Existing responsibility | Implementation role |
|---|---|---|
| `gframe/game.cpp` | GUI element construction, visibility, resize handling, coordinate conversion, card preview/info widgets | Build and place the new panel composition; centralize responsive geometry |
| `gframe/drawing.cpp` | `Game::DrawDeckBd()` manual deck/search rendering | Render the new deck tabs/grid and search result rows using shared geometry |
| `gframe/deck_con.cpp` | `DeckBuilder::OnEvent()`, hover hit testing, filtering, drag/drop, deck operations | Implement split hitboxes and touch gesture ownership while retaining existing deck operations |
| `gframe/deck_con.h` | `DeckBuilder` state and declarations | Add only the minimal state/declarations required for layout selection and gesture tracking, if needed |

The existing architecture is sufficient. Do not replace DeckBuilder or introduce a second search/deck data model.

## 3. Requirement-to-code mapping

### 3.1 Responsive panel geometry and coordinate conversion

**Contract:** Sections 3, 8, and 12 of the geometry spec.

| Existing location | Required work |
|---|---|
| `gframe/game.cpp` — GUI construction block in `Game::Game()`, around lines 549–735 | Replace Deck Editor's fixed 1024×640 panel placements with positions supplied by one layout result. This block creates `wDeckEdit`, `wFilter`, `wSort`, and `scrFilter`, plus their child controls. |
| `gframe/game.cpp` — `Game::OnResize()`, around line 3600 | Recompute the Deck Editor layout when the viewport changes and apply the resulting rectangles to the Deck Editor controls. This is the primary integration point for responsive reflow. |
| `gframe/game.cpp` — `Game::Resize()` overloads and coordinate helpers, around lines 3712 onward | Reuse the existing conversion boundary where appropriate. Do not change global conversion semantics for all EDOPro screens. Keep new dp/proportion calculations isolated to the Deck Editor layout layer. |
| **New responsibility:** a Deck Editor layout calculator, preferably a small dedicated struct/helper | Compute safe-area-adjusted usable bounds, preview width, right workspace, header, deck/search split, grid bounds, and search-list bounds once. Both drawing and hit testing must consume the same result. |

**Important implementation decision:** Do not independently recalculate panel rectangles in `game.cpp`, `drawing.cpp`, and `deck_con.cpp`. A single geometry result must be shared by GUI placement, drawing, and hit testing, otherwise touch targets will drift away from visible controls.

Do not modify the global Android cutout/window policy. The layout calculator consumes the already-established usable viewport; it does not redefine platform insets.

### 3.2 Left preview panel and classic information mode

**Contract:** Section 4 of the geometry spec.

| Existing location | Required work |
|---|---|
| `gframe/game.cpp` — `wCardImg` / `imgCard` construction, around lines 308–311 | Reuse the existing card image container and image child. Their Deck Editor placement must be driven by the new preview-panel rectangle when the Deck Editor is active. |
| `gframe/game.cpp` — `Game::PopulateTabSettingsWindow()`, around line 1430 | Reposition/re-size the existing `wInfos` tab control and its content to fit the classic information mode in the left panel. Retain Info, Log, Chat, and Repositories tabs. |
| `gframe/game.cpp` — `Game::OnResize()` and existing `stName`, `lstLog`, `lstChat` placement | Apply the left-panel geometry to information content only in the relevant Deck Editor presentation state. |
| `DeckBuilder::GetHoveredCard()`, around line 916 | Continue to call the existing card-information path when a card selection changes; the existing code calls `Game::ShowCardInfo()`. |

**Protected boundary:** `Game::ResizeCardinfoWindow()` contains the working standard/fullscreen Card Viewer geometry, including the fullscreen artwork placement. Do not rewrite this function to implement the Deck Editor preview. In particular, do not change its fullscreen branch, standard artwork constants, aspect-ratio handling, or z-order behavior. Add a Deck Editor-specific placement path that activates only in the Deck Editor context.

The existing `wCardImg` and `wInfos` widgets are shared UI resources. Their visibility, parentage, and z-order must be checked before changing their placement; do not create a duplicate card preview pipeline.

### 3.3 Header and deck actions

**Contract:** Section 5 of the geometry spec.

| Existing location | Required work |
|---|---|
| `gframe/game.cpp` — `wDeckEdit` construction, around lines 549–575 | Recompose deck name, banlist, Save, Save As, Rename, Shuffle, Sort, Clear, and Delete into the new right-side header. Existing control IDs and behavior should remain. |
| `gframe/game.cpp` — `btnHandTest` and `btnYdkeManage` construction, around lines 590 and 625 onward | Place Test Hand and YDKE management actions into the header or compact overflow presentation without changing their event IDs. |
| `gframe/deck_con.cpp` — `DeckBuilder::OnEvent()`, GUI-event switch beginning around line 164 | Preserve the existing button/combobox event handlers. Layout changes should not rewrite action semantics. |
| Existing menu/exit controls elsewhere in `game.cpp` | Trace their existing ownership and visibility before deciding whether they can be placed in the header. Do not duplicate or reimplement global navigation controls as part of the first layout patch. |

Compact mode may group secondary actions, but all existing actions must remain reachable. The first implementation should prioritize moving/reflowing existing controls rather than creating a new command system.

### 3.4 Deck panel, tabs, statistics, and card grid

**Contract:** Section 6 of the geometry spec.

| Existing location | Required work |
|---|---|
| `gframe/drawing.cpp` — `Game::DrawDeckBd()`, beginning around line 1251 | Replace the current fixed Main/Extra/Side horizontal regions with the contract's Deck panel hierarchy: Main/Extra/Side tabs, capacity/statistics row, and one card-grid viewport. Calculate grid columns from available width and minimum tile width. |
| `gframe/deck_con.cpp` — `DeckBuilder::GetHoveredCard()`, around line 916 | Replace fixed Main/Extra/Side hit-test rectangles and hardcoded row/column calculations with the same deck-grid geometry used by `DrawDeckBd()`. Hit testing must identify the active tab's card and insertion target. |
| `gframe/deck_con.cpp` — `DeckBuilder::OnEvent()`, around line 164 | Preserve existing drag/drop and click semantics while routing events through the new deck-grid hit testing. |
| `gframe/deck_con.cpp` — `push_main()`, `push_extra()`, `push_side()`, `pop_main()`, `pop_extra()`, `pop_side()`, around lines 1584–1677 | Preserve these operations as the backend mutation path. Change their callers only where new tab/grid interactions require it. |

**Current-to-target difference:** the audited renderer currently draws Main, Extra, and Side as three separate horizontal bands. The frozen design calls for tabs and a single active grid viewport. This is a genuine presentation/state addition, not just a coordinate shift. Add the smallest possible active-deck-section state; do not change the underlying `Deck` vectors or their persistence semantics.

### 3.5 Search filters and controls

**Contract:** Sections 7.1 and 8 of the geometry spec.

| Existing location | Required work |
|---|---|
| `gframe/game.cpp` — `wFilter` construction and child controls, around lines 664–727 | Reposition the existing filter controls into the search panel. Provide a standard two-column arrangement and a compact collapsed/reflow arrangement. |
| `gframe/game.cpp` — `wSort`, `cbSortType`, and `scrFilter` construction, around lines 653–662 | Place results count/sort and scrollbar within the new search panel geometry. Keep the scrollbar as a supported fallback unless device testing justifies a separate change. |
| `gframe/deck_con.cpp` — `DeckBuilder::StartFilter()`, around line 1047 | Preserve. It reads existing filter controls and applies filter state. |
| `gframe/deck_con.cpp` — `DeckBuilder::FilterCards()`, around line 1066 | Preserve search/filter semantics and result generation. Only adjust if the new UI requires a narrowly scoped presentation-state input; do not rewrite search logic for layout. |
| `gframe/deck_con.cpp` — `DeckBuilder::SortList()` | Preserve existing sort behavior and selection state. |

The filter UI is being reflowed, not redesigned as a new search engine. Keep the existing widget IDs, filter values, and event pathways.

### 3.6 Search-result rendering and split hitboxes

**Contract:** Sections 7.2 and 7.3 of the geometry spec. This is the highest-risk interaction change.

| Existing location | Required work |
|---|---|
| `gframe/drawing.cpp` — search-result block inside `Game::DrawDeckBd()`, around lines 1360–1450 | Draw each result using shared row geometry: thumbnail hitbox on the left, name/metadata scroll surface in the middle, optional card ID on the right. Keep row height and artwork ratio consistent with the geometry result. |
| `gframe/deck_con.cpp` — `DeckBuilder::GetHoveredCard()`, around line 916 | Replace the single `searchResultRect` hit test with explicit result-row and subregion tests. Resolve row index from the same scroll offset and row height used by rendering. Distinguish artwork from metadata region. |
| `gframe/deck_con.cpp` — `DeckBuilder::OnEvent()`, mouse input handling around lines 630–800 | Start card drag only when the pointer-down begins in the artwork hitbox. A vertical gesture beginning in the metadata hitbox must own the search-list scroll interaction and must not set `is_draging`. Preserve card click/selection behavior when the gesture does not become a scroll. |
| `gframe/deck_con.cpp` — `EGET_SCROLL_BAR_CHANGED` and `EMIE_MOUSE_WHEEL` handling | Keep scrollbar synchronization. Restrict list scrolling to the search list's metadata surface for touch gestures; retain wheel/scrollbar support where applicable. |

**Current behavior confirmed in source:**
- `GetHoveredCard()` currently uses one rectangle, `{ 810, 165, 995, 626 }`, for the complete search result row.
- `OnEvent()` starts drag from the current hovered card on left-button down.
- Mouse-wheel scrolling currently checks the whole search area, approximately `805,160` to `1020,630`.
- `DrawDeckBd()` currently draws the thumbnail at x≈810 and name/metadata from x≈859 onward, with a 66-unit row step.

Therefore the split is not already implemented. It requires coordinated changes to rendering, hover hit testing, and event handling.

**Touch-event investigation boundary:** the audited `DeckBuilder::OnEvent()` handles `EET_MOUSE_INPUT_EVENT` and mouse-wheel events; no dedicated touch-event branch was identified in that function. The Android-to-Irrlicht event translation was not fully audited. Before implementing the gesture state machine, inspect only the specific platform event translation path needed to determine whether touch arrives as mouse events or a distinct event. Do not expand into unrelated Android input code.

### 3.7 Card preview synchronization

**Contract:** Sections 4.3 and 7.2.

- Keep `DeckBuilder::GetHoveredCard()` as the existing selection/hover source for search and deck cards.
- Keep its existing call to `Game::ShowCardInfo()` when the hovered card changes.
- Keep `Game::ShowCardInfo()` and the existing card-data pipeline; adapt panel placement, not card lookup or card-data ownership.
- Ensure tab changes and scrolling do not accidentally change the selected card unless the pointer actually selects a different result.

## 4. Proposed new shared layout interface

The current code distributes geometry across fixed coordinates in `game.cpp`, `drawing.cpp`, and `deck_con.cpp`. The redesign should introduce one shared geometry representation.

A minimal conceptual interface (names are illustrative, not an instruction to create a particular public API):

```cpp
struct DeckEditorLayout {
    irr::core::recti usableViewport;
    irr::core::recti previewPanel;
    irr::core::recti header;
    irr::core::recti deckPanel;
    irr::core::recti searchPanel;
    irr::core::recti deckTabs;
    irr::core::recti deckStats;
    irr::core::recti deckGrid;
    irr::core::recti searchFilters;
    irr::core::recti searchResults;
    irr::core::recti searchScrollbar;
    // Search row subregions are derived from searchResults:
    // artwork, metadata, optional card ID.
};
```

The implementation may use a different representation if it is simpler and fits the existing codebase. Requirements:
- calculate once per resize/layout change;
- use the same geometry for GUI controls, drawing, and hit testing;
- keep conversion between dp, viewport pixels, and EDOPro logical coordinates centralized;
- expose search row subregions from the same row-layout helper;
- avoid global changes to generic `Game::Resize()` behavior.

## 5. Functions and code that should remain unchanged

The following are explicitly not targets for logic rewrites:

| Existing function/path | Reason |
|---|---|
| `DeckBuilder::StartFilter()` | Existing filter-control-to-filter-state mapping remains valid |
| `DeckBuilder::FilterCards()` | Existing search engine and result generation are outside layout scope |
| `DeckBuilder::push_main/extra/side()` | Existing deck mutation and validation path must remain |
| `DeckBuilder::pop_main/extra/side()` | Existing deck removal path must remain |
| Deck save/import/export operations | Header relocation must not change persistence or interchange behavior |
| `Game::ResizeCardinfoWindow()` fullscreen branch | Protected working Card Viewer and artwork geometry |
| Android safe-area/window/cutout implementation | Protected verified camera-cutout behavior |
| Duel UI drawing and controls | Outside Deck Editor scope |

A function may receive a narrow call-site adaptation, but its underlying behavior must remain unchanged unless a separate requirement proves otherwise.

## 6. Recommended implementation sequence

Keep each step independently reviewable. Do not trigger a build until the implementation stage explicitly reaches its build/test checkpoint.

### Step 1 — Shared geometry foundation
- Add the Deck Editor layout data/helper.
- Compute usable bounds, preview/header/workspace panels, deck/search split, and compact/standard/wide mode.
- Connect it to `Game::OnResize()`.
- Do not change card rendering or interactions yet.

### Step 2 — Native control placement and preview composition
- Reposition existing header/filter/sort/scrollbar controls from the shared layout.
- Integrate existing `wCardImg` and `wInfos` into the left panel without touching the fullscreen Card Viewer path.
- Confirm all existing controls remain reachable.

### Step 3 — Deck renderer and active tab
- Change `Game::DrawDeckBd()` to render the new panel structure and active Main/Extra/Side grid.
- Add only the minimal active-section state.
- Update `GetHoveredCard()` deck hit testing to match the grid.
- Keep existing push/pop backend functions.

### Step 4 — Search row geometry and split hitboxes
- Render search rows from shared row geometry.
- Add distinct thumbnail and metadata hitboxes.
- Implement gesture ownership and scrolling on the metadata side.
- Inspect Android-to-Irrlicht touch translation only as required for this step.

### Step 5 — Regression and device validation
- Verify card selection, add/remove, drag/drop, filters, sort, save, import/export, and Test Hand.
- Validate standard landscape, constrained landscape, and wide/tablet layout.
- Confirm all protected baseline items remain unchanged.

## 7. Regression-critical test matrix

| Test | Expected result |
|---|---|
| Select search result by tapping artwork | Selects card; drag can begin from artwork |
| Drag search thumbnail into Main/Extra/Side | Existing deck insertion path works |
| Vertical swipe beginning on name/metadata | Scrolls search results; never starts card drag |
| Tap name/metadata without vertical movement | Card selection/info remains available |
| Change search filter or sort | Existing results and ordering update correctly |
| Change Main/Extra/Side tab | Shows correct deck section; deck data remains intact |
| Add/remove cards in each section | Existing push/pop behavior and limits remain correct |
| Resize/rotate among target landscape aspect ratios | Panels and hitboxes remain aligned; no overlap |
| Open/close classic information mode | Preview and info remain synchronized |
| Enter/exit fullscreen Card Viewer | Existing fullscreen artwork geometry remains unchanged |
| Return to duel/main menu | Existing protected baseline UI remains unchanged |

## 8. First native patch boundary

The first C++ patch should be limited to **Step 1: shared geometry foundation**. It should not yet:
- replace the deck renderer;
- add the active deck tab interaction;
- alter search gestures;
- change Android touch translation;
- modify card artwork or fullscreen Card Viewer code;
- change deck backend logic.

This keeps the first implementation diff small and makes geometry validation possible before interaction changes are layered on top.

## 9. Audit conclusion

The minimum native implementation surface is:

1. `gframe/game.cpp`: Deck Editor control construction, resize/layout integration, and Deck Editor-specific preview/info placement.
2. `gframe/drawing.cpp`: `Game::DrawDeckBd()` for deck/search panel rendering.
3. `gframe/deck_con.cpp`: `DeckBuilder::GetHoveredCard()` and `DeckBuilder::OnEvent()` for geometry-aligned hit testing and split drag/scroll interaction.
4. `gframe/deck_con.h`: minimal state/declaration additions only if the selected-tab or gesture model requires them.

No deck search backend rewrite is required. The next implementation action, after this map is accepted, is Step 1 only.
