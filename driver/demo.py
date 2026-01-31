#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse
from time import sleep
from math import degrees, radians, sin, cos
from sys import path, stdout, stderr
from posixpath import dirname

path.append(dirname(__file__))

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

driver = Driver(baudrate=baud, verbose=verbose)
print(f"Device Info: {driver.getInfo()}", file=stderr)

m0 = Motor(
    driver,
    id=0,
    invert=0,
    scale=6.0 / 360.0,
    micro_steps=16,
    init_pos=0.0,
    rms_current=1000,
    stall_sensitivity=50,
)

m1 = Motor(
    driver,
    id=1,
    invert=1,
    scale=6.0 / 360.0,
    micro_steps=16,
    init_pos=0.0,
    rms_current=1000,
    stall_sensitivity=50,
)


try:
    with driver.enable(), m0.enable(), m1.enable():
        R = 15.0  # degrees
        V = 15.0  # degrees per second
        try:
            Expect.wait(m0.move(R, speed=V), m1.move(0, speed=V))
            while True:
                for theta in range(0, 360):
                    rx = R * cos(radians(theta))
                    vx = V * max(abs(sin(radians(theta))), 0.1)
                    ry = R * sin(radians(theta))
                    vy = V * max(abs(cos(radians(theta))), 0.1)
                    Expect.wait(
                        m0.move(rx, speed=vx),
                        m1.move(ry, speed=vy),
                    )
        except KeyboardInterrupt:
            Expect.wait(m0.move(0.0, speed=V), m1.move(0.0, speed=V))
except KeyboardInterrupt:
    pass

driver.close()
