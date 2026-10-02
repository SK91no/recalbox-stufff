<template>
  <div class="sleep zzz">Z</div>
  <div class="sleep zz">Z</div>
  <div class="sleep z">Z</div>
</template>

<script lang="ts" setup>

</script>

<style lang="sass" scoped>
.sleep
  width: 44px
  animation-name: zzz
  animation-duration: 2s
  animation-timing-function: ease-out
  animation-iteration-count: infinite
  color: $accent
  font-weight: bold
  z-index: 100
  transform: translateY(100%)

.z
  animation-delay: 0s
  right: 10px

.zz
  animation-delay: .5s
  right: -20px

.zzz
  animation-delay: 1s
  right: 0

@-webkit-keyframes zzz
  0%
    color: rgba($accent, 0)
    font-size: 2px
    -webkit-transform: translateY(100%)
    transform: translateY(100%)
  50%
    color: $accent
  100%
    color: rgba($accent, 0)
    font-size: 72px
    -webkit-transform: translateY(-100%)
    transform: translateY(-100%)

@keyframes zzz
  0%
    color: rgba($accent, 0)
    font-size: 2px
    -webkit-transform: translateY(100%)
    transform: translateY(100%)
  50%
    color: $accent
  100%
    color: rgba($accent, 0)
    font-size: 72px
    -webkit-transform: translateY(-100%)
    transform: translateY(-100%)
</style>
