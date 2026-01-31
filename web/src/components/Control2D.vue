<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<script setup lang="ts">
import debounce from "debounce";
import { computed } from "vue";
import PosView, { type Pos } from "./PosView.vue";
import { m0, m1 } from "lib/motor";
const p = computed(() => ({ x: m0.position, y: m1.position }));
const t = computed(() => ({ x: m0.target, y: m1.target }));
const onSelect = debounce(({ x, y }: Pos) => {
  m0.target = x;
  m1.target = y;
}, 0.1);
</script>

<template>
  <PosView
    class="view"
    color="#0B6"
    :lim="30"
    :pos="p"
    unit="°"
    :fontSize="3"
    @select="(p) => p && onSelect(p)"
  >
    <template #top>
      <line
        :x1="p.x"
        :y1="p.y"
        :x2="t.x"
        :y2="t.y"
        stroke="white"
        stroke-linecap="round"
        style="stroke-width: var(--T)"
      />
      <circle :cx="t.x" :cy="t.y" fill="white" style="r: calc(var(--T) * 2)" />
      <circle
        :cx="p.x"
        :cy="p.y"
        fill="black"
        stroke="white"
        style="r: calc(var(--R) / 2); stroke-width: var(--T)"
      />
    </template>
  </PosView>
</template>

<style scoped></style>
