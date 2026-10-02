<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-linear-progress
    v-for="item in items"
    size="60px"
    :value="item.used"
    :color="Dark.isActive ? '#3C435A' : 'accent'"
    class="q-mt-sm progress"
    :key="item.mount"
    :track-color="Dark.isActive ? '#1D2333' : 'white'"
    rounded
  >
    <div class="absolute-full flex items-center">
      <q-icon :name="item.icon" color="secondary"/>
      <div class="progress-text">
        {{ item.recalbox }} | {{ (item.used * 100).toFixed(2) }}% | {{ item.label }}
      </div>
    </div>
  </q-linear-progress>
</template>

<script lang="ts" setup>
import { Item } from 'stores/types/misc';
import { computed, toRefs } from 'vue';
import { Dark } from 'quasar';

const props = defineProps({
  data: { type: Object, required: true },
});

const { data } = toRefs(props);

const items = computed(() => {
  let result: Item[] = [];
  const storages = JSON.parse(JSON.stringify(data.value));

  Object.keys(storages).forEach((key) => {
    result = {
      ...result,
      [storages[key].mount]: {
        recalbox: storages[key].recalbox,
        icon: 'mdi-harddisk',
        mount: storages[key].mount,
        used: storages[key].used / storages[key].size,
        label: storages[key].label,
      },
    };
  });
  return result;
});
</script>

<style lang="sass">
body.body--dark
  .progress
    border: none

    .q-linear-progress__track
      opacity: 1
      background: $dark-grey-medium

    .q-linear-progress__model
      background: $dark-grey-light

    .progress-text
      color: $secondary

.progress
  border: 1px solid $rc-input-grey

  .q-linear-progress__model
    opacity: .5

  i
    font-size: .7em
    margin-left: .2em
    margin-right: .2em

  .progress-text
    display: flex
    align-items: center
    font-size: 12px!important
    font-weight: bold
    text-transform: capitalize
    height: 100%
    line-height: 28px
    color: $primary

    span
      vertical-align: middle
</style>
