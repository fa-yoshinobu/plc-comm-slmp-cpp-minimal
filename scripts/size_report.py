#!/usr/bin/env python3
"""Run PlatformIO size probes and compare them against a committed baseline."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


RAM_RE = re.compile(r"RAM:\s+\[[^\]]+\]\s+([\d.]+)% \(used (\d+) bytes from (\d+) bytes\)")
FLASH_RE = re.compile(r"Flash:\s+\[[^\]]+\]\s+([\d.]+)% \(used (\d+) bytes from (\d+) bytes\)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--project-dir", type=Path, required=True)
    parser.add_argument("--baseline", type=Path, required=True)
    parser.add_argument("--pio", default="pio")
    parser.add_argument("--markdown-out", type=Path, required=True)
    parser.add_argument("--json-out", type=Path, required=True)
    return parser.parse_args()


def run_env(project_dir: Path, pio: str, env: str) -> dict[str, object]:
    cmd = [pio, "run", "-e", env]
    proc = subprocess.run(
        cmd,
        cwd=project_dir,
        check=False,
        capture_output=True,
        text=True,
    )
    output = proc.stdout + proc.stderr
    ram_match = RAM_RE.search(output)
    flash_match = FLASH_RE.search(output)
    if proc.returncode != 0 or ram_match is None or flash_match is None:
        raise RuntimeError(f"failed to collect size for {env}\n{output}")

    ram_percent, ram_used, ram_total = ram_match.groups()
    flash_percent, flash_used, flash_total = flash_match.groups()
    return {
        "env": env,
        "ram_percent": float(ram_percent),
        "ram_used": int(ram_used),
        "ram_total": int(ram_total),
        "flash_percent": float(flash_percent),
        "flash_used": int(flash_used),
        "flash_total": int(flash_total),
        "raw_output": output,
    }


def make_markdown(results: list[dict[str, object]]) -> str:
    lines = [
        "# Size Report",
        "",
        "| Probe env | Label | Flash used | Flash delta | RAM used | RAM delta | Status |",
        "|---|---|---:|---:|---:|---:|---|",
    ]
    for item in results:
        lines.append(
            "| {env} | {label} | `{flash_used}` | `{flash_delta:+}` | `{ram_used}` | `{ram_delta:+}` | {status} |".format(
                env=item["env"],
                label=item["label"],
                flash_used=item["flash_used"],
                flash_delta=item["flash_delta"],
                ram_used=item["ram_used"],
                ram_delta=item["ram_delta"],
                status=item["status"],
            )
        )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    args = parse_args()
    baseline_data = json.loads(args.baseline.read_text(encoding="utf-8"))
    baseline_envs: dict[str, dict[str, object]] = baseline_data["envs"]

    results: list[dict[str, object]] = []
    has_regression = False

    for env, baseline in baseline_envs.items():
        actual = run_env(args.project_dir, args.pio, env)
        flash_delta = int(actual["flash_used"]) - int(baseline["flash_used"])
        ram_delta = int(actual["ram_used"]) - int(baseline["ram_used"])
        flash_limit = int(baseline["allowed_flash_growth"])
        ram_limit = int(baseline["allowed_ram_growth"])
        status = "pass"
        if flash_delta > flash_limit or ram_delta > ram_limit:
            status = "fail"
            has_regression = True

        results.append(
            {
                "env": env,
                "label": baseline["label"],
                "flash_used": actual["flash_used"],
                "flash_total": actual["flash_total"],
                "flash_percent": actual["flash_percent"],
                "flash_delta": flash_delta,
                "flash_limit": flash_limit,
                "ram_used": actual["ram_used"],
                "ram_total": actual["ram_total"],
                "ram_percent": actual["ram_percent"],
                "ram_delta": ram_delta,
                "ram_limit": ram_limit,
                "status": status,
            }
        )

    args.markdown_out.write_text(make_markdown(results), encoding="utf-8")
    args.json_out.write_text(json.dumps({"results": results}, indent=2), encoding="utf-8")
    return 1 if has_regression else 0


if __name__ == "__main__":
    sys.exit(main())
