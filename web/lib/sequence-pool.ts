/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

class NotFound<K> extends Error {
  public readonly name = "Sequence Not Found";
  constructor(public readonly sequence: K) {
    super(`Sequence [${sequence}] Not Found`);
  }
}

export default class SequencePool<T, K> {
  readonly #pool: Set<K>;
  readonly #pending = new Map<K, T>();
  constructor(pool: Iterable<K>) {
    this.#pool = new Set(pool);
  }
  assignSequence(item: T): K {
    for (const seq of this.#pool) {
      this.#pool.delete(seq);
      this.#pending.set(seq, item);
      return seq;
    }
    throw new Error("Sequence Pool Exhausted");
  }
  resolveSequence(seq: K): T {
    if (!this.#pending.has(seq)) throw new NotFound(seq);
    return this.#pending.get(seq)!;
  }
  releaseSequence(seq: K, match: T): boolean {
    if (!this.#pending.has(seq)) return false;
    if (this.#pending.get(seq) !== match) return false;
    this.#pending.delete(seq);
    this.#pool.add(seq);
    return true;
  }
}
