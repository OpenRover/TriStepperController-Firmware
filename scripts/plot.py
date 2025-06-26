#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys, argparse, re, queue
from threading import Thread
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
from time import time

parser = argparse.ArgumentParser(description="Plot data from stdin.")
parser.add_argument(
    "-s", "--separator", type=str, default=None, help="Separator for data columns"
)
parser.add_argument(
    "-T",
    "--time-window",
    type=float,
    default=5.0,
    help="Viewing time window in seconds",
)
parser.add_argument(
    "-p",
    "--preamble",
    type=str,
    default=None,
    help="Preamble of valid data in input lines",
)

args = parser.parse_args()
sep = str(args.separator) if args.separator is not None else None
preamble = str(args.preamble) if args.preamble is not None else None
time_window = float(args.time_window)
t0 = time()

flag_term = False
input_queue = queue.Queue()


def input_handler():
    global flag_term
    for line in sys.stdin:
        # Forward input to output
        sys.stdout.write(line)
        sys.stdout.flush()
        if preamble is not None:
            if not line.startswith(preamble):
                continue
            line = line[len(preamble) :]
        input_queue.put(line.strip())
    flag_term = True


input_thread = Thread(target=input_handler)
input_thread.daemon = False
input_thread.start()


class Swatch:
    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]
    color_index = 0

    def __call__(self):
        color = self.colors[self.color_index % len(self.colors)]
        self.color_index += 1
        return color


class FigureContext:
    # Cleared per loop
    unnamed_count = 0

    def __init__(self, name: str):
        print(f"Creating figure '{name}'")
        self.name = name
        self.swatch = Swatch()

        self.fig = plt.figure(name)
        self.ax = self.fig.add_subplot(111)
        self.ax.set_xlabel("Time (s)")
        self.ax.set_ylabel("Value")
        self.ax.grid()
        self.ax.set_xlim(0, time_window)
        plt.show(block=False)

        self.range = None
        self.lines: dict[str, "Line2D"] = {}

    def get_line(self, id: str) -> Line2D:
        if id in self.lines:
            return self.lines[id]
        else:
            print(f"Creating line {id} for figure '{self.name}'")
            line = Line2D(
                [],
                [],
                marker="o",
                linestyle="-",
                markersize=2,
                color=self.swatch(),
            )
            self.ax.add_line(line)
            self.lines[id] = line
            return line

    def add_data(self, t: float, v: float, id: str | None = None):
        if self.range is None:
            self.range = (v, v)
        else:
            a, b = self.range
            self.range = (min(a, v), max(b, v))
        if id is None:
            id = f"unnamed-{self.unnamed_count}"
            self.unnamed_count += 1
        line = self.get_line(id)
        T, V = line.get_data(orig=True)
        line.set_data([*T, t], [*V, v])

    def loop_end(self):
        self.unnamed_count = 0
        if self.range is not None:
            a, b = self.range
            self.ax.set_ylim(a - 0.1 * (b - a), b + 0.1 * (b - a))


ctx = dict[str, FigureContext]()


def get_fig(name: str) -> FigureContext:
    if name not in ctx:
        ctx[name] = FigureContext(name)
    return ctx[name]


def parse_col(col: str) -> tuple[str, str | None, float]:
    if "=" not in col:
        return "(Unnamed Data)", None, float(col.strip())
    else:
        name, value = col.split("=", 1)
        f, id = name.split(".", 1) if "." in name else (name, None)
        return f.strip(), id.strip(), float(value.strip())


try:
    while not flag_term:
        plt.draw()
        plt.pause(0.001)
        try:
            input = input_queue.get(block=False)
        except queue.Empty:
            input = None
        try:
            t1 = time() - t0
            if input is not None:
                input = str(input).strip()
            if input is None:
                cols = []
            elif sep is not None:
                cols = input.split(sep)
            else:
                cols = re.split(r"\s+|,|;", input)
            ddl = t1 - time_window
            for col in cols:
                f, id, v = parse_col(col)
                print(f"parse_col() => {f}, {id}, {v}")
                get_fig(f).add_data(t1, v, id)
            for f in ctx.values():
                f.loop_end()
                if ddl > 0:
                    f.ax.set_xlim(ddl, t1)
        except ValueError:
            print(f"Invalid input: {input}", file=sys.stderr)
except KeyboardInterrupt:
    print("\nPress Ctrl-D to exit.")
