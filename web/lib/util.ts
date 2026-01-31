/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

export function approach(current: number, target: number, delta: number) {
  if (current < target) {
    return Math.min(current + delta, target);
  } else {
    return Math.max(current - delta, target);
  }
}
