<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<script setup lang="ts">
import { computed } from 'vue';
const props = defineProps({
    modelValue: Boolean
})
const emit = defineEmits({ 'update:modelValue': (_: boolean) => true })
function toggle() {
    emit('update:modelValue', !props.modelValue)
}
function attrs(obj: Record<string, any>): Record<string, string> {
    return Object.fromEntries(Object.entries(obj).map(([k, v]) => [k, v.toString()]));
}
const offset = computed(() => props.modelValue ? 100 : -100)
const greenRect = computed(() => attrs({
    x: -200,
    y: -100,
    width: offset.value + 300,
    height: 200,
    rx: 100,
    fill: 'green'
}));
const redRect = computed(() => attrs({
    x: offset.value - 100,
    y: -100,
    width: 300 - offset.value,
    height: 200,
    rx: 100,
    fill: 'red'
}));
</script>

<template>
    <svg viewBox="-220 -110 440 220" @click="toggle" style="cursor: pointer">
        <rect v-bind="greenRect" />
        <rect v-bind="redRect" />
        <circle r="100" :cx="offset" :fill="props.modelValue ? 'green' : 'red'" stroke="white" stroke-width="40" />
    </svg>
</template>

<style scoped lang="scss">
svg {
    height: 1em;
    width: 2em;
    display: block;
    overflow: visible;

    * {
        transition: all 0.2s ease-in-out;
    }

    rect {
        stroke: #000A;
        stroke-width: 40;
    }

    circle {
        box-shadow: 0 0 10px #000;
    }

    &:hover circle {
        r: 110;
        stroke-width: 60;
    }
}
</style>
