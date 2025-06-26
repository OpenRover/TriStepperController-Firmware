#!/usr/bin/env python3
import sys, os, subprocess, json

subprocess.run(["pio", "run", "-t", "compiledb"])

with open("compile_commands.json", "r") as f:
    compile_commands: list[dict[str, str]] = json.load(f)

# Filter out the compile commands that are not for the source files in the current directory
for item in compile_commands:
    item["command"] = (
        item["command"]
        .replace(" -mlongcalls", "")
        .replace(" -fstrict-volatile-bitfields", "")
        .replace(" -fno-tree-switch-conversion", "")
    )

# Add additional compile command from emulator/Makefile
proc = subprocess.run(
    ["make", "compile_commands"],
    capture_output=True,
    text=True,
    check=True,
    cwd=os.path.join(os.path.dirname(__file__), "..", "emulator"),
)

compile_commands.append(
    {
        "directory": os.path.join(os.path.dirname(__file__), "..", "emulator"),
        "command": proc.stdout.replace(r"\s+", " ").strip(),
        "file": "emulator.c",
        "output": "../emulator.o",
    }
)

with open("compile_commands.json", "w") as f:
    json.dump(compile_commands, f, indent=4)
