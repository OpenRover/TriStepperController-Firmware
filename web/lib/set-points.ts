/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import { ref, computed } from "vue";

type Pos = {
  x: number;
  y: number;
};

type CommandHandler = (data: Pos[], args: string[]) => void;

function optionalFlag(args: string[], ...flags: string[]): number {
  let count = 0;
  for (const flag of flags) {
    for (;;) {
      const index = args.indexOf(flag);
      if (index === -1) break;
      args.splice(index, 1);
      count += 1;
    }
  }
  return count;
}

const commands: Record<string, CommandHandler> = {
  interpolate(points: Pos[], args: string[]) {
    const zigzag = optionalFlag(args, "zigzag") > 0;
    if (args.length !== 1 && args.length !== 2)
      throw new Error("Interpolate command requires 1 or 2 parameters");
    const params = args.map((arg) => parseInt(arg));
    if (params.some((p) => isNaN(p)))
      throw new Error("Invalid parameters for interpolate command");
    const N1 = params[0]!;
    const N2 = params.length === 2 ? params[1]! : N1;
    // Take 4 points from the end of points as control points
    // Generate N1 x N2 interpolated points using bilinear interpolation
    if (points.length < 4)
      throw new Error("Not enough points for interpolation");
    const [a, b, c, d] = points.splice(-4) as [Pos, Pos, Pos, Pos];
    // for (let i = 0; i < N1; i++) {
    //   const k = zigzag && i % 2 === 1 ? 1 - i / (N1 - 1) : i / (N1 - 1);
    //   console.log({ k });
    //   const ab = { x: a.x * (1 - k) + b.x * k, y: a.y * (1 - k) + b.y * k };
    //   const cd = { x: c.x * (1 - k) + d.x * k, y: c.y * (1 - k) + d.y * k };
    //   for (let j = 0; j < N2; j++) {
    //     const t = j / (N2 - 1);
    //     points.push({
    //       x: ab.x * (1 - t) + cd.x * t,
    //       y: ab.y * (1 - t) + cd.y * t,
    //     });
    //   }
    // }
    // Iterate over Y
    for (let j = 0; j < N2; j++) {
      const t = j / (N2 - 1);
      const ac = { x: a.x * (1 - t) + c.x * t, y: a.y * (1 - t) + c.y * t };
      const bd = { x: b.x * (1 - t) + d.x * t, y: b.y * (1 - t) + d.y * t };
      for (let i = 0; i < N1; i++) {
        const k = zigzag && j % 2 === 1 ? 1 - i / (N1 - 1) : i / (N1 - 1);
        points.push({
          x: ac.x * (1 - k) + bd.x * k,
          y: ac.y * (1 - k) + bd.y * k,
        });
      }
    }
  },
};

interface RefLike<T> {
  value: T;
}

export default class SetPoints {
  constructor(initial: string | RefLike<string> = "") {
    if (typeof initial === "string") {
      this.#raw = ref(initial);
    } else {
      this.#raw = initial;
    }
  }
  #raw: RefLike<string>;
  get raw() {
    return this.#raw.value;
  }
  set raw(value: string) {
    this.#raw.value = value;
  }

  private get lines() {
    return this.raw
      .split("\n")
      .map((l) => l.trim())
      .filter(Boolean);
  }

  private build() {
    const points = new Array<Pos>();
    for (const line of this.lines) {
      if (line.startsWith("#"))
        continue; // comment line
      else if (line.startsWith("@")) {
        const [cmd, ...args] = line.slice(1).split(/\s+/);
        const handler = commands[cmd!];
        if (handler) {
          handler(points, args);
        } else {
          throw new Error(`Unknown command: ${cmd}`);
        }
      } else {
        // Parse point as "x,y"
        const [x_str, y_str] = line.split(",");
        const x = parseFloat(x_str ?? "");
        const y = parseFloat(y_str ?? "");
        if (isNaN(x) || isNaN(y)) throw new Error(`Invalid point: ${line}`);
        points.push({ x, y });
      }
    }
    return points;
  }

  append(point: Pos) {
    let line = `${point.x},${point.y}\n`;
    if (this.raw && !this.raw.endsWith("\n")) line = "\n" + line;
    this.raw += line;
  }

  #output = computed(() => {
    try {
      return this.build();
    } catch (e) {
      if (e instanceof Error) return e;
      else return new Error(String(e));
    }
  });

  get output() {
    return this.#output.value;
  }

  [Symbol.iterator]() {
    const { output } = this;
    if (output instanceof Error) {
      return [][Symbol.iterator]();
    } else {
      return output[Symbol.iterator]();
    }
  }
}
