<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<script setup lang="ts">
import Layout from './components/Layout.vue'
import { FontAwesomeIcon as Icon } from '@fortawesome/vue-fontawesome'
import { faGears, faNetworkWired } from '@fortawesome/free-solid-svg-icons';
import SerialConfig, { connInfo } from './components/SerialConfig.vue';
import Overlay from './components/Overlay.vue';
import { computed } from 'vue';
import MotorConfigPanel from './components/MotorConfigPanel.vue';
import Control2D from './components/Control2D.vue';
const serialIconStyle = computed(() => {
  switch (connInfo.value) {
    case 'disconnected':
      return { color: 'red' }
    case 'connected':
      return { color: 'lime' }
    case 'connecting':
    case 'disconnecting':
      return { color: 'yellow' }
  }
})
</script>

<template>
  <Layout>
    <template #actions>
      <Overlay :overlay="MotorConfigPanel">
        <Icon :icon="faGears" />
      </Overlay>
      <Overlay :overlay="SerialConfig">
        <Icon :icon="faNetworkWired" :style="serialIconStyle" />
      </Overlay>
    </template>
    <Control2D />
  </Layout>
</template>

<style scoped>
pre {
  text-align: left;
}
</style>
