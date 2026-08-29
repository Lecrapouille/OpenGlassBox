#!/usr/bin/env python3
"""Convert a CiudadSim Scilab .net (chisincen) file to OpenGlassBox .ogc."""

from __future__ import annotations

import argparse
import hashlib
import struct
import sys
from pathlib import Path


def read_double_matrix(data: bytes, pos: int) -> list[float]:
    if struct.unpack_from("<i", data, pos)[0] != 1:
        raise ValueError(f"expected double matrix at {pos}")
    rows = struct.unpack_from("<i", data, pos + 4)[0]
    cols = struct.unpack_from("<i", data, pos + 8)[0]
    start = pos + 16
    return [
        struct.unpack_from("<d", data, start + 8 * i)[0] for i in range(rows * cols)
    ]


def find_matrices(data: bytes, cols: int) -> list[tuple[int, list[float]]]:
    found: list[tuple[int, list[float]]] = []
    for pos in range(0, len(data) - 20, 4):
        if struct.unpack_from("<i", data, pos)[0] != 1:
            continue
        rows = struct.unpack_from("<i", data, pos + 4)[0]
        if rows == 1 and struct.unpack_from("<i", data, pos + 8)[0] == cols:
            found.append((pos, read_double_matrix(data, pos)))
    return found


def load_chisincen(path: Path) -> tuple[list[float], list[float], list[int], list[int]]:
    data = path.read_bytes()
    nn = 546
    na = 2176

    node_mats = find_matrices(data, nn)
    link_mats = find_matrices(data, na)
    if len(node_mats) < 3:
        raise ValueError("could not locate node coordinate matrices")
    if len(link_mats) < 3:
        raise ValueError("could not locate link matrices")

    _names = node_mats[0][1]
    xs = node_mats[1][1]
    ys = node_mats[2][1]
    _link_names = link_mats[0][1]
    tails = [int(v) for v in link_mats[1][1]]
    heads = [int(v) for v in link_mats[2][1]]

    if not (350 <= min(xs) <= 900 and 350 <= max(xs) <= 900):
        raise ValueError("unexpected x coordinate range in chisincen.net")
    if not (1500 <= min(ys) <= 2300 and 1500 <= max(ys) <= 2300):
        raise ValueError("unexpected y coordinate range in chisincen.net")

    return xs, ys, tails, heads


def write_ogc(
    out_path: Path,
    xs: list[float],
    ys: list[float],
    tails: list[int],
    heads: list[int],
    *,
    city_name: str,
    ruleset: str,
    ruleset_hash: str,
) -> None:
    min_x = int(min(xs))
    min_y = int(min(ys))
    max_x = int(max(xs))
    max_y = int(max(ys))
    size_u = max_x - min_x + 1
    size_v = max_y - min_y + 1

    lines: list[str] = [
        "save",
        f"\truleset {ruleset}",
        f"\thash {ruleset_hash}",
        "\ttypes [ Road Dirt ]",
        "end",
        "",
        "clock 0",
        "",
        f"city {city_name} size {size_u} {size_v}",
        "globals [ ]",
        "path Road",
    ]

    for index, (x, y) in enumerate(zip(xs, ys), start=1):
        u = int(round(x)) - min_x
        v = int(round(y)) - min_y
        lines.append(f"\tnode {index} {u} {v}")

    for way_id, (tail, head) in enumerate(zip(tails, heads), start=1):
        lines.append(f"\tway {way_id} Dirt {tail} {head} flow 0")

    lines.extend(["end", ""])

    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(
        f"Wrote {out_path} : {len(xs)} nodes, {len(tails)} ways, "
        f"grid {size_u}x{size_v}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("net", type=Path, help="CiudadSim .net file")
    parser.add_argument("-o", "--output", type=Path, required=True)
    parser.add_argument("--city", default="Chicago")
    parser.add_argument("--ruleset", default="sandbox.ogs")
    parser.add_argument(
        "--hash",
        help="SHA-256 the save records for its ruleset. Read from the ruleset "
        "sitting next to the output when not given, which is where the loader "
        "looks for it.",
    )
    args = parser.parse_args()

    ruleset_hash = args.hash
    if ruleset_hash is None:
        ruleset_file = args.output.parent / args.ruleset
        if not ruleset_file.is_file():
            parser.error(
                f"cannot read '{ruleset_file}' to fingerprint it; "
                f"pass --hash instead"
            )
        ruleset_hash = hashlib.sha256(ruleset_file.read_bytes()).hexdigest()

    xs, ys, tails, heads = load_chisincen(args.net)
    write_ogc(
        args.output,
        xs,
        ys,
        tails,
        heads,
        city_name=args.city,
        ruleset=args.ruleset,
        ruleset_hash=ruleset_hash,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
