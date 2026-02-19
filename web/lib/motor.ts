/* ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 * ------------------------------------------------------ */

import { reactive, ref, watch } from "vue";
import { Method, Packet, Prop } from "./protocol";
import { u8, u16, u32, i32 } from "./stdint";
import driver from "./driver";
import type { Local } from "./local";
import local from "./local";
import { type Motion, trapezoidal, discretize } from "./motion";

export class MotorConfig {
  micro_steps: number = 16; // 1, 2, 4, 8, 16, 32, 64, 128, 256
  stall_sensitivity: number = 50; // 0 - 255, 0 = disabled
  rms_current: number = 1000; // mA
  // Host side only
  active: boolean = false; // whether motor is in active use
  invert: number = 0; // forward = HIGH(false) or LOW(true)
  scale: number = 1.0; // revolutions per unit distance
  unit: string = "unit"; // unit name
  // Trapezoidal control parameters
  trapezoidal: boolean = false; // enable trapezoidal control
  min_vel: number = 0.1; // units per second
  max_vel: number = 10.0; // units per second
  max_acc: number = 10.0; // units per second squared
  // Motion planner parameters
  max_delay: number = 100; // milliseconds
  min_segment: number = 16; // milliseconds

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
      u8(this.micro_steps),
      u8(this.stall_sensitivity),
      u16(this.rms_current),
    ];
  }
  async apply(id: number) {
    const packed = this.pack(id);
    await driver.request(
      Packet.encode(Method.SET, Prop.MOT_CFG, ...packed),
      1000,
    );
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
    if (data.length < 5)
      throw new Error("Data too short to unpack MotorConfig");
    const [id, micro_steps, stall_sensitivity, rc_l, rc_h] = data;
    return {
      id,
      micro_steps,
      stall_sensitivity,
      rms_current: rc_l! | (rc_h! << 8),
    };
  }
}

export default class Motor {
  readonly config = reactive(new MotorConfig()) as MotorConfig;

