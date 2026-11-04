#!/usr/bin/env python3
"""Prepare real Dark-name cards and their thumbnails for the Android test build."""
from __future__ import annotations

import sqlite3
import sys
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

if len(sys.argv) != 2:
    raise SystemExit("usage: prepare-dark-test-cards.py <assets-defaults-directory>")

assets = Path(sys.argv[1])
database = assets / "cards.cdb"
pics = assets / "pics"
pics.mkdir(parents=True, exist_ok=True)

if not database.is_file():
    raise SystemExit(f"Missing official card database: {database}")

with sqlite3.connect(f"file:{database}?mode=ro", uri=True) as db:
    rows = db.execute(
        """
        SELECT d.id, t.name
        FROM datas AS d
        JOIN texts AS t ON t.id = d.id
        WHERE instr(lower(t.name), 'dark') > 0
        ORDER BY t.name COLLATE NOCASE, d.id
        """
    ).fetchall()

if len(rows) < 40:
    raise SystemExit(f"Official database contains only {len(rows)} cards with 'Dark' in the name")

selected = []
for code, name in rows:
    destination = pics / f"{code}.jpg"
    if destination.is_file() and destination.stat().st_size > 1024:
        selected.append((code, name))
    else:
        request = Request(
            f"https://images.ygoprodeck.com/images/cards/{code}.jpg",
            headers={"User-Agent": "AetherDuel-DeckEditor-Test/1.0"},
        )
        try:
            with urlopen(request, timeout=20) as response:
                image_data = response.read()
            if len(image_data) < 1024 or not image_data.startswith(b"\xff\xd8"):
                raise ValueError("response is not a valid JPEG")
            destination.write_bytes(image_data)
            selected.append((code, name))
            time.sleep(0.12)
        except (HTTPError, URLError, TimeoutError, ValueError) as error:
            print(f"Skipping card {code} ({name}): {error}", file=sys.stderr)
            destination.unlink(missing_ok=True)

    # Keep going through every matching card: the full CDB remains searchable,
    # so every Dark-name result should have its corresponding local artwork.

if len(selected) < 40:
    raise SystemExit(
        f"Only {len(selected)} cards with downloadable artwork found; 40 are required"
    )

manifest = assets / "dark-test-cards.txt"
manifest.write_text(
    "# Real cards from Project Ignis BabelCDB; matching artwork stored in pics/\n"
    + "".join(f"{code}\t{name}\n" for code, name in selected),
    encoding="utf-8",
)
print(f"Prepared {len(selected)} real Dark-name cards and thumbnails:")
for code, name in selected:
    print(f"  {code}: {name}")
