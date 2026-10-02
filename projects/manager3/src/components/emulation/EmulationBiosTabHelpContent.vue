<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="help-line row">
    <div class="col-1">
      <q-icon name="mdi-checkbox-marked-circle-outline" color="positive" size="sm"/>
    </div>
    <div class="col">{{ t('emulation.bios.help.green.title') }}</div>
  </div>
  <div class="help-line row">
    <div class="col-1">
      <q-icon name="mdi-checkbox-marked-circle-outline" color="orange-4" size="sm"/>
    </div>
    <div class="col">
      {{ t('emulation.bios.help.yellow_1.title') }}<br>
      <span>
        {{ t('emulation.bios.help.yellow_1.text') }}
      </span>
    </div>
  </div>
  <div class="help-line row">
    <div class="col-1">
      <q-icon name="mdi-checkbox-marked-circle-outline" color="orange-4" size="sm"/>
    </div>
    <div class="col">
      {{ t('emulation.bios.help.yellow_2.title') }}<br>
      <span>
        {{ t('emulation.bios.help.yellow_2.text') }}
      </span>
    </div>
  </div>
  <div class="row">
    <div class="col-1">
      <q-icon name="mdi-checkbox-marked-circle-outline" color="negative" size="sm"/>
    </div>
    <div class="col">{{ t('emulation.bios.help.red.title') }}</div>
  </div>
</template>

<script lang="ts" setup>
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });
</script>

<style lang="sass">
.help-line
  margin-bottom: 1em

  span
    color: $dark-blue
</style>
