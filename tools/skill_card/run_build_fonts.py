#!/usr/bin/env python3
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.skill_card.build_skill_card_fonts import NAME, COMPACT, main as build_main

build_main()

text = Path("src/data/move_database.c").read_text(encoding="utf-8", errors="replace")
names = re.findall(r'^\s+"([^"]+)"\s*,\s*$', text, re.M)
chars = set()
for n in names:
    if len(n) < 40 and not n.endswith("."):
        chars.update(n)
missing = [c for c in sorted(chars) if c not in NAME]
print("missing name glyphs:", missing)
print("name count sample:", len(names))
