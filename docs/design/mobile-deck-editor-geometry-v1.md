# Mobile Deck Editor — Universal Geometry Specification v1

- **Status:** Frozen design contract for native implementation
- **Target branch:** `feature/mobile-deck-editor`
- **Baseline:** `integrate/verified-card-cutout-spacing` checkpoint `38480b1db0a9f0a4357b591efa9abc4be083138f`
- **Reference:** `EDOPro Deck Editor 1_1 Simulator (Landscape) (1).html`
- **Implementation target:** Native EDOPro C++ / Irrlicht. The HTML file is a visual and structural reference, not a web-port target.
- **Version:** 1.0
- **Date:** 2026-11-04

## 1. Purpose and scope

This document freezes the layout architecture and interaction geometry for the mobile Deck Editor. It is the implementation contract for subsequent native C++ work.

The goal is a landscape-first, responsive Deck Editor that prioritizes usable card artwork, readable search results, and touch-friendly deck construction across phones and tablets.

This contract covers:
- panel composition and responsive sizing;
- coordinate conversion and safe-area handling;
- preview, deck workspace, and search workspace geometry;
- card-grid and search-result layout;
- separation of search-result drag and scroll hitboxes;
- acceptance criteria and implementation boundaries.

It does not authorize changes to unrelated duel UI, Card Viewer behavior, Android window/cutout policy, asset-loading strategy, or other verified baseline features.

## 2. Source-of-truth hierarchy

When implementing, use this order:

1. This document for native layout and interaction requirements.
2. The HTML prototype for visual hierarchy, content grouping, and intended panel composition.
3. Existing EDOPro code for established deck state, filtering, sorting, card operations, and persistence behavior.

If the prototype's fixed CSS dimensions conflict with this document's responsive rules, this document takes precedence. Do not copy the prototype's fixed 16:9 assumptions or browser-specific behavior into native code.

## 3. Layout architecture

The editor has three primary regions:

1. **Left preview panel:** approximately one quarter of usable viewport width.
2. **Right editor region:** remaining width, containing the header and workspace.
3. **Right workspace:** deck panel and search panel, arranged at a 7:5 width ratio.

The header belongs to the right editor region, not above the left preview panel.

### 3.1 Structural hierarchy

```text
Usable landscape viewport
├── Left preview panel (~25%)
└── Right editor region
    ├── Header / deck actions
    └── Workspace
        ├── Deck panel (7/12)
        └── Search panel (5/12)
```

### 3.2 Coordinate model

All layout calculations must be based on the **usable viewport after applying the platform-provided safe-area insets**.

Let:
- `W` = usable viewport width;
- `H` = usable viewport height;
- `P` = outer panel padding;
- `G` = gap between adjacent panels;
- `L` = preview panel width;
- `R` = right editor region width;
- `U` = inner workspace width after the deck/search gap.

Use the following geometry:

```text
Wi = W - 2P
L  = clamp(0.25 * Wi, 180 dp, 320 dp)
R  = Wi - L - G
U  = R - G
D  = (7 / 12) * U
S  = (5 / 12) * U
```

Where:
- `D` = deck panel width;
- `S` = search panel width.

The 7:5 ratio applies to the workspace content width `U`; the deck/search gap is accounted for separately. If a target device cannot satisfy the preview minimum and workspace minimum simultaneously, enter compact responsive mode rather than allowing overlap or negative widths.

### 3.3 Initial geometry values

These values are frozen as **initial design targets**, not as device-validated constants:

| Element | Initial target |
|---|---:|
| Outer padding | 4 dp |
| Inter-panel gap | 4 dp |
| Preview width | 25% of inner width, clamped to 180–320 dp |
| Header height | 40–48 dp |
| Deck tab row | 32–40 dp |
| Search result row | 60–68 dp |
| Deck card tile minimum width | 60–72 dp |
| Card-grid gap | 6 dp |

The implementation must keep these values centralized and tunable. Do not scatter magic numbers across renderer and event-handling code.

### 3.4 Native coordinate conversion

The layout contract is expressed in density-independent units (dp) and proportions. Existing EDOPro rendering uses a 1024×640 logical reference coordinate space.

Native implementation must use one consistent conversion layer between:
- safe-area-adjusted physical viewport dimensions;
- dp-based layout dimensions;
- EDOPro/Irrlicht logical coordinates.

Do not mix raw screen pixels, dp, and 1024×640 logical coordinates inside individual panel calculations. The conversion layer must be the only place that translates between these spaces.

## 4. Preview panel

The left panel occupies the full usable height and supports two presentation modes.

### 4.1 Full artwork mode

- Display the selected card artwork using contain/fit behavior.
- Preserve the original card aspect ratio (approximately 1:1.435).
- Never stretch or crop artwork to fill the panel.
- Center the artwork within the available preview region.
- Tapping the preview toggles between full artwork and classic information mode.

