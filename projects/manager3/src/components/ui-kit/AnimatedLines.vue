<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="animatedLines">
    <div
      v-for="(color, index) in colors"
      :class="`animatedLine ${index}`"
      :key="color"
      :style="{ backgroundColor: color }"
    />
  </div>
</template>

<script lang="ts" setup>
import { toRefs } from 'vue';

const props = defineProps({
  colors: { type: Object, required: true },
});
const { colors } = toRefs(props);
</script>

<style lang="sass">
.animatedLines
  position: absolute
  display: flex
  flex-direction: row
  top: -1em
  left: 8px
  width: 12%
  height: calc(110% + 1em)
  opacity: 0.3
  transform: rotate(357deg)

  .animatedLine
    animation-name: grow
    width: 20%
    height: 0
    animation-duration: .4s
    animation-timing-function: ease-in
    border-bottom-left-radius: 5px
    border-bottom-right-radius: 5px
    animation-fill-mode: forwards

    @keyframes grow
      from
        height: 0
      to
        height: 110%

  .band1
    animation-delay: 250ms
  .band2
    animation-delay: 350ms
  .band3
    animation-delay: 450ms
  .band4
    animation-delay: 550ms
</style>
