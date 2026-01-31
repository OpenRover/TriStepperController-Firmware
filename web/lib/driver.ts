/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import { Method, Prop, Packet } from "./protocol";
import AsyncChain from "async-chain-list";
import { bool } from "./stdint";
import serial from "./serial";
import createEvent from "./event";
import abortable from "./abortable";

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

const driver = new (class Driver {
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
    serial.onConnect(() => this.enable());
    serial.onBeforeDisconnect(() => this.disable());
    (async () => {
      let buffer = new Uint8Array(0);
      // Read until next zero byte (which indicates end of packet)
      for await (const chunk of serial) {
        // Append chunk to buffer
        const newBuffer = new Uint8Array(buffer.length + chunk.length);
        newBuffer.set(buffer);
        newBuffer.set(chunk, buffer.length);
        buffer = newBuffer;
        // check for packet termination
        while (true) {
          const zero_idx = buffer.indexOf(0);
          if (zero_idx === -1) break;
          const packet_raw = buffer.slice(0, zero_idx + 1);
          buffer = buffer.slice(zero_idx + 1);
          try {
            const packet = Packet.decode(packet_raw);
            if (packet.validate())
              this.rx = this.rx.push(packet.print("⬆", "color: lime"));
            else console.warn("Invalid packet checksum:", packet);
          } catch (e) {
            console.warn(e);
          }
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

  send(packet: Packet) {
    return serial.send(packet.print("⬇", "color: cyan").COBS);
  }

  wait(
    method: Method,
    prop: Prop,
    validate: (payload: Uint8Array) => boolean | number | undefined,
  ) {
    return abortable(async (abortable) => {
      for await (const packet of abortable.iter(this.rx)) {
        if (
          packet.method === method &&
          packet.prop === prop &&
          validate(packet.payload)
        )
          return packet;
      }
    });
  }

  #enabled = false;
  get enabled() {
    return this.#enabled;
  }

  async enable() {
    await Promise.allSettled(this.onBeforeEnable.dispatch());
    await this.send(Packet.encode(Method.SET, Prop.SYS_ENA, bool(true)));
    await this.wait(
      Method.ACK,
      Prop.SYS_ENA,
      ([enabled]) => enabled === 0x01,
    ).timeout(1000);
    this.#enabled = true;
    await Promise.allSettled(this.onEnable.dispatch());
  }

  async disable() {
    await Promise.allSettled(this.onBeforeDisable.dispatch());
    await this.send(Packet.encode(Method.SET, Prop.SYS_ENA, bool(false)));
    await this.wait(
      Method.ACK,
      Prop.SYS_ENA,
      ([enabled]) => enabled === 0x00,
    ).timeout(1000);
    this.#enabled = false;
    await Promise.allSettled(this.onDisable.dispatch());
  }
})();

export default driver;

driver.onBeforeEnable(() => console.log("driver.onBeforeEnable"));
driver.onEnable(() => console.log("driver.onEnable"));
driver.onBeforeDisable(() => console.log("driver.onBeforeDisable"));
driver.onDisable(() => console.log("driver.onDisable"));
