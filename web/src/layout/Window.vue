<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<script setup lang="ts">
const emit = defineEmits<{ close: [] }>();
const props = defineProps<{
    title: string;
    style?: Record<string, string> | string;
}>();
</script>

<template>
    <div class="window">
        <div class="title-bar">
            <span class="title">{{ title }}</span>
            <button class="close-button" @click="emit('close')">
                <svg viewBox="-3 -3 6 6" width="2em" height="2em" fill="none" stroke="currentColor" stroke-width="0.25"
                    stroke-linecap="round" stroke-linejoin="round">
                    <line x1="-1" y1="-1" x2="+1" y2="+1"></line>
                    <line x1="-1" y1="+1" x2="+1" y2="-1"></line>
                </svg>
            </button>
        </div>
        <div class="content" :style="style">
            <slot></slot>
        </div>
    </div>
</template>

<style scoped>
.window {
    border-radius: 8px;
    background-color: #1e1e1e;
    outline: 1px solid #333;
    overflow: hidden;
    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
    display: block;
    padding: 0;
}

.window:hover,
.window:focus-within {
    outline: 1px solid #3AF;
}

.window .title-bar {
    padding-left: 1em;
    display: flex;
    justify-content: space-between;
    align-items: center;
    user-select: none;
    border-bottom: 1px solid #333;
}

.window .title-bar .title {
    font-size: 1.1em;
    font-weight: 600;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

.window .title-bar .close-button {
    background: transparent;
    border: none;
    color: #999;
    width: 4em;
    height: 4em;
    padding: 0;
    margin: 0;
}

.window .title-bar .close-button:hover {
    background: #333;
    color: #fff;
}

.window .title-bar .close-button:active {
    background: #222;
    color: #fff;
}

.window .content {
    padding: 1em;
    display: block;
}
</style>