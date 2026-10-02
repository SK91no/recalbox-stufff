<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-btn
    padding="16px"
    flat
    square
    icon="mdi-camera-plus"
    color="white"
    id="virtual-devices-button"
    @click="useMediaStore().takeScreenshot()"
    style="margin-right: 1em; border-radius: 4px;"
  >
    <q-tooltip class="bg-primary" anchor="top middle" self="bottom middle" :offset="[10, 10]">
      {{ t('emulation.screenshots.tooltip') }}
    </q-tooltip>
  </q-btn>
</template>

<script lang="ts" setup>
import { useMediaStore } from 'stores/common/media';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });
</script>
