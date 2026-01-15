#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse
from time import sleep
from sys import stdout, stderr

from lib.expect import Expect
from lib.driver import Driver
from lib.motor import Motor
from lib.util import debounce, loop_for

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
print(f"Device Info: {driver.getInfo()}", file=stderr)
m0 = Motor(driver, id=0, micro_steps=32, invert=1, scale=1.0/40, init_pos=0.0, rms_current=6000)

try:
    with driver.enable(), m0.enable():
        while True:
            Expect.wait(m0.move(+100.0, speed=100))
            Expect.wait(m0.move(+300.0, speed=200))
            sleep(1.0)
            Expect.wait(m0.move(+000.0, speed=100))
            sleep(1.0)
except KeyboardInterrupt:
    pass

driver.close()
