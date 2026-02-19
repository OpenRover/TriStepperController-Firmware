/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import { Method, Prop, Packet } from "./protocol";
import AsyncChain from "async-chain-list";
import { bool } from "./stdint";
import { hex, hexView } from "./util";
import serial from "./serial";
import createEvent from "./event";
import defer, { type Defer } from "./defer";
import SequencePool from "./sequence-pool";
import COBS from "./cobs";

class SyncMessage extends Map<string, string> {
  readonly type: string;
  constructor(message: string) {
    super();
    const [type, ...entries] = message.trim().split(" ");
    if (!type) throw new Error("Invalid sync message: " + message);
    this.type = type;
    for (const entry of entries) {
      const [key, ...rest] = entry.split("=");
      if (!key) console.warn("Invalid sync entry:", entry);
      else this.set(key, rest.join("="));
    }
  }
}

class TimeoutError extends Error {
  name = "RequestTimeout";
  private end = Date.now();
  constructor(
    message: string = "Operation timed out",
    public readonly start: number = Date.now(),
  ) {
    super(message);
  }
  refresh() {
    this.end = Date.now();
    return this;
  }
  toString() {
    return `${this.name}: ${this.message} (after ${Math.round(this.end - this.start)} ms)`;
  }
}

class RejectedError extends Error {
  name = "RequestRejected";
}

class Driver extends SequencePool<Defer<Packet>, number> {
  // Incoming stream for broadcast packets (seq=0)
  private rx = new AsyncChain<Packet>();
  [Symbol.asyncIterator]() {
    return this.rx[Symbol.asyncIterator]();
  }
  public readonly SYN = new AsyncChain<SyncMessage>();
  // Events
  public readonly onBeforeEnable = createEvent();
  public readonly onEnable = createEvent();
  public readonly onBeforeDisable = createEvent();
  public readonly onDisable = createEvent();

  constructor() {
    super(new Set(Array.from({ length: 255 }, (_, i) => i + 1)));
    serial.onConnect(() => this.enable(1000));
    serial.onBeforeDisconnect(() => this.disable(1000));
    (async () => {
      // Read until next zero byte (which indicates end of packet)
      for await (const chunk of COBS.chunks(serial)) {
        try {
          const decoded = COBS.decode(chunk);
          if (decoded.length < 3)
            throw new Error("Packet too short after COBS decoding");
          // Verify CRC
          const checksum = decoded.reduce((c, b) => c ^ b, 0);
          if (checksum !== 0)
            throw new Error(
              `Invalid packet CRC: expected 0, got ${hex(checksum)}`,
            );
          const sequence = (decoded[2]! << 8) | decoded[1]!;
          const payload = decoded.slice(3);
          if (sequence === 0) {
            const packet = new Packet(payload);
            packet.print(`⬆ ${sequence.toString().padStart(6, " ")}`);
            this.rx = this.rx.push(packet);
          } else {
            const deferred = this.resolveSequence(sequence);
            try {
              const packet = new Packet(payload);
              packet.print(`⬆ ${sequence.toString().padStart(6, " ")}`);
              switch (packet.method) {
                case Method.ACK:
                  deferred.resolve(packet);
                  break;
                case Method.REJ:
                  deferred.reject(new RejectedError(packet.text));
                  break;
                default:
                  console.warn(
                    `Unexpected method for #${sequence}: ${Method[packet.method]}`,
                  );
                  deferred.resolve(packet);
              }
            } catch (e) {
              deferred.reject(e);
            }
          }
        } catch (e) {
          console.warn(e, hexView(chunk));
        }
      }
    })();
    (async () => {
      for await (const packet of this.rx) {
        if (packet.method === Method.SYN && packet.prop === Prop.NA) {
          const text = new TextDecoder().decode(packet.payload);
          try {
            const msg = new SyncMessage(text);
            this.SYN = this.SYN.push(msg);
          } catch (e) {
            console.warn("Invalid sync message:", text, e);
          }
        }
      }
    })();
  }

  request(packet: Packet, timeout?: number) {
    const deferred = defer<Packet>();
    const { promise, reject } = deferred;
    const sequence = this.assignSequence(deferred);
    packet.print(`⬇ ${sequence.toString().padStart(6, " ")}`);
    // Set up timeout if specified
    if (timeout !== undefined) {
      const error = new TimeoutError([sequence, packet].join(" "));
      const handler = setTimeout(() => reject(error.refresh()), timeout);
      promise.finally(() => clearTimeout(handler));
    }
    // Catch-all to release sequence on promise settlement
    promise.finally(() => this.releaseSequence(sequence, deferred));
    // Split u16 sequence into two u8 bytes
    const seq_l = sequence & 0xff;
    const seq_h = (sequence >> 8) & 0xff;
    // Compute CRC byte
    const crc = packet.reduce((c, b) => c ^ b, seq_l ^ seq_h);
    // Prefix with crc and sequence byte
    const payload = new Uint8Array(packet.length + 3);
    payload[0] = crc;
    payload[1] = seq_l; // Lower 8 bits
    payload[2] = seq_h; // Higher 8 bits
    payload.set(packet, 3);
    // Send via serial
    try {
      serial.write(COBS.encode(payload));
    } catch (e) {
      reject(e);
    }
    // Return deferred promise
    return promise;
  }

  #enabled = false;
  get enabled() {
    return this.#enabled;
  }

  async enable(timeout?: number) {
    await Promise.all(this.onBeforeEnable.dispatch());
    await this.request(
      Packet.encode(Method.SET, Prop.SYS_ENA, bool(true)),
      timeout,
    );
    this.#enabled = true;
    return Promise.all(this.onEnable.dispatch());
  }

  async disable(timeout?: number) {
    await Promise.allSettled(this.onBeforeDisable.dispatch());
    await this.request(
      Packet.encode(Method.SET, Prop.SYS_ENA, bool(false)),
      timeout,
    );
    this.#enabled = false;
    await Promise.all(this.onDisable.dispatch());
  }
}

const driver = new Driver();

driver.onBeforeEnable(() => console.log("driver.onBeforeEnable"));
driver.onEnable(() => console.log("driver.onEnable"));
driver.onBeforeDisable(() => console.log("driver.onBeforeDisable"));
driver.onDisable(() => console.log("driver.onDisable"));

export default driver;
