/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import AsyncChain from "async-chain-list";
import { type AbortablePromise } from "./abortable";
import abortable from "./abortable";
import createEvent from "./event";
import { hex, hexView } from "./util";

export function getPortIdString(port?: SerialPort | null) {
  if (!port) return;
  const info = port.getInfo();
  return [hex(info.usbVendorId, 4), hex(info.usbProductId, 4)].join(":");
}

// Global serial communication singleton
const serial = new (class Serial {
  private port: SerialPort | null = null;
  // RX stream reader
  private rx_task: AbortablePromise<void> | null = null;
  private tx_task: AbortablePromise<void> | null = null;
  // RX Stream
  private rx = new AsyncChain<Uint8Array>();
  // TX Stream
  private tx = new AsyncChain<Uint8Array>();
  // Events
  readonly onBeforeConnect = createEvent();
  readonly onConnect = createEvent();
  readonly onBeforeDisconnect = createEvent();
  readonly onDisconnect = createEvent();

  private connected = false;

  public async connect(port: SerialPort, baudRate = 115200) {
    if (this.connected) await this.disconnect();
    this.connected = true;
    this.port = port;
    await Promise.all(this.onBeforeConnect.dispatch());
    await port.open({ baudRate });
    const portName = getPortIdString(port);
    this.rx_task = abortable(async (abortable) => {
      const reader = port.readable?.getReader() ?? null;
      try {
        if (!reader) return console.warn("Port", portName, "not readable");
        abortable.onAbort(() => reader.cancel());
        while (!abortable.aborted) {
          const { value, done } = await reader.read();
          if (done) break;
          if (!value) continue;
          this.rx = this.rx.push(value);
        }
      } catch (e) {
        console.error("Serial read error:", e);
        this.disconnect();
      } finally {
        reader?.releaseLock();
        this.rx_task = null;
      }
    });
    this.tx_task = abortable(async (abortable) => {
      try {
        for await (const chunk of abortable.iter(this.tx)) {
          const writer = port.writable?.getWriter() ?? null;
          if (!writer) continue;
          try {
            await writer.ready;
            await writer.write(chunk);
          } catch (e) {
            console.error("Serial write error:", e);
          } finally {
            writer.releaseLock();
          }
        }
      } finally {
        this.tx_task = null;
        this.disconnect();
      }
    });
    await Promise.all(this.onConnect.dispatch());
  }

  public async disconnect() {
    if (!this.connected) return;
    this.connected = false;
    await Promise.allSettled(this.onBeforeDisconnect.dispatch());
    await Promise.allSettled([this.rx_task?.abort(), this.tx_task?.abort()]);
    await this.port?.close();
    this.port = null;
    await Promise.all(this.onDisconnect.dispatch());
  }

  public write(data: Uint8Array) {
    if (this.port === null) throw new Error("Serial port not connected");
    if (!this.tx_task) throw new Error("Serial port TX task not running");
    console.log("RAW TX:", hexView(data));
    this.tx = this.tx.push(data);
  }

  public [Symbol.asyncIterator]() {
    return this.rx[Symbol.asyncIterator]();
  }
})();

export default serial;

window.addEventListener("beforeunload", () => serial.disconnect());

serial.onConnect(() => console.log("serial.onConnect"));
serial.onDisconnect(() => console.log("serial.onDisconnect"));
serial.onBeforeConnect(() => console.log("serial.onBeforeConnect"));
serial.onBeforeDisconnect(() => console.log("serial.onBeforeDisconnect"));
