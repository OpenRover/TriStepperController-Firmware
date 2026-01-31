/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import AsyncChain from "async-chain-list";
import type { AbortablePromise } from "./abortable";
import abortable from "./abortable";
import createEvent from "./event";

function hex(n?: number, width = 4) {
  return (
    n?.toString(16).toUpperCase().padStart(width, "0") ?? "-".repeat(width)
  );
}

export function hexView(data: Uint8Array) {
  return Array.from(data)
    .map((b) => hex(b, 2))
    .join(" ");
}

export function getPortIdString(port?: SerialPort | null) {
  if (!port) return;
  const info = port.getInfo();
  return [hex(info.usbVendorId, 4), hex(info.usbProductId, 4)].join(":");
}

type Reader = ReadableStreamDefaultReader<Uint8Array>;
type Writer = WritableStreamDefaultWriter<Uint8Array>;

// Global serial communication singleton
const serial = new (class Serial {
  private port: SerialPort | null = null;
  private reader: Reader | null = null;
  private writer: Writer | null = null;
  // RX stream reader
  private task: AbortablePromise<void> | null = null;
  // RX Stream multiplexer
  private chain = new AsyncChain<Uint8Array>();
  // Events
  readonly onBeforeConnect = createEvent();
  readonly onConnect = createEvent();
  readonly onBeforeDisconnect = createEvent();
  readonly onDisconnect = createEvent();

  public async connect(port: SerialPort, baudRate = 115200) {
    await this.disconnect();
    this.port = port;
    await Promise.allSettled(this.onBeforeConnect.dispatch());
    await port.open({ baudRate });
    this.reader = port.readable?.getReader() ?? null;
    this.writer = port.writable?.getWriter() ?? null;
    this.task = abortable(async (abortable) => {
      const { reader } = this;
      if (!reader)
        return console.warn("Port", getPortIdString(this.port), "not readable");
      abortable.onAbort(() => reader.cancel());
      try {
        while (!abortable.aborted) {
          const { value, done } = await reader.read();
          if (done) break;
          if (!value) continue;
          this.chain = this.chain.push(value);
        }
      } catch (e) {
        console.error("Serial read error:", e);
      }
    });
    await Promise.allSettled(this.onConnect.dispatch());
  }

  public async disconnect() {
    const { port, task } = this;
    if (!port && !task) return;
    await Promise.allSettled(this.onBeforeDisconnect.dispatch());
    this.port = null;
    this.task = null;
    await task?.abort();
    this.reader = (this.reader?.releaseLock(), null);
    this.writer = (this.writer?.releaseLock(), null);
    await port?.close();
    await Promise.allSettled(this.onDisconnect.dispatch());
  }

  public send(data: Uint8Array) {
    const { writer } = this;
    if (!writer) throw new Error("Serial port not connected or not writable");
    return writer.write(data);
  }

  public [Symbol.asyncIterator]() {
    return this.chain[Symbol.asyncIterator]();
  }
})();

export default serial;

window.addEventListener("beforeunload", () => serial.disconnect());

serial.onConnect(() => console.log("serial.onConnect"));
serial.onDisconnect(() => console.log("serial.onDisconnect"));
serial.onBeforeConnect(() => console.log("serial.onBeforeConnect"));
serial.onBeforeDisconnect(() => console.log("serial.onBeforeDisconnect"));
