# Mobile Deck Editor — temporary test fixture

The Deck Editor test fixture is opt-in and compile-time gated by `AETHERDUEL_DECK_EDITOR_TEST_FIXTURE`.

## Enable

Run the existing **AetherDuel Android APK (Startup-ready)** workflow manually on branch `feature/mobile-deck-editor` and set the `deck_editor_test_fixture` input to `true`. The default is `false`. This patch does not trigger a workflow or build.

## Fixture contents

When enabled, entering Deck Editor with a fresh initialization seeds:
- Main Deck: 20 entries
- Extra Deck: 15 entries
- Side Deck: 15 entries
- Search results: up to 30 in-memory entries when the sole matching card is Dark Magician Girl (`38033121`)

The three deck vectors reuse the existing Dark Magician Girl card record and artwork. Search-only duplicates are held in a reserved in-memory vector. No additional card records or artwork are written to `cards.cdb` or the repository assets.

The fixture is intentionally visual-only. The compile-time test branch permits the Dark Magician Girl fixture card to be repositioned in the Extra tab, even though it is not a real Extra Deck card. Do not use this fixture for duel legality, deck validation, saving a real deck, or gameplay.

## Disable

Use the default `false` input for a normal build. No persistent database cleanup is needed because all extra entries exist only in memory for that test APK.
