#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse
from posixpath import dirname
from sys import stdout, stderr, path, stdin
from threading import Thread, Event

path.append(dirname(__file__))

from lib.expect import Expect
from lib.driver import Driver
from lib.motor import Motor

parser = argparse.ArgumentParser(description="Motor Demo")
parser.add_argument(
    "-B", "--baud", type=int, default=115200, help="Baud rate for serial communication"
)
parser.add_argument(
    "-v",
    "--verbose",
    action="count",
    default=0,
    help="Increase verbosity level",
)
parser.add_argument(
    "-p",
    "--plot",
    action="store_true",
    help="Enable plotting of stall guard results",
)
args = parser.parse_args()
baud = int(args.baud)
verbose = int(args.verbose)
should_plot = bool(args.plot)

driver = Driver(baudrate=baud, verbose=verbose)
print(f"# Device Info: {driver.getInfo()}", file=stderr)
kw = dict(
    init_pos=0,
    rms_current=2000,
    microsteps=16,
    stall_sensitivity=10,
)
m0 = Motor(driver, id=0, invert=1, scale=6.0 / 360.0, **kw)
m1 = Motor(driver, id=1, invert=0, scale=6.0 / 360.0, **kw)

stop_event = Event()


def command_thread():
    """Thread to read commands from stdin."""
    t0: float = m0.getPosition()  # target position in degrees
    t1: float = m1.getPosition()  # target position in degrees
    s0: float = 20.0  # degrees per second
    s1: float = 20.0  # degrees per second
    while not stop_event.is_set():
        try:
            line = stdin.readline().strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) == 0:
                continue
            cmd = parts[0].lower()
            if cmd == "move" and len(parts) == 3:
                try:
                    t0, t1 = map(float, parts[1:3])
                    m0.move(t0, speed=s0)
                    m1.move(t1, speed=s1)
                except ValueError as e:
                    print(f"Error: Invalid position values - {e}", file=stderr)
                except Exception as e:
                    print(f"Error moving motors: {e}", file=stderr)
            elif cmd == "speed" and len(parts) == 3:
                try:
                    s0, s1 = map(float, parts[1:3])
                    m0.move(t0, speed=s0)
                    m1.move(t1, speed=s1)
                except ValueError as e:
                    print(f"Error: Invalid speed values - {e}", file=stderr)
            else:
                if cmd not in ("help", "move", "speed"):
                    print(f"Unknown command: {line}", file=stderr)
                print("Available commands:", file=stderr)
                print("  help          - Show this help message", file=stderr)
                print("  move <x> <y>  - Move motors to positions", file=stderr)
                print("  speed <x> <y> - Set motor speeds", file=stderr)
        except EOFError:
            break
        except KeyboardInterrupt:
            break


def monitor_thread():
    """Thread to monitor and print incoming packets."""
    for packet in driver.rx():
        print(packet)
        if stop_event.is_set():
            break


try:
    with driver.enable(), m0.enable(), m1.enable():
        # Start the command thread
        cmd_thread = Thread(target=command_thread, daemon=True)
        cmd_thread.start()

        # Start the monitor thread
        mon_thread = Thread(target=monitor_thread, daemon=True)
        mon_thread.start()

        # Wait for threads
        cmd_thread.join()
        mon_thread.join()

except KeyboardInterrupt:
    pass
finally:
    stop_event.set()

driver.close()
