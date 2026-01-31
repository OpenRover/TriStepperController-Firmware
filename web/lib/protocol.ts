/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import { hexView } from "./serial";

const METHOD_MASK = 0xF0;

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
    LOG = 0xF0,
}

const PROP_MASK = 0x0F;

export enum Prop {
    NA = 0x0,
    SYS_ENA = 0x1,
    MOT_ENA = 0x2,
    MOT_CFG = 0x3,
    MOT_MOV = 0x4,
    MOT_HOME = 0x5,
    MOT_STAT = 0x6,
    LED_PROG = 0xA,
    ODOM_SENSOR = 0xB,
    COLOR_SENSOR = 0xC,
    FW_INFO = 0xF,
}

class COBSDecodeError extends Error {
    readonly name = 'Error decoding COBS packet';
    constructor(message: string, public readonly raw: Uint8Array) {
        super(message + ' [' + Array.from(raw).map(b => b.toString(16).padStart(2, '0')).join(' ') + ']');
    }
}

export class Packet extends Uint8Array {
    constructor(buffer: ArrayBuffer | ArrayLike<number>) {
        super(buffer);
        if (this.length < 2)
            throw new Error('Packet too short');
    }
    static encode(method: Method, prop: Prop, ...data: (ArrayBuffer | string)[]): Packet {
        // Calculate total length
        const normalized = data.map((item) => {
            if (typeof item === 'string') {
                const encoder = new TextEncoder();
                return encoder.encode(item);
            } else {
                return new Uint8Array(item);
            }
        });
        const payload_size = normalized.reduce((sum, arr) => sum + arr.length, 0);
        const packet = new Uint8Array(2 + payload_size);
        packet[0] = 0; // checksum placeholder
        packet[1] = (method & METHOD_MASK) | (prop & PROP_MASK);
        // Copy data arrays
        let offset = 2;
        for (const arr of normalized) {
            packet.set(arr, offset);
            offset += arr.length;
        }
        // XOR all bytes in the payload with the first byte (checksum)
        packet[0] = packet.slice(1).reduce((chk, byte) => chk ^ byte, 0);
        return new Packet(packet);
    }
    static decode(data: Uint8Array): Packet {
        // COBS decoding
        if (data.length <= 2)
            throw new COBSDecodeError('Raw data too short', data);
        const zero_idx = data.indexOf(0);
        if (zero_idx < data.length - 1)
            throw new COBSDecodeError('Bad zero position', data);
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
            if (j >= out.length) {
                throw new COBSDecodeError('Early termination', data);
            }
            out[j] = byte;
        });
        return new Packet(out.buffer);
    }
    get method(): Method {
        const header = this[1]!;
        return (header & METHOD_MASK) as Method;
    }
    get method_name(): string {
        return Method[this.method];
    }
    set method(value: Method) {
        const header = this[1]!;
        this[1] = (header & ~METHOD_MASK) | (value & METHOD_MASK);
    }
    get prop(): Prop {
        const header = this[1]!;
        return (header & PROP_MASK) as Prop;
    }
    get prop_name(): string {
        return Prop[this.prop];
    }
    set prop(value: Prop) {
        const header = this[1]!;
        this[1] = (header & ~PROP_MASK) | (value & PROP_MASK);
    }
    get payload(): Uint8Array {
        return new Uint8Array([...this].slice(2));
    }
    validate(): boolean {
        return this.reduce((chk, byte) => chk ^ byte, 0) === 0;
    }
    get COBS(): Uint8Array {
        if (this.length >= 254)
            throw new Error(`Packet too large for COBS encoding (${this.length})`);
        const out = new Uint8Array(this.length + 2);
        let zero_idx = 0;
        this.forEach((byte, index) => {
            if (byte === 0) {
                out[zero_idx] = index - zero_idx + 1;
                zero_idx = index + 1;
            } else {
                out[index + 1] = byte;
            }
        });
        out[zero_idx] = this.length - zero_idx + 1;
        out[out.length - 1] = 0;
        return out;
    }
    print(prefix: string, style: string = 'color: blue') {
        const { method, prop, payload } = this;
        let content: string;
        if ([Method.LOG, Method.SYN].includes(method))
            content = new TextDecoder().decode(payload);
        else
            content = hexView(payload);
        console.debug(
            `%c%s %c%s`,
            `font-weight: bold; ${style}`,
            `${prefix && prefix + ' '}${Method[method]}:${Prop[prop].padEnd(7, ' ')}`,
            'color: gray',
            content
        );
        return this;
    }
}
