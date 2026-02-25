#!/usr/bin/env python3
"""
Generate TOML dataset blocks with N writers and M fields per writer and save it to config.toml.

Pattern generated (per writer):
[[publisher.writerGroup.dataSets]]
writerId = <id>
name = "writer_<id>"
fields = [
  { name = "<id>_0", type = "Long" },
  { name = "<id>_1", type = "Real" },
]

Usage:
  python gen_datasets_toml.py
  python gen_datasets_toml.py --writers 10 --fields 2
  python gen_datasets_toml.py --writers 10 --fields 4 --types Long Real Real Long
"""

from __future__ import annotations

import argparse
from pathlib import Path


def generate(
    writer_count: int = 10,
    fields_per_writer: int = 2,
    field_type: str | None = None,
    writer_id_start: int = 0,
    name_prefix: str = "writer_",
) -> str:
    """
    field_types:
      - If provided, types are assigned by index and cycled if fields_per_writer > len(field_types)
      - If omitted, defaults to ["Long", "Real"] (cycled)
    """
    if not field_type:
        field_type =  "Real"

    lines: list[str] = []

    for i in range(writer_count):
        writer_id = writer_id_start + i
        lines.append("[[publisher.writerGroup.dataSets]]")
        lines.append(f"writerId = {writer_id}")
        lines.append(f'name = "{name_prefix}{writer_id}"')
        lines.append("fields = [")

        for f in range(fields_per_writer):
            ftype = field_type
            lines.append(f'  {{ name = "{writer_id}_{f}", type = "{ftype}" }},')

        lines.append("]")
        lines.append("")  # blank line between writers

    return "\n".join(lines).rstrip() + "\n"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, default=Path("config.toml"), help="output file path")
    ap.add_argument("--writers", type=int, default=10, help="number of writers/datasets to generate")
    ap.add_argument("--fields", type=int, default=2, help="number of fields per writer")
    ap.add_argument("--type", nargs="*", default="Real", help='field types (cycled), e.g. --types Real')
    ap.add_argument("--writer-id-start", type=int, default=0, help="starting writerId")
    ap.add_argument("--name-prefix", type=str, default="writer_", help='dataset name prefix, default "writer_"')
    args = ap.parse_args()

    toml_text = generate(
        writer_count=args.writers,
        fields_per_writer=args.fields,
        field_type=args.type,
        writer_id_start=args.writer_id_start,
        name_prefix=args.name_prefix,
    )

    args.out.write_text(toml_text, encoding="utf-8")
    print(f"Wrote {args.out} ({len(toml_text)} bytes)")


if __name__ == "__main__":
    main()