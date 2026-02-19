/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

export default function defer<T>() {
  let settled = false;
  let resolve!: (value: T | PromiseLike<T>) => void;
  let reject!: (reason?: any) => void;
  const promise = new Promise<T>((res, rej) => {
    resolve = res;
    reject = rej;
  });
  return {
    promise,
    resolve(value: T | PromiseLike<T>) {
      if (!settled) {
        resolve(value);
        settled = true;
      }
    },
    reject(reason?: any) {
      if (!settled) {
        reject(reason);
        settled = true;
      }
    },
    get settled() {
      return settled;
    },
  };
}

export type Defer<T> = ReturnType<typeof defer<T>>;
