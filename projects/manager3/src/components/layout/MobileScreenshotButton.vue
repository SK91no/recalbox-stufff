<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-btn
    flat
    icon="mdi-camera-plus"
    @click="useMediaStore().takeScreenshot()"
  />
</template>

<script lang="ts" setup>
import { useMediaStore } from 'stores/common/media';
</script>