  readonly #position = ref<bigint>(0n); // absolute step position
  get position() {
    return Number(this.#position.value) / this.config.steps_per_unit;
  }
  get position_steps() {
    return this.#position.value;
  }
  set position(value: number) {
    const steps = BigInt(Math.round(value * this.config.steps_per_unit));
    this.#position.value = steps;
  }
  set position_steps(value: bigint) {
    this.#position.value = value;
  }

  readonly #position_transient = ref<bigint>(0n); // transient step position
  get position_transient() {
    return Number(this.#position_transient.value) / this.config.steps_per_unit;
  }

  #target = ref<number>(0); // units
  get target() {
    return this.#target.value;
  }
  private get target_steps() {
    return BigInt(Math.round(this.#target.value * this.config.steps_per_unit));
  }
  private set target_steps(value: bigint) {
    this.#target.value = Number(value) / this.config.steps_per_unit;
  }
  set target(value: number) {
    if (this.enabled) {
      this.#target.value = value;
      this.plan();
    }
  }

  #speed_manual: Local<number>; // units per second
  #speed_transient = ref(0); // units per second
  get speed() {
    return this.config.trapezoidal
      ? this.#speed_transient.value
      : this.#speed_manual.value;
  }
  get speed_steps_per_sec() {
    return this.speed * this.config.steps_per_unit;
  }
  set speed(value: number) {
    if (this.config.trapezoidal) {
      console.warn("Cannot set speed in trapezoidal mode");
    } else {
      this.#speed_manual.value = value;
      this.plan();
    }
  }

  #enabled = ref<boolean>(false);
  get enabled() {
    return this.#enabled.value;
  }
  async enable(timeout?: number) {
    if (!this.config.active) return;
    await this.config.apply(this.id);
    await driver.request(
      Packet.encode(Method.SET, Prop.MOT_ENA, u8(this.id), u8(0x01)),
      timeout,
    );
    this.#enabled.value = true;
  }
  async disable(timeout?: number) {
    await driver.request(
      Packet.encode(Method.SET, Prop.MOT_ENA, u8(this.id), u8(0x00)),
      timeout,
    );
    this.#enabled.value = false;
  }

  get local_storage_key() {
    return `motor/config:${this.id}`;
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
        await driver.request(
          Packet.encode(Method.SET, Prop.MOT_CFG, ...packed),
          1000,
        );
      },
    );
    driver.onEnable(() => this.enable(1000));
    driver.onBeforeDisable(() => this.disable(1000));
    watch(this.#enabled, (en) => {
      if (!en) {
        this.#position.value = 0n;
        this.#target.value = 0;
      } else {
        this.plan();
      }
    });
    watch(
      () => this.config.active,
      (active) => {
        if (driver.enabled) {
          if (active) this.enable(1000);
          else this.disable(1000);
        }
      },
    );
  }

  public plan() {
    // Clear pending motions
    this.motion_queue.length = 0;
    // Config parameters
    const { max_delay, min_segment } = this.config;
    // Current motion state
    const x0 = this.position_steps;
    const x1 = this.target_steps;
    console.log(`Motor ${this.id} planning from ${x0} to ${x1}`);
    if (!this.config.trapezoidal) {
      // Simple planning: constant speed, split into segments to avoid long blocking
      const direction = Math.sign(Number(x1 - x0));
      if (direction === 0) return;
      this.motion_queue.push(
        ...discretize({
          dx: Number(x1 - x0),
          v0: direction * this.speed_steps_per_sec,
          dt: 1e-3 * Math.max(max_delay / 2, min_segment),
        }),
      );
    } else {
      // Trapezoidal planning: accelerate to max speed, cruise, then decelerate
      const v0 = this.#speed_transient.value * this.config.steps_per_unit; // current speed in steps per second
      const dx = Number(x1 - x0); // distance to travel in steps
      const v_max = this.config.max_vel * this.config.steps_per_unit; // steps per second
      const acc = this.config.max_acc * this.config.steps_per_unit; // steps per second squared
      const dt = min_segment * 1e-3; // time resolution in seconds
      for (const motion of trapezoidal(dx, v0, 0, v_max, acc)) {
        console.log(motion);
        this.motion_queue.push(...discretize({ ...motion, dt }));
      }
    }
    console.log("queued:", [...this.motion_queue]);
    this.queue();
  }

  // Set position without moving the motor, used for homing
  public resetPosition() {
    this.#position.value = 0n;
    this.#target.value = 0;
    this.#position_transient.value = 0n;
    this.plan();
  }

  private t_next_mot_ack = performance.now();
  private readonly dispatched_motion = new Map<symbol, number>();
  get motion_delay() {
    let delay = 0;
    for (const d of this.dispatched_motion.values()) delay += d;
    return delay;
  }
  get dispatched_motion_count() {
    return this.dispatched_motion.size;
  }

  private async dispatch(
    { steps, interval }: Motion,
    duration: number = (steps * interval) / 1000,
  ) {
    const token = Symbol();
    const delta = BigInt(steps);
    try {
      this.dispatched_motion.set(token, duration);
      this.position_steps += delta;
      const speed = 1e6 / interval / this.config.steps_per_unit;
      this.#speed_transient.value = speed;
      console.log({ speed });
      let timeout = this.motion_delay;
      const now = performance.now();
      if (this.t_next_mot_ack > now) timeout += this.t_next_mot_ack - now;
      timeout += steps * interval * 1e-3; // add expected motion duration
      const packet = Packet.encode(
        Method.SET,
        Prop.MOT_MOV,
        u8(this.id),
        i32(this.config.invert ? -steps : steps),
        u32(interval),
      );
      await driver.request(packet, timeout + 10000);
      this.t_next_mot_ack = performance.now() + this.motion_delay;
      this.#position_transient.value += delta;
    } catch (e) {
      this.position_steps -= delta;
      this.target_steps -= delta;
      console.warn("Failed to dispatch motion:", e);
    } finally {
      this.dispatched_motion.delete(token);
      this.queue();
    }
  }

  private async barrier() {
    const packet = Packet.encode(
      Method.SET,
      Prop.MOT_MOV,
      u8(this.id),
      i32(0),
      u32(0),
    );
    try {
      await driver.request(packet);
      this.#speed_transient.value = 0;
    } catch {
    } finally {
      this.queue();
    }
  }

  readonly motion_queue: Motion[] = [];

  private async queue() {
    await new Promise((r) => setTimeout(r, 0));
    // Dispatch motions to fill the plan window
    while (this.motion_queue.length > 0) {
      const motion = this.motion_queue[0]!;
      const duration = Math.max(1, motion.steps) * motion.interval * 1e-3; // milliseconds
      if (this.motion_delay + duration > this.config.max_delay) break;
      this.dispatch(motion, duration);
      this.motion_queue.shift();
      await new Promise((r) => setTimeout(r, 0));
    }
  }
}

export const m0 = new Motor(0);
export const m1 = new Motor(1);
export const m2 = new Motor(2);
