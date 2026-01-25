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
args = parser.parse_args()
baud = int(args.baud)
verbose = int(args.verbose)

print(dict(baud=baud, verbose=verbose), file=stderr)

driver = Driver(baudrate=baud, verbose=verbose)
print(f"Device Info: {driver.getInfo()}", file=stderr)
config = dict(
    init_pos=0,
    micro_steps=32,
    stall_sensitivity=64,
    rms_current=600,
)
m0 = Motor(driver, id=0, invert=0, scale=1.0, **config)
m1 = Motor(driver, id=1, invert=0, scale=1.0, **config)


@debounce(1.0 / 20.0)
def tick():
    status = [m.getStatus() for m in (m0, m1)]
    diag = (f"m{i}.diag={s.diag_pin * 100 - 100}" for i, s in enumerate(status))
    sg_result = (f"m{i}.sg={s.sg_result}" for i, s in enumerate(status))
    print("$", *diag, *sg_result)
    stdout.flush()


try:
    with driver.enable(), m0.enable(), m1.enable():
        while True:
            for slot in range(4):
                x0 = slot % 2 - 0.5
                Expect.wait(m0.move(x0, speed=0.2), m1.move(slot, speed=0.2))
                for _ in loop_for(0.2):
                    tick()
            Expect.wait(m0.move(0, speed=0.2), m1.move(0, speed=0.2))
            for _ in loop_for(0.2):
                tick()
except KeyboardInterrupt:
    pass

driver.close()
