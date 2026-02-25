#!/usr/bin/env python3
"""
Generate a TOML PubSub config with N writerGroups and save it to config.toml.

- writerGroup ids increment by 1 starting at wg_id_start
- intervals increment by interval_step_ms starting at interval_start_ms
- each writerGroup has exactly 1 dataset
- dataset writerIds increment by 1 starting at writer_id_start
- dataset/field names are unique per writerId

Usage:
  python gen_pubsub_toml.py
  python gen_pubsub_toml.py --n 100 --interval-start 500 --step 5
"""

from __future__ import annotations
import argparse
from pathlib import Path


def generate(
    n: int = 100,
    publisher_name: str = "PLC0",
    publisher_id: int = 10000,
    wg_id_start: int = 103,
    writer_id_start: int = 108,
    interval_start_ms: int = 500,
    interval_step_ms: int = 5,
    fields_per_dataset: int = 10,
    wg_name_prefix: str = "Processors / CPU - WG",
    dataset_name_prefix: str = "core_",
    field_type: str = "Float",
) -> str:
    lines: list[str] = []
    lines.append("[[publishers]]")
    lines.append(f'name = "{publisher_name}"')
    lines.append(f"publisherId = {publisher_id}")
    lines.append("")

    for idx in range(n):
        wg_id = wg_id_start + idx
        interval = interval_start_ms + idx * interval_step_ms
        writer_id = writer_id_start + idx

        wg_name = f"{wg_name_prefix}{wg_id}"
        ds_name = f"{dataset_name_prefix}{writer_id}"

        lines.append("[[publishers.writerGroups]]")
        lines.append(f"id = {wg_id}")
        lines.append(f"interval = {interval}")
        lines.append(f'name = "{wg_name}"')
        lines.append("")
        lines.append("[[publishers.writerGroups.dataSets]]")
        lines.append(f"writerId = {writer_id}")
        lines.append(f'name = "{ds_name}"')
        lines.append("fields = [")

        for f in range(1, fields_per_dataset + 1):
            field_name = f"{ds_name}_{f}"
            lines.append(f'  {{ name = "{field_name}", type = "{field_type}" }},')

        lines.append("]")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, default=Path("config.toml"), help="output file path")
    ap.add_argument("--n", type=int, default=100, help="number of writerGroups")
    ap.add_argument("--publisher-name", type=str, default="PLC0")
    ap.add_argument("--publisher-id", type=int, default=10000)
    ap.add_argument("--wg-id-start", type=int, default=100)
    ap.add_argument("--writer-id-start", type=int, default=108)
    ap.add_argument("--interval-start", type=int, default=500, help="ms")
    ap.add_argument("--step", type=int, default=10, help="interval step in ms")
    ap.add_argument("--fields", type=int, default=10, help="fields per dataset")
    ap.add_argument("--wg-name-prefix", type=str, default="Group_")
    ap.add_argument("--dataset-prefix", type=str, default="")
    ap.add_argument("--type", type=str, default="Float", help='field type string, e.g. "Float"')
    args = ap.parse_args()

    toml_text = generate(
        n=args.n,
        publisher_name=args.publisher_name,
        publisher_id=args.publisher_id,
        wg_id_start=args.wg_id_start,
        writer_id_start=args.writer_id_start,
        interval_start_ms=args.interval_start,
        interval_step_ms=args.step,
        fields_per_dataset=args.fields,
        wg_name_prefix=args.wg_name_prefix,
        dataset_name_prefix=args.dataset_prefix,
        field_type=args.type,
    )

    args.out.write_text(toml_text, encoding="utf-8")
    print(f"Wrote {args.out} ({len(toml_text)} bytes)")


if __name__ == "__main__":
    main()