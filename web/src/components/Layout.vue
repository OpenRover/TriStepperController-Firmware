<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<script setup lang="ts">
import { overlay } from './Overlay.vue';
</script>

<template>
    <div class="layout">
        <header class="title-bar">
            <div class="title">TriStepper Control</div>
            <div class="actions">
                <slot name="actions"></slot>
            </div>
        </header>
        <main class="content">
            <slot></slot>
        </main>
    </div>
    <div class="overlay">
        <component :is="overlay?.overlay" @close="overlay = null" />
    </div>
</template>

<style scoped>
.layout {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    margin: 0;
    padding: 0;
}

.title-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.5rem 1rem;
    background: #1a1a1a;
    border-bottom: 1px solid #333;
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    width: 100%;
    z-index: 100;
    box-sizing: border-box;
    height: 4rem;
}

.title {
    font-size: 1.2rem;
    font-weight: 600;
    color: #fff;
}

.actions {
    display: flex;
    gap: 0.5rem;
}

.icon-button {
    padding: 0.5rem;
    background: transparent;
    border: 1px solid #444;
    border-radius: 4px;
    cursor: pointer;
    color: #999;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.3s;
}

.icon-button:hover {
    background: #2a2a2a;
    border-color: #666;
}

.icon-button.connected {
    color: #42b883;
    border-color: #42b883;
}

.icon-button.connected:hover {
    background: rgba(66, 184, 131, 0.1);
}

.content {
    flex: 1;
    padding: 2rem;
    margin-top: 4rem;
    overflow-y: auto
}


.overlay {
    position: fixed;
    top: 4rem;
    left: 0;
    right: 0;
    bottom: 0;
    pointer-events: none;

    &>* {
        pointer-events: all;
    }
}
</style>
