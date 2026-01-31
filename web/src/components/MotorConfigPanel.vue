<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<script setup lang="ts">
import MotorConfig from "./MotorConfig.vue";
import { m0, m1, m2 } from "lib/motor";
const emit = defineEmits<{ close: [] }>();
</script>

<template>
  <div class="panel" @click="emit('close')">
    <div class="content" @click.stop>
      <MotorConfig :motor="m0" />
      <MotorConfig :motor="m1" />
      <MotorConfig :motor="m2" />
    </div>
  </div>
</template>

<style scoped>
.panel {
  position: relative;
  width: 100%;
  height: 100%;
  backdrop-filter: blur(10px);
}

.content {
  pointer-events: none;
  position: absolute;
  top: 0;
  left: 0;
  bottom: 0;
  right: 0;
  padding: 1rem;
  overflow-x: hidden;
  overflow-y: scroll;
  pointer-events: none;
}

.content > * {
  pointer-events: all;
  display: inline-block;
  margin: 1rem;
  max-width: min(calc(100vw - 2rem), 100ch);
}
</style>