### 4.2 Classic information mode

- Show a smaller card thumbnail above the information area.
- Place card information below the thumbnail.
- The information area may scroll independently.
- Maintain the same card aspect ratio for the thumbnail.

### 4.3 Native integration

Reuse and reposition/adapt the existing native card preview and information systems (including the existing `wCardImg` and `wInfos` elements where appropriate). Do not create a second, independent card-data or preview pipeline.

The selected card must remain synchronized with the search result selection and existing deck-editor state.

## 5. Header and actions

The header occupies the top of the right editor region.

It must preserve access to the existing deck-level actions represented by the prototype:
- deck name;
- banlist selection;
- Save and Save As;
- Rename;
- Shuffle and Sort;
- Clear and Delete;
- Test Hand;
- YDKE Import/Export;
- Exit and Menu.

On compact landscape devices, secondary actions may move into an overflow menu or a collapsible action group. Do not silently remove existing actions or change their behavior.

The header must not overlap the workspace. Its height is deducted before calculating the workspace's available vertical region.

## 6. Deck workspace

The deck panel occupies 7/12 of the workspace width.

Its vertical hierarchy is:

1. Main / Extra / Side tabs;
2. capacity and statistics row;
3. card-grid viewport.

Requirements:
- Preserve existing Main, Extra, and Side deck semantics.
- Preserve existing deck state and operations; layout work must not replace deck backend logic.
- Use five columns as the prototype's standard-landscape baseline.
- Adapt the number of columns dynamically, targeting approximately 4–7 columns according to available panel width.
- Keep card tiles proportional; do not distort card artwork.
- Use the initial 60–72 dp tile-width target and 6 dp gap as tunable starting values.
- The card-grid viewport must use the remaining vertical space and must not overlap tabs or statistics.

The number of columns must be derived from available width and minimum tile width, not from a hardcoded device resolution.

## 7. Search workspace

The search panel occupies 5/12 of the workspace width.

Its vertical hierarchy is:

1. filter controls;
2. filter actions;
3. results count and sort controls;
4. scrollable search-result list.

### 7.1 Filters

- Use a two-column filter arrangement when the available width supports readable controls.
- In compact mode, collapse or reflow filters instead of shrinking controls until they become unusable.
- Keep filter actions and results controls accessible.
- Retain existing filter/search backend behavior.

### 7.2 Search-result row

Each row contains:
- card artwork thumbnail on the left;
- card name and type/metadata in the flexible middle region;
- card ID on the right when sufficient width is available.

Initial row-height target: 60–68 dp.

The thumbnail preserves the card's aspect ratio. The name/metadata area receives the flexible width; optional ID text may be reduced or hidden in compact mode, but the card identity must remain accessible through existing information behavior.

### 7.3 Mandatory split hitbox contract

A search result row must not have one undifferentiated interaction rectangle.

It must expose two distinct interaction regions:

**A. Artwork/thumbnail hitbox — drag target**
- Initiates the existing card drag/add interaction.
- Must not initiate list scrolling merely because the user starts a gesture on the artwork.
- Must retain a practical touch target; 44 dp is the minimum target where geometry permits.

**B. Name/metadata hitbox — scroll surface**
- Receives vertical touch scrolling for the search-results list.
- Must not initiate card dragging.
- A vertical gesture beginning here scrolls the list rather than selecting/dragging the card.

The implementation must resolve gesture ownership at the start of the gesture and avoid accidental handoff that makes card dragging unreliable. Existing click/select behavior must remain available without turning a scroll gesture into a card drag.

This split is a required behavior, not an optional optimization. It is based on the observed usability issue where whole-row touch scrolling interfered with dragging cards into the deck.

## 8. Responsive behavior

The layout must respond to usable viewport dimensions and aspect ratio. Do not hardcode 16:9, one phone model, or one physical resolution.

### 8.1 Compact landscape

When the viewport is constrained:
- reduce preview width within its defined limits;
- compact or collapse filters;
- compress header presentation and move secondary actions to overflow;
- reduce deck-grid columns as necessary;
- preserve minimum usable touch targets;
- never allow panels to overlap.

If the minimum panel constraints cannot all be satisfied, prioritize a valid compact/reflow layout over forcing the standard 25% / 7:5 proportions.

### 8.2 Standard landscape

- Use the 25% preview target.
- Use the 7:5 deck/search workspace split.
- Use five deck-grid columns as the starting arrangement.
- Keep the header and filter hierarchy visible.

### 8.3 Wide landscape / tablets

- Cap preview width at the defined 320 dp target.
- Allow the deck grid to gain columns, up to the 4–7 target range where appropriate.
- Use additional space to improve card visibility and control spacing; do not enlarge every control indiscriminately.
- Maintain the same panel hierarchy and interaction contract.

