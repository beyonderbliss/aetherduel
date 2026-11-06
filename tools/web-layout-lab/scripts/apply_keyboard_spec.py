#!/usr/bin/env python3
"""Generate the Android keyboard row definitions from the shared lab JSON."""
import json
import re
import sys
from pathlib import Path

spec_path = Path(sys.argv[1])
java_path = Path(sys.argv[2])
spec = json.loads(spec_path.read_text(encoding="utf-8"))
source = java_path.read_text(encoding="utf-8")

def java_items(items):
    return ", ".join(json.dumps(item, ensure_ascii=True) for item in items)

lines = [
    "    private void rebuildKeyboard() {",
    "        keyboardRows.removeAllViews();",
]
for index, layout_name in enumerate(("letters", "symbols")):
    lines.append("        " + ("if (!symbols) {" if index == 0 else "else {"))
    for row in spec["rows"][layout_name]:
        lines.append(
            "            addSplitRow(new String[]{" + java_items(row["left"]) +
            "}, new String[]{" + java_items(row["right"]) + "});"
        )
    lines.append("        }")
lines.append("    }")
method = "\n".join(lines)
pattern = r"    private void rebuildKeyboard\(\) \{.*?\n    \}\n\n    private void addSplitRow"
updated, count = re.subn(
    pattern,
    lambda _: method + "\n\n    private void addSplitRow",
    source,
    count=1,
    flags=re.S,
)
if count != 1:
    raise SystemExit("Could not uniquely replace TextEntry keyboard rows")
java_path.write_text(updated, encoding="utf-8")
print("Generated TextEntry key rows from the shared keyboard layout JSON.")
