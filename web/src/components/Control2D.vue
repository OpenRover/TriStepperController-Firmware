<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<script setup lang="ts">
import debounce from "debounce";
import { computed, ref, useTemplateRef, watch } from "vue";
import PosView, { type Pos } from "./PosView.vue";
import { m0, m1 } from "lib/motor";
import HorizontalDivision from "src/layout/HorizontalDivision.vue";
import local from "lib/local";
import SetPoints from "lib/set-points";
import Diagnostics from "./Diagnostics.vue";
import RangeSlider from "./RangeSlider.vue";

const p = computed(() => ({
  x: m0.position_transient,
  y: m1.position_transient,
}));
const t = computed({
  get() {
    return { x: m0.target, y: m1.target };
  },
  set({ x, y }: Pos) {
    m0.target = x;
    m1.target = y;
  },
});
const onSelect = debounce(({ x, y }: Pos) => {
  m0.target = x;
  m1.target = y;
}, 0.01);
function resetPosition() {
  m0.resetPosition();
  m1.resetPosition();
}
const lim = local<number>("Control2D/lim", 30);
const points_raw = local<string>("Control2D/points", "");
const points = new SetPoints(points_raw);
function recordPoint() {
  points.append(p.value);
}
const editorToggle = ref(false);
const pointsEditor = useTemplateRef<HTMLTextAreaElement>("pointsEditor");
watch(editorToggle, (v) => {
  if (v) setTimeout(() => pointsEditor.value?.focus(), 0);
});
const hover_point_idx = ref<number | null>(null);
const segments = computed(() => {
  const pts = points.output;
  const hover = hover_point_idx.value;
  if (pts instanceof Error) return [];
  return pts.map((a, i) => ({
    a,
    b: pts[i + 1],
    hover: i === hover && hover === i,
  }));
});
</script>

<template>
  <HorizontalDivision style="height: 100%">
    <template #left>
      <div class="motion-pad">
        <PosView
          class="view"
          color="#0B6"
          :lim="lim"
          :pos="p"
          unit="°"
          :fontSize="lim / 10"
          @select="(p) => p && onSelect(p)"
        >
          <template v-for="{ a, b, hover } of segments">
            <line
              v-if="b"
              :x1="a.x + 'px'"
              :y1="a.y + 'px'"
              :x2="b.x + 'px'"
              :y2="b.y + 'px'"
              opacity="0.25"
              stroke="cyan"
              stroke-linecap="round"
              style="stroke-width: var(--T)"
            />
            <line
              v-else
              :x1="a.x + 'px'"
              :y1="a.y + 'px'"
              :x2="p.x + 'px'"
              :y2="p.y + 'px'"
              stroke="yellow"
              opacity="0.5"
              stroke-dasharray="calc(var(--T) * 2) calc(var(--T) * 2)"
              style="stroke-width: var(--T)"
            />
          </template>
          <template
            v-if="points.output instanceof Array && true"
            v-for="(a, i) of points.output"
            :key="i"
          >
            <circle
              v-if="hover_point_idx === i"
              :cx="a.x + 'px'"
              :cy="a.y + 'px'"
              style="r: var(--R); stroke-width: var(--T)"
              stroke="yellow"
              fill="none"
            />
            <circle
              :cx="a.x + 'px'"
              :cy="a.y + 'px'"
              fill="white"
              style="r: calc(var(--R) / 2)"
            />
          </template>
          <template #top>
            <line
              :x1="p.x + 'px'"
              :y1="p.y + 'px'"
              :x2="t.x + 'px'"
              :y2="t.y + 'px'"
              stroke="white"
              stroke-linecap="round"
              style="stroke-width: var(--T)"
            />
            <circle
              :cx="t.x + 'px'"
              :cy="t.y + 'px'"
              fill="white"
              style="r: calc(var(--T) * 2)"
            />
            <circle
              :cx="p.x + 'px'"
              :cy="p.y + 'px'"
              fill="black"
              stroke="white"
              style="r: calc(var(--R) / 2); stroke-width: var(--T)"
            />
          </template>
        </PosView>
        <textarea
          ref="pointsEditor"
          class="points-editor monospace"
          v-model="points.raw"
          v-if="editorToggle"
          @keydown.esc="editorToggle = false"
        ></textarea>
      </div>
    </template>
    <template #right>
      <div class="sidebar">
        <Diagnostics />
        <RangeSlider
          v-model="lim"
          :min="1"
          :max="200"
          :step="10"
          style="display: block; width: auto"
        >
          <span>Motion Limit</span>
          <span
            >±{{ lim.toFixed(2)
            }}{{ m0.config.unit ?? m1.config.unit ?? "units" }}</span
          >
        </RangeSlider>
        <div class="set-points monospace">
          <div
            v-for="(p, i) in points.output"
            :key="i"
            v-if="points.output instanceof Array && true"
            :class="{
              active: t.x === p.x && t.y === p.y,
            }"
            @click="t = p"
            @mouseenter="hover_point_idx = i"
            @mouseleave="hover_point_idx = null"
          >
            [{{ i.toString().padStart(2, "0") }}] {{ p.x.toFixed(2) }}°,
            {{ p.y.toFixed(2) }}°
          </div>
        </div>
      </div>
      <div class="bottom-sticky">
        <button style="background-color: #080" @click="recordPoint">
          Record
        </button>
        <button
          style="background-color: #06c"
          @click="editorToggle = !editorToggle"
        >
          Edit
        </button>
        <button style="background-color: #c00" @click="resetPosition">
          Reset Position
        </button>
      </div>
    </template>
  </HorizontalDivision>
</template>

<style scoped lang="scss">
.motion-pad {
  width: 100%;
  height: 100%;
  background-color: #0004;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 1rem;
  padding: 1rem;
}

.sidebar {
  width: 100%;
  height: 100%;
  overflow-y: scroll;
  color: white;
  background-color: #0004;
  margin: 0;
  padding: 0;
  padding: 1px 0 4rem 0;
  & > * {
    margin: 1rem;
  }
}

.bottom-sticky {
  position: absolute;
  bottom: 0;
  height: 3rem;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  gap: 1rem;
  width: 100%;
  margin: 0;
  background-color: #0002;
  border-top: 1px solid #fff2;
  backdrop-filter: blur(8px);
  button {
    padding: 0.4rem 0.6rem;
    border: none;
    border-radius: 0.25rem;
    color: white;
    font-weight: bold;
    cursor: pointer;
  }
}

.points-editor {
  position: absolute;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  padding: 1rem;
  line-height: 1.6;
  font-size: 1rem;
  background-color: #0002;
  backdrop-filter: blur(2px);
}

.set-points {
  margin-top: 1rem;
  display: flex;
  flex-direction: column;
  gap: 0.25rem;
  margin: 0;
  & > * {
    border-left: 0.5ch solid transparent;
    padding: 0.5em 1em;
    &:hover {
      border-left: 0.5ch solid #fff2;
      background-color: #fff1;
    }
    &.active {
      color: #08f;
      font-weight: bold;
      border-left: 0.5ch solid #08f;
    }
  }
}
</style>
