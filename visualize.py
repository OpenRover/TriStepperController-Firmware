#!/usr/bin/env python3
import sys
import json
from typing import TypedDict
from itertools import accumulate
import matplotlib.pyplot as plt


class Motion(TypedDict):
    steps: int
    interval: int


# Load json string from stdin
data: list[Motion] = json.load(sys.stdin)

dt = [motion["interval"] * motion["steps"] * 1e-6 for motion in data]
dx = [motion["steps"] for motion in data]
V = [x / t if t > 0 else 0 for x, t in zip(dx, dt)]
T = list(accumulate(dt, initial=0))
TM = [(t1 + t2) / 2 for t1, t2 in zip(T[:-1], T[1:])]
X = list(accumulate(dx, initial=0))

fig, ax = plt.subplots(figsize=(10, 5))
ax.plot(T, X, marker="o", label="Position (steps)")
ax.plot(TM, V, marker="x", label="Speed (steps/s)")
ax.set_xlabel("Time (s)")
ax.set_title("Motion Profile")
ax.legend()
plt.grid()
plt.show()