### 8.4 Safe areas

Use the usable viewport supplied by the existing Android/window safe-area handling. Do not modify the verified Android cutout/window policy as part of this Deck Editor task.

## 9. Existing native architecture boundaries

The read-only audit identified the following implementation areas:

- `gframe/deck_con.h/.cpp`: DeckBuilder state, filtering, hover, drag, and deck operations.
- `gframe/game.cpp`: GUI construction, control placement, and resize handling.
- `gframe/drawing.cpp`: `Game::DrawDeckBd()` and manual deck/search rendering.

Implementation should preserve the existing responsibilities:
- Keep card filtering, sorting, deck state, add/remove operations, and persistence in the existing DeckBuilder/backend path.
- Rework geometry, renderer placement, and hit testing to match this contract.
- Introduce the split search-result hitboxes in the relevant event/hit-test path.
- Inspect Android-to-Irrlicht touch event translation only when needed to implement the gesture contract; do not broaden the investigation into unrelated platform code.

This document does not prescribe exact C++ class names, function signatures, or a new UI framework. Choose the smallest native changes that satisfy the contract.

## 10. Explicitly protected baseline

The following verified features are outside this task's scope and must remain unchanged unless separately authorized:

- Android 15 left camera-cutout protection and right-edge fill behavior;
- Fullscreen Card Viewer behavior and artwork rendering;
- verified 10-unit spacing in duel action buttons and Deck Editor buttons;
- Quick Settings shortcut lifecycle and visibility behavior;
- Android UI Scale default/migration behavior (220%);
- FPS/VSync behavior;
- card asset loading/integration strategy.

The implementation branch must continue to be based on the verified checkpoint stated at the top of this document. Do not base this work on the later experimental deck-content commit containing the 30-card Dark Magician test fixture.

## 11. Acceptance criteria

The native implementation is not considered complete until all of the following are verified:

### Layout
- [ ] Preview panel is on the left and occupies approximately 25% of usable width in standard landscape.
- [ ] Header is above the right-side workspace only.
- [ ] Deck and search panels follow the 7:5 content-width ratio in standard mode.
- [ ] No panel overlaps or extends beyond the usable viewport.
- [ ] Compact and wide layouts adapt without relying on a fixed 16:9 resolution.

### Preview
- [ ] Full artwork mode preserves the card ratio and never stretches/crops artwork.
- [ ] Classic information mode shows a thumbnail and independently scrollable information.
- [ ] Preview selection stays synchronized with search/deck selection.

### Deck
- [ ] Main / Extra / Side tabs and capacity/statistics remain functional.
- [ ] Grid columns adapt to available width.
- [ ] Existing card add/remove, drag, sort, save, and deck-state behavior remain intact.

### Search and touch
- [ ] Filters remain usable in compact and standard layouts.
- [ ] Search results remain readable and scrollable.
- [ ] Drag starts from the artwork/thumbnail hitbox.
- [ ] Vertical scrolling starts from the name/metadata hitbox.
- [ ] Scrolling does not steal artwork drag gestures.
- [ ] Existing selection and card-information behavior remain intact.

### Regression protection
- [ ] Protected baseline features listed in Section 10 remain unchanged.
- [ ] No unrelated UI or platform changes are included.

## 12. Values requiring device validation

The following are design targets, not empirically validated final constants:
- preview clamp (180–320 dp);
- outer padding and panel gaps (4 dp);
- header and tab heights;
- search-result row height;
- deck tile minimum width and grid gap;
- compact-mode thresholds and filter-collapse breakpoints.

Validate these on at least:
1. the current target Android landscape phone;
2. a narrower landscape viewport or equivalent constrained aspect ratio;
3. a wide landscape/tablet viewport.

Adjust the tunable values only when measurements show a usability or fit problem. Preserve the layout architecture and interaction contract unless a deliberate spec revision is recorded.

## 13. Implementation rule

Treat this document as the frozen v1 contract. Before changing native code, map each requirement to the smallest relevant implementation location and keep the work limited to the Deck Editor.

Any deliberate deviation must be documented as a spec revision with its reason and affected acceptance criteria. Do not silently reinterpret the prototype or weaken the split drag/scroll hitbox requirement.


## Deck panel tab behavior (Step 3 implementation)

The native deck panel displays one category at a time through three tabs: Main, Extra, and Side. The card viewport uses a five-column grid and shows at most two rows (10 cards) at once. Additional rows are traversed vertically. Each tab retains its own vertical row offset so changing tabs does not reset another deck's position.

Step 3 adds viewport rendering, tab selection, per-tab scroll state, card hit-testing for the visible rows, and mouse-wheel scrolling. Mobile touch-drag scrolling and gesture arbitration with card dragging remain Step 4 work and must be completed before treating touch scrolling as validated.
