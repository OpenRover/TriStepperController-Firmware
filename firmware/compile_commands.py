#!/usr/bin/env python3
from pathlib import Path
from argparse import ArgumentParser

parser = ArgumentParser(description="Generate compile_commands.json for the project.")

parser.add_argument(
    "--env",
    type=str,
    default=None,
    help="PlatformIO environment to use (e.g., main). If not specified, "
    "the first environment found in platformio.ini will be used.",
)

args = parser.parse_args()
env = args.env

CWD = Path(__file__).parent.resolve()

# Execute pio run -t compiledb
import subprocess

cmd = ["pio", "run", "-t", "compiledb"]
if env is not None:
    cmd += ["-e", env]
subprocess.run(cmd, cwd=CWD).check_returncode()

# Transform compile_commands for clangd
import json

with open(CWD / "compile_commands.json", "r") as f:
    db = json.load(f)

for item in db:
    #     Unknown argument '-mlongcalls'; did you mean '-mlong-calls'?clang(drv_unknown_argument_with_suggestion)
    # Unknown argument: '-fstrict-volatile-bitfields'clang(drv_unknown_argument)
    # Unknown argument: '-fno-tree-switch-conversion'clang(drv_unknown_argument)
    command = item["command"]
    if type(command) is str:
        command = command.replace("-mlongcalls", "")
        command = command.replace("-fstrict-volatile-bitfields", "")
        command = command.replace("-fno-tree-switch-conversion", "")
        item["command"] = command

with open(CWD / "compile_commands.json", "w") as f:
    json.dump(db, f, indent=2)
