/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import { reactive, ref, watch } from "vue";
import { Method, Packet, Prop } from "./protocol";
import { u8, u16, u64, i64 } from "./stdint";
import driver from "./driver";
import type { Local } from "./local";
import local from "./local";
import { approach } from "./util";

export class MotorConfig {
  invert: number = 0; // forward = HIGH(false) or LOW(true)
  micro_steps: number = 16; // 1, 2, 4, 8, 16, 32, 64, 128, 256
  stall_sensitivity: number = 50; // 0 - 255, 0 = disabled
  rms_current: number = 1000; // mA
  // Host side only
  active: boolean = false; // whether motor is in active use
  scale: number = 1.0; // revolutions per unit distance
  unit: string = "unit"; // unit name
  // Trapezoidal control parameters
  trapezoidal: boolean = false; // enable trapezoidal control
  min_vel: number = 0.1; // units per second
  max_vel: number = 10.0; // units per second
  max_acc: number = 10.0; // units per second squared
  get steps_per_unit(): number {
    return 200 * this.micro_steps * this.scale;
  }
  update(data: Partial<MotorConfig>) {
    Object.assign(this, data);
    return this;
  }
  pack(id: number) {
    return [
      u8(id),
      u8(this.invert),
      u8(this.micro_steps),
      u8(this.stall_sensitivity),
      u16(this.rms_current),
    ];
  }
  async send(id: number) {
    const packed = this.pack(id);
    await driver.send(Packet.encode(Method.SET, Prop.MOT_CFG, ...packed));
    await driver
      .wait(Method.ACK, Prop.MOT_CFG, ([_id]) => id === _id)
      .timeout(1000);
  }
  save(key: string) {
    localStorage.setItem(key, JSON.stringify(this));
  }
  load(key: string) {
    const saved = localStorage.getItem(key);
    if (saved) {
      try {
        const data = JSON.parse(saved);
        this.update(data);
      } catch (e) {
        console.warn("Failed to parse saved motor config:", e);
        localStorage.removeItem(key);
      }
    }
  }
  static unpack(data: Uint8Array) {
    if (data.length < 6)
      throw new Error("Data too short to unpack MotorConfig");
    const [id, invert, micro_steps, stall_sensitivity, ...rc] = data;
    return {
      id,
      invert,
      micro_steps,
      stall_sensitivity,
      rms_current: rc[0]! | (rc[1]! << 8),
    };
  }
}

export default class Motor {
  readonly config = reactive(new MotorConfig()) as MotorConfig;

  readonly #position = ref<bigint>(0n); // absolute step position
  get position() {
    return Number(this.#position.value) / this.config.steps_per_unit;
  }

  #target = ref<bigint>(0n); // absolute step position
  get target() {
    return Number(this.#target.value) / this.config.steps_per_unit;
  }
  set target(value: number) {
    if (this.enabled) {
      this.#target.value = BigInt(
        Math.round(value * this.config.steps_per_unit),
      );
      this.move();
    }
  }

  #speed_manual: Local<number>; // units per second
  #speed_trapezoidal = ref(0); // units per second, can be negative
  get speed() {
    return this.config.trapezoidal
      ? this.#speed_trapezoidal.value
      : this.#speed_manual.value;
  }
  set speed(value: number) {
    if (this.config.trapezoidal) {
      console.warn("Cannot set speed in trapezoidal mode");
    } else {
      this.#speed_manual.value = value;
      this.move();
    }
  }

  /**
   * In a linear motion involving multiple axes, the proportion defines how
   * much motion component this motor contributes to the overall motion.
   * Speed and accelerations are scaled accordingly.
   */
  #proportion = ref(1.0);
  get proportion() {
    return this.#proportion.value;
  }
  set proportion(value: number) {
    this.#proportion.value = value;
  }

  #enabled = ref<boolean>(false);
  get enabled() {
    return this.#enabled.value;
  }
  async enable() {
    if (!this.config.active) return;
    await this.config.send(this.id);
    await driver.send(
      Packet.encode(Method.SET, Prop.MOT_ENA, u8(this.id), u8(0x01)),
    );
    await driver
      .wait(Method.ACK, Prop.MOT_ENA, ([id, en]) => this.match(id) && en)
      .timeout(1000);
    this.#enabled.value = true;
  }
  async disable() {
    await driver.send(
      Packet.encode(Method.SET, Prop.MOT_ENA, u8(this.id), u8(0x00)),
    );
    await driver
      .wait(Method.ACK, Prop.MOT_ENA, ([id, en]) => this.match(id) && !en)
      .timeout(1000);
    this.#enabled.value = false;
  }

  get local_storage_key() {
    return `motor/config:${this.id}`;
  }

  private match(id?: number) {
    return id === this.id;
  }

  constructor(public readonly id: number) {
    const { local_storage_key } = this;
    this.#speed_manual = local([local_storage_key, "speed"].join("/"), 1.0);
    this.config.load(local_storage_key);
    watch(this.config, () => this.config.save(local_storage_key), {
      deep: true,
    });
    watch(
      () => this.config.pack(this.id),
      async () => {
        const packed = this.config.pack(this.id);
        const prev_position = this.position;
        await driver.send(Packet.encode(Method.SET, Prop.MOT_CFG, ...packed));
        await driver
          .wait(Method.ACK, Prop.MOT_CFG, ([id]) => this.match(id))
          .timeout(1000);
        await this.setPosition(prev_position);
      },
    );
    driver.onEnable(() => this.enable());
    driver.onBeforeDisable(() => this.disable());
    watch(this.#enabled, (en) => {
      if (!en) {
        this.#position.value = 0n;
        this.#target.value = 0n;
      }
    });
    // Subscribe to position updates
    (async () => {
      const id = this.id.toString();
      for await (const msg of driver.SYN) {
        try {
          if (msg.type === "POS" && msg.has(id)) {
            this.#position.value = BigInt(msg.get(id)!);
            if (this.enabled && this.config.trapezoidal)
              this.trapezoidalUpdate();
          }
        } catch (e) {
          console.warn("Failed to parse motor position message:", msg, e);
        }
      }
    })();
  }

