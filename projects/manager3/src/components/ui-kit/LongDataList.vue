<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="col-12 container"
       v-bind:style="{ maxHeight: height }"
  >
    <div
      :key="index"
      @click="copy(`${index} : ${value}`)"
      class="row version-line"
      v-for="(value, index) in data"
    >
      <div
        class="col col-md-6 text-uppercase self-center"
        style="text-align: right; padding-right: 5%;"
      >
        <div>{{index}}</div>
      </div>
      <div class="col col-md-6" style="padding-left: 5%;">
        <q-chip color="secondary" dense icon="mdi-source-branch" square text-color="white">
          {{value}}
        </q-chip>
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { toRefs } from 'vue';
import { useTools } from 'composables/useTools';

const { copy } = useTools();

const props = defineProps({
  height: { type: String },
  data: { type: Object, required: true },
});

const {
  height, data,
} = toRefs(props);
</script>

<style lang="sass">
.container
  padding: 1em

  .version-line
    &:hover
      background: $rc-input-grey
      cursor: pointer

      .q-chip
        background: $primary !important
        color: $accent !important

@media(max-width: 700px)
  .container
    padding: 0
    .version-line
      .col-md-6
        padding-right: 0 !important
        padding-left: 0 !important
</style>
