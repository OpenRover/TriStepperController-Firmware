/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

export function approach(current: number, target: number, delta: number) {
  if (current < target) {
    return Math.min(current + Math.abs(delta), target);
  } else {
    return Math.max(current - Math.abs(delta), target);
  }
}

// Utility functions for hex representation

export function hex(n?: number, width = 2) {
  return (
    n?.toString(16).toUpperCase().padStart(width, "0") ?? "-".repeat(width)
  );
}

export function hexView(data: Uint8Array) {
  return Array.from(data)
    .map((b) => hex(b))
    .join(" ");
}

Object.assign(window, {
  hex,
  hexView,
  fromHex(hex: string): Uint8Array {
    return new Uint8Array(hex.split(/\s/).map((b) => parseInt(b, 16)));
  },
  checksum(data: Uint8Array): number {
    return data.reduce((a, b) => a ^ b, 0);
  },
});