  private get target_position_steps() {
    if (this.config.trapezoidal) {
      return this.#trapezoidal_overshoot.value ?? this.#target.value;
    } else {
      return this.#target.value;
    }
  }

  private get step_interval() {
    const { steps_per_unit, trapezoidal, min_vel } = this.config;
    // Get relative speed (units per second)
    let relative_speed = Math.abs(this.speed);
    // In case of trapezoidal control, don't allow zero speed
    if (trapezoidal && relative_speed < min_vel) relative_speed = min_vel;
    // Convert to absolute speed (steps per second)
    const absolute_speed = Math.abs(relative_speed * steps_per_unit);
    // Return step interval in microseconds
    return absolute_speed > 0 ? 1e6 / absolute_speed : 0;
  }

  public move(target?: number | null, speed?: number | null) {
    if (typeof target === "number" && !isNaN(target)) this.target = target;
    if (typeof speed === "number" && !isNaN(speed)) this.speed = speed;
    const payload = [
      // Motor ID
      u8(this.id),
      // Target position (int64)
      i64(this.target_position_steps),
      // Step interval in us (uint64)
      u64(this.step_interval),
    ];
    return driver.send(Packet.encode(Method.SET, Prop.MOT_MOV, ...payload));
  }

  public setPosition(position: number = 0) {
    this.target = position;
    const payload = [
      // Motor ID
      u8(this.id),
      // Target position (int64)
      i64(this.#target.value),
      // Step interval in us (uint64)
      u64(0),
    ];
    return driver.send(Packet.encode(Method.SET, Prop.MOT_MOV, ...payload));
  }

  private last_tick = performance.now();
  private tick(dt_max = 1.0) {
    const now = performance.now();
    const dt = (now - this.last_tick) / 1000.0; // seconds
    this.last_tick = now;
    return Math.min(dt, dt_max);
  }

  #trapezoidal_overshoot = ref<number | null>(null);
  public trapezoidalUpdate() {
    const dt = this.tick();
    if (!this.config.trapezoidal) return;
    const { min_vel: V0, max_vel: V1, max_acc: A } = this.config;
    const { position, target, speed: v0 } = this;
    const d = target - position; // distance to target (signed)
    // Termination condition: slow approach near target
    if (Math.abs(v0) <= V0 && Math.abs(d / V0) < V0 / A) {
      this.#speed_trapezoidal.value = 0;
      this.#trapezoidal_overshoot.value = null;
      return;
    }
    // Suppose start braking now, compute distance until stop
    const break_distance = (Math.sign(v0) * v0 ** 2) / (2 * A);
    // Stop position relative to target position
    const stop_pos = d - break_distance;
    const side = Math.sign(d) * Math.sign(stop_pos);
    let v1: number;
    switch (side) {
      case +1:
        // Undershoot, accelerate towards target
        v1 = Math.sign(d) * V1;
        this.#trapezoidal_overshoot.value = null;
        break;
      case -1:
        // Overshoot
        v1 = 0;
        this.#speed_trapezoidal.value = approach(v0, 0, A * dt);
        break;
      case 0:
      default:
        // Will stop exactly at target
        v1 = 0;
        this.#speed_trapezoidal.value = approach(v0, 0, A * dt);
        return;
    }
    this.#speed_trapezoidal.value = approach(v0, v1, A * dt);
    this.move();
  }
}

export const m0 = new Motor(0);
export const m1 = new Motor(1);
export const m2 = new Motor(2);
