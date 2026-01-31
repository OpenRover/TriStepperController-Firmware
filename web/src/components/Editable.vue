<!-- ---------------------------------------------------------
 * Copyright (c) 2026 Yuxuan Zhang, web-dev@z-yx.cc
 * This source code is licensed under the MIT license.
 * You may find the full license in project root directory.
 --------------------------------------------------------- -->

<template>
  <span
    ref="input"
    :contenteditable="!disabled"
    @input="() => emit('update:modelValue', span?.textContent || '')"
  ></span>
</template>

<script setup lang="ts">
import { useTemplateRef, watch } from "vue";

const props = defineProps<{
  disabled?: boolean;
  modelValue: string;
}>();

const emit = defineEmits<{
  "update:modelValue": [string];
}>();
const span = useTemplateRef("input");

watch(span, (s) => {
  if (s) s.textContent = props.modelValue;
});

watch(
  () => props.modelValue,
  (v) => {
    if (span.value && span.value.textContent !== v) {
      span.value.textContent = v;
    }
  },
  { immediate: true },
);
</script>

<style scoped>
span:focus {
  outline: none;
  border-bottom: 1px solid currentColor;
}
</style>
