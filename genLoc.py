import csv, os

LANGS = ["EN", "DE", "ZH"]
CSV_FILE = "txt.csv"
OUT_FILE = "txt.h"

rows = list(csv.DictReader(open(CSV_FILE, encoding="utf-8")))

out = []

out.append("#pragma once")
out.append("// AUTO GENERATED - DO NOT EDIT")
out.append("// Generated from: " + CSV_FILE + "\n")

# ----- enum of IDs -----
out.append("typedef enum {")
for r in rows:
    out.append(f"\t{'TXT_' + r['ID']},")
out.append("\tTXT_COUNT")
out.append("} TextID;\n")

# ----- language enum -----
out.append("typedef enum {")
for L in LANGS:
    out.append(f"\tLANG_{L.upper()},")
out.append("\tLANG_COUNT")
out.append("} Language;\n")

# ----- table -----
out.append("static const char *gText[LANG_COUNT][TXT_COUNT] = {")

for lang in LANGS:
    out.append("  {")
    for r in rows:
        s = r[lang] \
            .replace('"', '\\"') \
            .replace("\r", "") \
            .replace("\n", "\\n")
        out.append(f'\t"{s}",')
    out.append("  },")        # ← this now goes into the file

out.append("};\n")

# ----- WRITE TO FILE -----
with open(OUT_FILE, "w", encoding="utf-8") as f:
    f.write("\n".join(out))

print(f"Generated: {OUT_FILE}  ({len(rows)} entries, {len(LANGS)} languages)")

