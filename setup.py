#!/usr/bin/env python3
"""Cross-platform project bootstrapper for CollisionAvoidance."""
from __future__ import annotations

import argparse
import importlib
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
VENV = ROOT / ".venv"
ENVIRONMENTS = ("test", "espnow", "vehicle", "monitor")
REQUIRED_MODULES = ("serial", "serial.tools.list_ports", "numpy", "pandas", "scipy", "matplotlib", "matplotlib.backends.backend_tkagg", "tkinter")


def run(command: list[str], *, check: bool = True, env: dict | None = None) -> subprocess.CompletedProcess:
    print("\n$ " + " ".join(map(str, command)), flush=True)
    return subprocess.run(command, cwd=ROOT, env=env, check=check)


def venv_python() -> Path:
    return VENV / ("Scripts/python.exe" if os.name == "nt" else "bin/python")


def python_ok(executable: Path) -> bool:
    try:
        result = subprocess.run([str(executable), "-c", "import sys; print(sys.version_info >= (3,11))"],
                               capture_output=True, text=True, check=True)
        return result.stdout.strip() == "True"
    except (OSError, subprocess.SubprocessError):
        return False


def find_pio() -> str | None:
    candidates = []
    if os.name == "nt":
        candidates.append(Path.home() / ".platformio/penv/Scripts/pio.exe")
    else:
        candidates.append(Path.home() / ".platformio/penv/bin/pio")
    for path in candidates:
        if path.is_file():
            return str(path)
    return shutil.which("pio")


def check_imports() -> bool:
    failures = []
    for module in REQUIRED_MODULES:
        try:
            importlib.import_module(module)
            print(f"  OK: {module}")
        except Exception as exc:
            failures.append((module, exc))
            print(f"  NG: {module}: {exc}")
    if failures:
        if any(name.startswith("tkinter") or name.endswith("backend_tkagg") for name, _ in failures) and sys.platform.startswith("linux"):
            print("Tk support may be missing. Install it with: sudo apt install python3-tk")
        return False
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Set up Python dependencies and build PlatformIO environments.")
    parser.add_argument("--force", action="store_true", help="Recreate the Python virtual environment.")
    parser.add_argument("--skip-pio", action="store_true", help="Skip PlatformIO builds.")
    parser.add_argument("--rebuild", action="store_true", help="Build all PlatformIO environments even if already built.")
    args = parser.parse_args()

    status: dict[str, bool] = {}
    print("=" * 52)
    print(" CollisionAvoidance setup")
    print("=" * 52)

    current = venv_python()
    if args.force or (VENV.exists() and not python_ok(current)):
        print("[1/4] Recreating virtual environment")
        shutil.rmtree(VENV, ignore_errors=True)
    if not VENV.exists():
        print("[1/4] Creating virtual environment")
        try:
            run([sys.executable, "-m", "venv", str(VENV)])
        except subprocess.CalledProcessError:
            print("Failed to create venv. Install Python 3.11+ and its venv package.")
            return 1
    else:
        print("[1/4] Virtual environment already exists (Python 3.11+).")

    if not python_ok(current):
        print("ERROR: The venv Python must be 3.11 or newer.")
        print("Install Python 3.11+ and rerun setup, or use --force to recreate the venv.")
        return 1

    print("[2/4] Installing Python packages")
    try:
        run([str(current), "-m", "pip", "install", "--upgrade", "pip"])
        run([str(current), "-m", "pip", "install", "-r", str(ROOT / "requirements.txt")])
    except subprocess.CalledProcessError:
        status["Python dependencies"] = False
        print("Python package installation failed.")
        print("See the error above; check network access and requirements.txt.")
    else:
        status["Python dependencies"] = check_imports()

    print("[3/4] Checking PlatformIO")
    pio = find_pio()
    if args.skip_pio:
        status["PlatformIO builds"] = True
        print("PlatformIO build skipped (--skip-pio).")
    elif not pio:
        status["PlatformIO builds"] = False
        print("PlatformIO Core was not found.")
        print("Install the PlatformIO IDE extension in VS Code, then rerun setup.")
    else:
        print(f"PlatformIO: {pio}")
        builds_ok = True
        for env_name in ENVIRONMENTS:
            build_dir = ROOT / ".pio" / "build" / env_name
            if (build_dir / "firmware.bin").is_file() and not args.rebuild:
                print(f"  SKIP {env_name}: firmware.bin exists (use --rebuild to force)")
                continue
            print(f"  BUILD {env_name}")
            try:
                run([pio, "run", "-e", env_name])
            except subprocess.CalledProcessError:
                builds_ok = False
                print(f"  FAILED: {env_name}")
        status["PlatformIO builds"] = builds_ok

    print("[4/4] Platform checks")
    if sys.platform.startswith("linux"):
        groups = subprocess.run(["groups"], capture_output=True, text=True)
        if "dialout" not in groups.stdout.split():
            print("NOTICE: Current user is not in the dialout group.")
            print("For USB serial access, run: sudo usermod -aG dialout $USER")
            print("Then log out and back in.")
        else:
            print("USB serial group (dialout): OK")
    status["Platform checks"] = True

    print("\n" + "=" * 52)
    print(" Setup summary")
    print("=" * 52)
    for label, ok in status.items():
        print(f"{'OK' if ok else 'NG'}  {label}")
    if not all(status.values()):
        print("\nSetup finished with errors. Resolve the NG items and rerun setup.")
        return 1
    print("\nSetup completed.")
    print("Use the virtual environment Python directly:")
    print(f"  {current}")
    if os.name == "nt":
        print("Activate with: .\\.venv\\Scripts\\activate")
    else:
        print("Activate with: source .venv/bin/activate")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
