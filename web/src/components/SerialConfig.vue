<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<script lang="ts">
import { computed, ref, watch, shallowRef } from 'vue'
import serial, { getPortIdString } from 'lib/serial'
import Window from '../layout/Window.vue'
import Toggle from './Toggle.vue'

type ConnStatus = 'disconnected' | 'connecting' | 'connected' | 'disconnecting'
// Global singletons exported for use across the app
export const portInfo = ref<string | null>(null)
export const connInfo = ref<ConnStatus>('disconnected')

serial.onBeforeConnect(() => connInfo.value = 'connecting')
serial.onConnect(() => connInfo.value = 'connected')
serial.onBeforeDisconnect(() => connInfo.value = 'disconnecting')
serial.onDisconnect(() => connInfo.value = 'disconnected')

async function findLastUsedPort(): Promise<SerialPort | null> {
  try {
    const portInfoString = localStorage.getItem('serial/port')
    if (!portInfoString || !('serial' in navigator)) return null
    const info = JSON.parse(portInfoString)
    const ports = await navigator.serial.getPorts()
    search_ports:
    for (const port of ports) {
      const portInfo = port.getInfo()
      for (const key of Object.keys(info))
        if ((portInfo as any)[key] !== info[key])
          continue search_ports
      return port
    }
  } catch { }
  return null
}
const port = shallowRef<SerialPort | null>(null)
const baud = ref<number>(115200)
const errorMessage = ref<string>()
const autoConnect = ref<boolean>(localStorage.getItem('serial/auto-connect') === 'true')

findLastUsedPort().then(p => {
  console.log('Last Used Serial Port:', getPortIdString(p) ?? "[N/A]")
  if (p) port.value = p
})

// Watch for autoConnect changes and save to localStorage
watch(autoConnect, (value) => {
  localStorage.setItem('serial/auto-connect', String(value));
  if (value && port.value && connInfo.value === 'disconnected') {
    console.log('Auto-connecting Port:', getPortIdString(port.value))
    serial.connect(port.value, baud.value).catch(error => {
      console.error('Auto-connect failed:', error)
      errorMessage.value = `Auto-connect failed: ${error}`
      connInfo.value = 'disconnected';
    })
  }
})

// Watch for port changes and auto-connect if enabled
watch(port, async (p) => {
  if (autoConnect.value && p && connInfo.value === 'disconnected') {
    console.log('Auto-connecting Port:', getPortIdString(p))
    try {
      await serial.connect(p, baud.value)
    } catch (error) {
      console.error('Auto-connect failed:', error)
      errorMessage.value = `Auto-connect failed: ${error}`
    }
  }
})
</script>

<script setup lang="ts">
function clearError() {
  errorMessage.value = undefined
}

const portInfo = computed(() => getPortIdString(port.value) ?? '[No Port Selected]')

const isDisconnected = computed(() => connInfo.value === 'disconnected')

async function selectPort() {
  if (connInfo.value !== 'disconnected') return;
  clearError()
  try {
    if (!('serial' in navigator)) {
      errorMessage.value = 'Web Serial API not supported in this browser'
      return
    }
    port.value = await navigator.serial.requestPort();
    if (port.value) {
      // Save port info to local storage
      localStorage.setItem('serial/port', JSON.stringify(port.value.getInfo()))
    }
  } catch (error) {
    if (error instanceof DOMException && error.name === 'NotFoundError') {
      // No port selected, do nothing
    } else {
      errorMessage.value = `Port selection error: ${error}`
      port.value = null;
    }
  }
}
</script>

<template>
  <Window class="window" title="Serial Port Configuration" :style="{
    display: 'flex',
    flexDirection: 'column',
    gap: '1em',
  }">
    <label>
      Port
      <input type="text" readonly :value="portInfo" @click="selectPort" />
    </label>
    <label>
      Baudrate
      <input type="number" v-model="baud" :readonly="!isDisconnected"></input>
    </label>
    <div v-if="errorMessage" class="status">
      {{ errorMessage }}
    </div>
    <div class="serial-controls" :class="[connInfo]">
      <label style="flex-grow: 1">
        Auto Connect
        <Toggle v-model="autoConnect" />
      </label>
      <template v-if="['disconnected', 'connecting'].includes(connInfo)">
        <button @click="serial.connect(port!, baud)" :disabled="connInfo !== 'disconnected' || !port">Connect</button>
      </template>
      <template v-else>
        <button @click="serial.disconnect()" :disabled="connInfo !== 'connected'">Disconnect</button>
      </template>
    </div>
  </Window>
</template>

<style scoped>
.window {
  width: 400px;
  max-width: calc(100vw - 2rem);
  position: absolute;
  top: 1rem;
  right: 1rem;
}

.window label {
  display: flex;
  flex-direction: row;
  color: #ccc;
  font-size: 0.9rem;
  align-items: center;
  justify-content: space-between;
  font-family: 'Cascadia Code', 'Courier New', Courier, monospace;
}

.window label * {
  display: block;
  font-family: inherit;
}

.window label input {
  width: calc(100% - 14ch);
  padding: 0.4rem 0.5rem;
  background: #FFF1;
  box-shadow: none;
  border: none;
  border-radius: 4px;
  outline: 1px solid gray;
}

.serial-controls {
  color: #fff;
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
}

.serial-controls label {
  justify-content: flex-start;
  gap: 1rem;
}

.serial-controls.connecting,
.serial-controls.disconnected {
  --theme: #080;
}

.serial-controls.connected,
.serial-controls.disconnecting {
  --theme: #A00;
}

.serial-controls button {
  padding: 0.4rem 0.6rem;
  font-weight: 600;
  cursor: pointer;
  border-radius: 8px;
  border: 1px solid var(--theme);
  background: #111;
  color: var(--theme);
  filter: brightness(2) saturate(0.5);
  transition: all 0.1s;
  min-width: 16ch;
}

.serial-controls button:hover:not(:disabled) {
  color: white;
  background: var(--theme);
  filter: brightness(1) saturate(1);
}

.serial-controls button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.error-message {
  padding: 0.75rem;
  background: #2a2a2a;
  border-radius: 4px;
  color: #ff9a0c;
  font-size: 0.9rem;
  border: 1px solid #333;
}
</style>
