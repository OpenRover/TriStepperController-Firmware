/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

export function round(value: number | bigint) {
    if (typeof value === 'bigint') {
        return value;
    } else {
        return Math.round(value);
    }
}

export function bool(value: boolean | number): ArrayBuffer {
    const arr = new Uint8Array(1);
    arr[0] = value ? 1 : 0;
    return arr.buffer;
}

export function u8(value: number): ArrayBuffer {
    const arr = new Uint8Array(1);
    arr[0] = value & 0xFF;
    return arr.buffer;
}

export function i8(value: number): ArrayBuffer {
    const arr = new Int8Array(1);
    arr[0] = value | 0;
    return arr.buffer;
}

export function u16(value: number): ArrayBuffer {
    const arr = new Uint16Array(1);
    arr[0] = value & 0xFFFF;
    return arr.buffer;
}

export function i16(value: number): ArrayBuffer {
    const arr = new Int16Array(1);
    arr[0] = value | 0;
    return arr.buffer;
}

export function u32(value: number): ArrayBuffer {
    const arr = new Uint32Array(1);
    arr[0] = value >>> 0;
    return arr.buffer;
}

export function i32(value: number): ArrayBuffer {
    const arr = new Int32Array(1);
    arr[0] = value | 0;
    return arr.buffer;
}

export function u64(value: number | bigint): ArrayBuffer {
    const arr = new BigUint64Array(1);
    arr[0] = BigInt(round(value));
    return arr.buffer;
}

export function i64(value: number | bigint): ArrayBuffer {
    const arr = new BigInt64Array(1);
    arr[0] = BigInt(round(value));
    return arr.buffer;
}

export function f32(value: number): ArrayBuffer {
    const arr = new Float32Array(1);
    arr[0] = value;
    return arr.buffer;
}

export function f64(value: number): ArrayBuffer {
    const arr = new Float64Array(1);
    arr[0] = value;
    return arr.buffer;
}
