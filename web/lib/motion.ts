/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

export type Motion = {
  steps: number; // delta steps in this motion
  interval: number; // microseconds per step
};

export type MotionSegment = {
  dx: number; // total steps in this segment
  dt?: number; // total time in seconds for this segment
  v0: number; // initial velocity in steps/s
  v1?: number; // final velocity in steps/s
};

/**
 * Generate trapezoidal motion profile.
 * Splits motion into:
 * 1. Acceleration phase: from v0 to ±v_max with acceleration ±acc
 * 2. Constant velocity phase: at ±v_max
 * 3. Deceleration phase: from ±v_max to v1 with acceleration ±acc
 * Returns an array of motion segments, each being one of the three phases above.
 * @param dx Travel distance in steps (positive or negative integer)
 * @param v0 Initial velocity (steps/s)
 * @param v1 Final velocity (steps/s)
 * @param v_max Maximum velocity (steps/s)
 * @param acc Acceleration (steps/s^2)
 * @returns Array of motion segments with v0, v1, dx, dt
 */
export function trapezoidal(
  dx: number,
  v0: number,
  v1: number = v0,
  v_max: number = v0,
  acc: number = 0,
): Array<MotionSegment> {
  if (dx === 0) return [];

  const direction = Math.sign(dx);
  const distance = Math.abs(dx);
  const vm = Math.abs(v_max);
  const a = Math.abs(acc);

  // Project into motion direction so planning is performed in positive space.
  // Any opposite-direction boundary speed is treated as zero for this segment.
  const u0 = Math.max(0, direction * v0);
  const u1 = Math.max(0, direction * v1);

  // Degenerate case: no acceleration limit or no velocity limit.
  if (a <= 0 || vm <= 0) {
    return [{ dx, v0, v1 }];
  }

  const segments: Array<MotionSegment> = [];
  const push = (dx: number, v0: number, v1: number) => {
    dx = Math.round(dx);
    if (dx === 0) return;
    const avg = (Math.abs(v0) + Math.abs(v1)) / 2;
    const dt = avg > 0 ? Math.abs(dx) / avg : undefined;
    segments.push({ dx, v0, v1, dt });
  };

  // Distance required to reach capped max speed and then settle to final speed.
  const d_acc = Math.max(0, (vm * vm - u0 * u0) / (2 * a));
  const d_dec = Math.max(0, (vm * vm - u1 * u1) / (2 * a));

  if (d_acc + d_dec <= distance) {
    // Trapezoidal profile (accelerate - cruise - decelerate)
    const d1 = d_acc;
    const d3 = d_dec;
    const d2 = distance - d1 - d3;
    push(direction * d1, direction * u0, direction * vm);
    push(direction * d2, direction * vm, direction * vm);
    push(direction * d3, direction * vm, direction * u1);
  } else {
    // Triangular profile (accelerate/decelerate without cruise)
    const vp = Math.sqrt(Math.max(0, a * distance + (u0 * u0 + u1 * u1) / 2));
    const d1 = Math.max(0, (vp * vp - u0 * u0) / (2 * a));
    const d3 = Math.max(0, distance - d1);
    push(direction * d1, direction * u0, direction * vp);
    push(direction * d3, direction * vp, direction * u1);
  }

  // Keep total displacement exactly equal to requested dx.
  const planned_dx = segments.reduce((sum, s) => sum + s.dx, 0);
  const remainder = dx - planned_dx;
  if (remainder !== 0) {
    if (segments.length > 0) segments[segments.length - 1]!.dx += remainder;
    else segments.push({ dx, v0, v1 });
  }

  return segments;
}

/**
 * Discretize the motion profile into segments of fixed time step dt.
 * Each segment is represented as a Motion object with steps and interval.
 *
 * @param dx Travel distance in steps (integer)
 * @param v0 Initial velocity, positive (steps/s)
 * @param v1 Final velocity, positive (steps/s)
 * @param dt Time resolution for discretization (seconds)
 */
export function* discretize(segment: MotionSegment): Generator<Motion> {
  const { dx, v0, v1 = v0 } = segment;
  const T = dx / ((v0 + v1) / 2); // total time based on average velocity
  const dt = segment.dt ?? T;
  if (T <= 0) return []; // no motion needed
  const checkpoints = linspace(0, T, dt).map(
    v0 === v1
      ? (t) => {
          if (t <= 0) return { t, x: 0, v: v0 };
          if (t >= T) return { t, x: dx, v: v0 };
          return { t, x: Math.round(v0 * t), v: v0 };
        }
      : (t) => {
          if (t <= 0) return { t, x: 0, v: v0 };
          if (t >= T) return { t, x: dx, v: v1 };
          return {
            t,
            x: Math.round(v0 * t + ((v1 - v0) * t * t) / (2 * T)),
            v: v0 + ((v1 - v0) * t) / T,
          };
        },
  );
  for (const [a, b] of pairs(checkpoints)) {
    const steps = b.x - a.x;
    const interval = Math.round(Math.abs(2e6 / (a.v + b.v)));
    if (steps !== 0) yield { steps, interval };
  }
}

function linspace(t0: number, t1: number, dt: number): number[] {
  const times = [];
  for (let t = t0; t < t1; t += dt) times.push(t);
  if (times.length === 0 || times[times.length - 1]! < t1) times.push(t1);
  return times;
}

function* pairs<T>(arr: T[]): Generator<[T, T]> {
  for (let i = 0; i < arr.length - 1; i++) {
    yield [arr[i]!, arr[i + 1]!];
  }
}
