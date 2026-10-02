<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-list v-if="loader" separator>
    <q-item v-for="index in iterations" :key="index">
      <q-item-section avatar>
        <q-skeleton type="QToggle" width="40px"/>
      </q-item-section>
      <q-item-section>
        <q-item-label><q-skeleton type="rect" height="35px"/></q-item-label>
        <q-item-label caption><q-skeleton type="text" width="150px"/></q-item-label>
      </q-item-section>
    </q-item>
  </q-list>
</template>

<script lang="ts" setup>
import { toRefs } from 'vue';

const props = defineProps({
  iterations: { type: Number, required: true },
  loader: { type: Boolean, required: true },
});

const { iterations, loader } = toRefs(props);
</script>
