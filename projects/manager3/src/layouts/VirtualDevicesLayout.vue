<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-layout>
    <q-page-container>
      <div class="lines"></div>
      <router-view />
    </q-page-container>
  </q-layout>
</template>

<style lang="sass">
.q-layout
  overflow: hidden

  .q-page-container
    height: 100vh
    background: $rc-light-grey

    .lines
      border-left: 38em solid $primary
      border-right: 1.5em solid $accent
      height: 145vh
      position: absolute
      transform: rotate(18deg)
      top: -24em
      left: -19em
</style>
<script setup lang="ts">
</script>
