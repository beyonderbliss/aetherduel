#!/usr/bin/env python3
"""Build a 40-card mini CDB and download only its matching Dark-name artwork."""
from __future__ import annotations

import shutil
import sqlite3
import sys
import time
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen

TARGET_COUNT = 40

if len(sys.argv) != 3:
    raise SystemExit("usage: prepare-dark-test-cards.py <source-cards.cdb> <assets-defaults-directory>")

source_database = Path(sys.argv[1])
assets = Path(sys.argv[2])
mini_database = assets / "cards.cdb"
pics = assets / "pics"
manifest = assets / "dark-test-cards.txt"

if not source_database.is_file() or source_database.stat().st_size == 0:
    raise SystemExit(f"Missing source card database: {source_database}")

# This directory is dedicated to card artwork. Start clean so an earlier,
# larger dataset cannot leak into the APK.
shutil.rmtree(pics, ignore_errors=True)
pics.mkdir(parents=True, exist_ok=True)
mini_database.unlink(missing_ok=True)
manifest.unlink(missing_ok=True)

with sqlite3.connect(f"file:{source_database}?mode=ro", uri=True) as source:
    tables = {
        row[0] for row in source.execute(
            "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'"
        )
    }
    expected_tables = {"datas", "texts"}
    if tables != expected_tables:
        raise SystemExit(
            f"Unsupported CDB schema; expected only datas and texts, found: {sorted(tables)}"
        )

    table_ddl = {}
    for table in ("datas", "texts"):
        result = source.execute(
            "SELECT sql FROM sqlite_master WHERE type='table' AND name=?",
            (table,),
        ).fetchone()
        if not result or not result[0]:
            raise SystemExit(f"Missing CREATE TABLE definition for {table}")
        table_ddl[table] = result[0]

    index_ddl = [
        row[0] for row in source.execute(
            "SELECT sql FROM sqlite_master WHERE type='index' "
            "AND tbl_name IN ('datas', 'texts') AND sql IS NOT NULL"
        )
    ]

    rows = source.execute(
        """
        SELECT d.id, t.name
        FROM datas AS d
        JOIN texts AS t ON t.id = d.id
        WHERE instr(lower(t.name), 'dark') > 0
        ORDER BY t.name COLLATE NOCASE, d.id
        """
    ).fetchall()

if len(rows) < TARGET_COUNT:
    raise SystemExit(
        f"Source database contains only {len(rows)} cards with 'Dark' in the name"
    )

# Keep trying matching cards until exactly 40 valid artwork files are found.
# This handles cards absent from the image CDN without adding unrelated cards.
selected = []
for code, name in rows:
    destination = pics / f"{code}.jpg"
    request = Request(
        f"https://images.ygoprodeck.com/images/cards/{code}.jpg",
        headers={"User-Agent": "AetherDuel-DeckEditor-Test/1.0"},
    )
    try:
        with urlopen(request, timeout=25) as response:
            image_data = response.read()
        if len(image_data) < 1024 or not image_data.startswith(b"\xff\xd8"):
            raise ValueError("response is not a valid JPEG")
        destination.write_bytes(image_data)
        selected.append((code, name))
        print(f"Artwork {len(selected)}/{TARGET_COUNT}: {code} — {name}")
        if len(selected) == TARGET_COUNT:
            break
        time.sleep(0.12)
    except (HTTPError, URLError, TimeoutError, ValueError) as error:
        print(f"Skipping card {code} ({name}): {error}", file=sys.stderr)
        destination.unlink(missing_ok=True)

if len(selected) != TARGET_COUNT:
    shutil.rmtree(pics, ignore_errors=True)
    raise SystemExit(
        f"Only {len(selected)} matching cards had downloadable artwork; "
        f"exactly {TARGET_COUNT} are required"
    )

card_ids = [code for code, _ in selected]
placeholders = ",".join("?" for _ in card_ids)

# Create the standard EDOPro tables with the source's exact definitions, then
# copy only the selected IDs. The full source CDB is never placed in assets.
with sqlite3.connect(mini_database) as mini:
    for table in ("datas", "texts"):
        mini.execute(table_ddl[table])

    mini.execute("ATTACH DATABASE ? AS full_source", (str(source_database),))
    for table in ("datas", "texts"):
        mini.execute(
            f"INSERT INTO {table} SELECT * FROM full_source.{table} "
            f"WHERE id IN ({placeholders})",
            card_ids,
        )
    for statement in index_ddl:
        mini.execute(statement)
    mini.commit()

    for table in ("datas", "texts"):
        count = mini.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0]
        if count != TARGET_COUNT:
            raise SystemExit(f"Mini CDB validation failed: {table} contains {count} rows")

    mini_ids = {row[0] for row in mini.execute("SELECT id FROM datas")}
    text_ids = {row[0] for row in mini.execute("SELECT id FROM texts")}
    if mini_ids != set(card_ids) or text_ids != set(card_ids):
        raise SystemExit("Mini CDB validation failed: card IDs do not match the 40 downloaded artworks")

manifest.write_text(
    "# Exactly 40 real Dark-name cards; matching artwork stored in pics/\n"
    + "".join(f"{code}\t{name}\n" for code, name in selected),
    encoding="utf-8",
)

image_count = len(list(pics.glob("*.jpg")))
if image_count != TARGET_COUNT:
    raise SystemExit(f"Artwork validation failed: expected {TARGET_COUNT}, found {image_count}")
print(f"Created mini cards.cdb with {TARGET_COUNT} cards and exactly {image_count} matching artworks.")
