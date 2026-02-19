/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import { hexView } from "./util";

const METHOD_MASK = 0xf0;

export enum Method {
  NOP = 0x00,
  // HOST -> DEVICE
  GET = 0x10,
  SET = 0x20,
  // DEVICE -> HOST
  ACK = 0x30,
  REJ = 0x40,
  // DEVICE -> HOST, for asynchronous events
  SYN = 0x80,
  // Special Log Method
  LOG = 0xf0,
}

const PROP_MASK = 0x0f;

export enum Prop {
  NA = 0x0,
  SYS_ENA = 0x1,
  MOT_ENA = 0x2,
  MOT_CFG = 0x3,
  MOT_MOV = 0x4,
  MOT_HOME = 0x5,
  MOT_STAT = 0x6,
  LED_PROG = 0xa,
  ODOM_SENSOR = 0xb,
  COLOR_SENSOR = 0xc,
  FW_INFO = 0xf,
}

export class Packet extends Uint8Array {
  constructor(buffer: ArrayBuffer | ArrayLike<number>) {
    super(buffer);
    if (this.length < 1) throw new Error("Packet too short");
  }
  static encode(
    method: Method,
    prop: Prop,
    ...data: (ArrayBuffer | string)[]
  ): Packet {
    // Calculate total length
    const payload = data.map((item) => {
      if (typeof item === "string") {
        const encoder = new TextEncoder();
        return encoder.encode(item);
      } else {
        return new Uint8Array(item);
      }
    });
    const payload_size = payload.reduce((sum, arr) => sum + arr.length, 0);
    const packet = new Uint8Array(1 + payload_size);
    packet[0] = (method & METHOD_MASK) | (prop & PROP_MASK);
    // Copy data arrays
    let offset = 1;
    for (const arr of payload) {
      packet.set(arr, offset);
      offset += arr.length;
    }
    return new Packet(packet);
  }
  get header(): number {
    return this[0]!;
  }
  set header(value: number) {
    this[0] = value & 0xff;
  }
  get method(): Method {
    return (this.header & METHOD_MASK) as Method;
  }
  get method_name(): string {
    return Method[this.method];
  }
  set method(value: Method) {
    this[0] = (this.header & ~METHOD_MASK) | (value & METHOD_MASK);
  }
  get prop(): Prop {
    return (this.header & PROP_MASK) as Prop;
  }
  get prop_name(): string {
    return Prop[this.prop];
  }
  set prop(value: Prop) {
    this.header = (this.header & ~PROP_MASK) | (value & PROP_MASK);
  }
  get payload(): Uint8Array {
    return new Uint8Array([...this].slice(1));
  }
  get text() {
    if ([Method.LOG, Method.REJ].includes(this.method))
      return new TextDecoder().decode(this.payload);
    else return hexView(this.payload);
  }
  print(prefix: string) {
    const { method, prop } = this;
    const color =
      {
        [Method.NOP]: "gray",
        [Method.GET]: "cyan",
        [Method.SET]: "cyan",
        [Method.ACK]: "lime",
        [Method.REJ]: "red",
        [Method.SYN]: "orange",
        [Method.LOG]: "purple",
      }[method] || "white";
    console.debug(
      `%c%s %c%s`,
      `font-weight: bold; color: ${color}`,
      `${prefix && prefix + " "}${Method[method]}:${Prop[prop].padEnd(7, " ")}`,
      "color: gray",
      this.text,
    );
    return this;
  }
  toString() {
    return `[Packet ${Method[this.method]}:${Prop[this.prop]} ${this.payload.length} bytes] ${this.text}`;
  }
}
