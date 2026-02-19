<script setup lang="ts">
import { m0, m1, m2 } from "lib/motor";
import { ref } from "vue";
const toggle = ref(false);
</script>

<template>
  <div
    style="width: 100%; overflow: hidden; overflow-y: scroll; text-wrap: nowrap"
  >
    <div class="banner" @click="toggle = !toggle">
      Diagnostics (click to {{ toggle ? "hide" : "show" }})
    </div>
    <div class="content" v-if="toggle">
      <div>
        Motor X<br />
        &nbsp;&nbsp;Queue Delay {{ m0.motion_delay.toFixed(2) }} ms<br />
        &nbsp;&nbsp;{{ m0.dispatched_motion_count }} dispatched, pending<br />
      </div>
      <div>
        Motor Y<br />
        &nbsp;&nbsp;Queue Delay {{ m1.motion_delay.toFixed(2) }} ms<br />
        &nbsp;&nbsp;{{ m1.dispatched_motion_count }} dispatched, pending<br />
      </div>
      <div>
        Motor Z<br />
        &nbsp;&nbsp;Queue Delay {{ m2.motion_delay.toFixed(2) }} ms<br />
        &nbsp;&nbsp;{{ m2.dispatched_motion_count }} dispatched, pending<br />
      </div>
    </div>
  </div>
</template>

<style scoped lang="scss">
.banner {
  background-color: var(--theme);
  color: #aaa;
  font-weight: 600;
  cursor: pointer;
  user-select: none;
}

.content {
  margin: 0.4rem 0;
  padding: 0 0.8rem;
  border-left: 2px solid #888;
  color: #aaa;
  line-height: 1.5;
  font-family: "Cascadia Code", "Courier New", Courier, monospace;
  & > :not(:first-child) {
    margin-top: 1ch;
  }
}
</style>
