# PrimaNyx UI Foundation

Status: initial design contract  
Target branch: `feature/universal-edge-margin-5`

## Product principle

PrimaNyx is an application-level UI identity, not merely a selectable EDOPro skin.

The application owns the persistent component geometry and interaction character. Themes may change colors, surface treatments, borders, and state colors, but changing a theme must not make the application revert to the default EDOPro component shapes.

## Reference: approved Android dropdown

The existing native Android dropdown is the first approved PrimaNyx component and is the visual reference for rounded surfaces.

Confirmed geometry from `.github/workflows/android-build.yml`:

| Part | Corner radius |
| --- | ---: |
| Dropdown panel | 14 dp |
| Dropdown item | 9 dp |

These are confirmed values, not estimates. The dropdown implementation is currently native Android code using `GradientDrawable`; it is a design reference, not yet a shared cross-platform renderer.

## UI geometry policy

- Rounded geometry belongs to the PrimaNyx application UI layer.
- Theme/skin configuration may provide appearance values, but must not be the only mechanism that enables the rounded component geometry.
- Do not change existing component bounds, dimensions, spacing, hitboxes, focus behavior, or events as part of the visual migration.
- Preserve normal, pressed, focused, disabled, and selected states.
- **Corner radius is contextual, not global.** It must be chosen for each component category and visual role, taking the component's dimensions and proportions into account.
- Do not apply one fixed radius to every component, and do not derive every component's radius from the dropdown panel.
- Use semantic/category-based radius tokens where shared values are appropriate. Components with different roles or scales may use different tokens; sharing a token is a deliberate design decision, not a default.
- A radius must remain visually appropriate for the actual rendered dimensions. Small controls must not inherit a radius that makes them look excessively pill-shaped or collapses their usable shape.
- Confirmed reference values are: dropdown panel **14 dp**, dropdown item **9 dp**, and menu panel **8 dp**.
- The values above apply only to those respective components. Other controls' radii remain **to be measured or approved**; do not invent values before inspecting their actual dimensions and appearance.

## Initial component scope

The rounded UI migration is intended to cover:

1. Standard buttons and image buttons.
2. Checkbox controls.
3. Text input/edit fields.
4. ComboBox field and arrow control.
5. Tab buttons and other navigation buttons.
6. Other interactive controls after their rendering paths are identified.

The Android dropdown and V2 keyboard already have rounded native backgrounds and should remain unchanged unless a later consistency pass requires it.

## Renderer notes

The current C++ skin renderer is in `gframe/CGUISkinSystem/CImageGUISkin.cpp`.

- Standard and pressed button panes are routed through `draw3DButtonPaneStandard()` and `draw3DButtonPanePressed()`.
- The existing `drawElementStyle()` helper implements nine-slice texture rendering.
- Skin texture styles are loaded by `CGUISkinSystem::ParseGUIElementStyle()` in `gframe/CGUISkinSystem/CGUISkinSystem.cpp`.
- The current `skin/PrimaNyx/skin.xml` does not define button textures, so the button path falls back to the Irrlicht skin renderer.

This is the initial implementation entry point, not permission to make the rounded identity dependent on `skin/PrimaNyx/skin.xml`.

## Next implementation step

Implement the first application-level rounded button renderer while preserving the existing button API, layout, hitboxes, and events. Determine the button radius from the button's actual dimensions and visual role; do not automatically reuse the dropdown or menu radius. Confirm the button's final radius and visual states before applying the same geometry system to other controls.
