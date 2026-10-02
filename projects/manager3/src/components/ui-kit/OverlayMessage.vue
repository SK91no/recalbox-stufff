<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div :class="classes">
    <slot name="content"></slot>
  </div>
</template>

<script lang="ts" setup>
import { computed, toRefs } from 'vue';

const props = defineProps({
  background: { type: Boolean },
});

const {
  background,
} = toRefs(props);

const classes = computed(() => `overlayMessage${background.value ? ' background' : ''}`);
</script>

<style lang="sass" scoped>
body.body--dark
  .background
    background: rgba($dark-grey-medium, .8)

.background
  background: rgba($rc-light-grey, .8)

.overlayMessage
  position: absolute
  left: 0
  top: -1em
  width: 100%
  height: calc(100% + 1em)
  z-index: 10
  display: flex
  align-items: center
  justify-content: center
</style>
