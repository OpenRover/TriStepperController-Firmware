/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import { hex } from "./util";

class EncodeError extends Error {
  readonly name = "Error encoding COBS packet";
  constructor(
    message: string,
    public readonly raw: Uint8Array,
  ) {
    super(
      message +
        " [" +
        Array.from(raw)
          .map((b) => hex(b))
          .join(" ") +
        "]",
    );
  }
}

class DecodeError extends Error {
  readonly name = "Error decoding COBS packet";
  constructor(
    message: string,
    public readonly raw: Uint8Array,
  ) {
    super(
      message +
        " [" +
        Array.from(raw)
          .map((b) => hex(b))
          .join(" ") +
        "]",
    );
  }
}

export default class COBS {
  static readonly EncodeError = EncodeError;
  static readonly DecodeError = DecodeError;

  static encode(data: Uint8Array): Uint8Array {
    if (data.length >= 254)
      throw new EncodeError("Data too long for COBS", data);
    const out = new Uint8Array(data.length + 2);
    let zero_idx = 0;
    data.forEach((byte, index) => {
      if (byte === 0) {
        out[zero_idx] = index - zero_idx + 1;
        zero_idx = index + 1;
      } else {
        out[index + 1] = byte;
      }
    });
    out[zero_idx] = data.length - zero_idx + 1;
    out[out.length - 1] = 0;
    return out;
  }

  static decode(data: Uint8Array): Uint8Array {
    // COBS decoding
    if (data.length < 3) throw new DecodeError("Raw data too short", data);
    const zero_idx = data.indexOf(0);
    if (zero_idx < data.length - 1)
      throw new DecodeError("Bad zero position", data);
    const out = new Uint8Array(data.length - 2);
    let counter = 1; // counts down to next zero
    data.forEach((byte, index) => {
      counter--;
      if (counter === 0) {
        counter = byte;
        byte = 0;
        return;
      }
      const j = index - 1;
      if (j >= out.length) throw new DecodeError("Early termination", data);
      out[j] = byte;
    });
    return out;
  }

  static async *chunks(stream: AsyncIterable<Uint8Array>) {
    let buffer = new Uint8Array(0);
    // Read until next zero byte (which indicates end of packet)
    for await (const chunk of stream) {
      // Append chunk to buffer
      const extended = new Uint8Array(buffer.length + chunk.length);
      extended.set(buffer);
      extended.set(chunk, buffer.length);
      buffer = extended;
      // check for packet termination
      while (true) {
        const zero_idx = buffer.indexOf(0);
        if (zero_idx === -1) break;
        yield buffer.slice(0, zero_idx + 1);
        buffer = buffer.slice(zero_idx + 1);
      }
    }
  }
}

(window as any).COBS = COBS;
