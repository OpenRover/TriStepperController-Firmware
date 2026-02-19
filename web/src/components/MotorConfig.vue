<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<script setup lang="ts">
import { computed } from "vue";
import type Motor from "lib/motor";
import Toggle from "./Toggle.vue";
import Editable from "./Editable.vue";
const props = defineProps<{ motor: Motor }>();
const theme = computed(() => {
  const { id } = props.motor;
  // Cycle through some nice colors
  const colors = ["#234", "#324", "#342", "#243", "#423", "#432"];
  return { "--theme": colors[id % colors.length] };
});
const invert = computed({
  get: () => Boolean(props.motor.config.invert),
  set: (val: boolean) => {
    props.motor.config.invert = val ? 1 : 0;
  },
});
</script>

<template>
  <div class="motor-config" :style="theme">
    <div class="title" style="user-select: none">
      <span style="font-weight: 900">Motor {{ motor.id }}</span>
      <div
        style="
          display: flex;
          flex-direction: row;
          align-items: center;
          gap: 0.6em;
        "
      >
        <span class="monospace" style="font-size: 0.8em">
          [{{ motor.enabled ? "Enabled" : "Disabled" }}] [{{
            motor.config.active ? "Active" : "Inactive"
          }}]
        </span>
        <Toggle v-model="motor.config.active" />
      </div>
    </div>
    <div class="entry">
      <span class="label">uSteps</span>
      <!-- Select from predefined options -->
      <select v-model.number="motor.config.micro_steps">
        <option :value="1">1</option>
        <option :value="2">2</option>
        <option :value="4">4</option>
        <option :value="8">8</option>
        <option :value="16">16</option>
        <option :value="32">32</option>
        <option :value="64">64</option>
        <option :value="128">128</option>
        <option :value="256">256</option>
      </select>
    </div>
    <div class="entry">
      <span class="label">Current</span>
      <input
       
        v-model.number="motor.config.rms_current"
        step="10"
      />
      <span class="unit">mA</span>
    </div>
    <div class="entry">
      <span class="label">Invert</span>
      <Toggle v-model="invert" />
    </div>
    <div class="entry">
      <span class="label">Scale</span>
      <input v-model.number="motor.config.scale" />
      <span class="unit">Rev/<Editable v-model="motor.config.unit" /></span>
    </div>
    <div class="entry">
      <span class="label">Speed</span>
      <input
       
        v-model.number="motor.speed"
        step="1"
        :disabled="motor.config.trapezoidal"
      />
      <span class="unit"><Editable v-model="motor.config.unit" />/s</span>
    </div>
    <div class="entry compact">
      <span class="label">Position</span>
      <input :value="motor.position" disabled />
      &nbsp;&rarr;&nbsp;
      <input
       
        v-model.number="motor.target"
        :disabled="!motor.enabled"
      />
      <span class="unit"><Editable v-model="motor.config.unit" /></span>
    </div>
    <div class="entry">
      <span class="label">Trapezoidal</span>
      <Toggle v-model="motor.config.trapezoidal" />
    </div>
    <div class="entry compact" :class="{ inactive: !motor.config.trapezoidal }">
      <span class="label">Speed Range</span>
      <input v-model.number="motor.config.min_vel" />
      &nbsp;-&nbsp;
      <input v-model.number="motor.config.max_vel" />
      <span class="unit"><Editable v-model="motor.config.unit" />/s</span>
    </div>
    <div class="entry" :class="{ inactive: !motor.config.trapezoidal }">
      <span class="label">Acceleration</span>
      <input v-model.number="motor.config.max_acc" step="1" />
      <span class="unit"
        ><Editable v-model="motor.config.unit" />/s<sup>2</sup></span
      >
    </div>
    <div class="entry">
      <span class="label">Motion Delay</span>
      <input v-model.number="motor.config.max_delay" step="1" />
      <span class="unit">ms</span>
    </div>
    <div class="entry">
      <span class="label">Resolution</span>
      <input v-model.number="motor.config.min_segment" step="1" />
      <span class="unit">ms</span>
    </div>
  </div>
</template>

<style scoped lang="scss">
.motor-config {
  display: block;
  background-color: #222;
  border-radius: 0.5rem;
  overflow: hidden;
  outline: 1px solid #444;
  padding-bottom: 0.4em;

  &:focus-within {
    outline: 2px solid #3af;
  }

  &:not(:focus-within):hover {
    outline: 2px solid #666;
  }

  & > * {
    width: 100%;
    display: flex;
    flex-direction: row;
    padding: 0.6rem 1rem;
    box-sizing: border-box;
    align-items: center;
    justify-content: flex-start;
    overflow: hidden;
    flex-wrap: nowrap;
  }

  & > .title {
    display: flex;
    flex-direction: row;
    align-items: center;
    justify-content: space-between;
    gap: 1rem;
    background-color: var(--theme);
    font-weight: 600;
    font-size: 1.1rem;
    margin-bottom: 0.5rem;
  }

  & > .entry {
    display: flex;
    flex-direction: row;
    align-items: center;
    justify-content: flex-start;
    font-family: "Cascadia Code", "Courier New", Courier, monospace;
    &:hover,
    &:focus-within {
      background: #fff1;
    }

    &.inactive {
      opacity: 0.5;
    }

    .label {
      min-width: 12ch;
      text-align: right;
      margin-right: 1rem;
      user-select: none;
    }

    input,
    select {
      font-family: inherit;
      width: 12ch;
      padding: 0.2rem;
      overflow: hidden;
      text-overflow: ellipsis;
    }

    &.compact input {
      width: 6ch;
    }

    .unit {
      margin-left: 0.5rem;
      color: #ccc;
      user-select: none;
    }
  }
}
</style>
