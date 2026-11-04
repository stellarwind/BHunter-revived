"""Check parked real vehicles and native target/suspect HUD ownership."""

import json
import os
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1]
runtime = root / "harness_tools/output/portability-regression/runtime"
env = dict(os.environ)
env["BH_REGRESSION_MANUAL"] = "1"
env["BH_REGRESSION_STATIC_CHECK"] = "1"
env.pop("BH_REGRESSION_VISIBLE", None)
env.pop("BH_REGRESSION_DROP_KEYUP", None)
result = subprocess.run(
    [str(runtime / "BHunter-regression.exe"), "--profile"],
    cwd=runtime,
    env=env,
    timeout=30,
    check=False,
)
summary = json.loads((runtime / "regression-summary.json").read_text())
events = [
    json.loads(line)
    for line in (runtime / "regression-events.jsonl").read_text().splitlines()
]
summary.update(
    process_exit=result.returncode,
    failed_checks=[row["check"] for row in events if row.get("passed") is False],
)
summary["passed"] = (
    result.returncode == 0
    and summary["completed"]
    and summary["ticks"] == 120
    and summary["failures"] == 0
)
(root / "harness_tools/output/portability-step2/static-vehicles.json").write_text(
    json.dumps(summary, indent=2) + "\n"
)
print(json.dumps(summary, indent=2))
raise SystemExit(not summary["passed"])
