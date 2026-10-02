<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row items-stretch line">
    <div class="col-12 col-md-4 title vertical-middle text-right">
      {{ t(title) }}
    </div>
    <div class="col-12 col-md-8 content">
      <slot name="content"></slot>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { toRefs } from 'vue';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });

const props = defineProps({
  title: { type: String, required: true, default: 'title' },
});

const { title } = toRefs(props);
</script>

<style lang="sass">
body.body--dark
  .line
    background: $dark-grey-strong!important

    .title
      color: $dark-grey-light

.line
  margin: 0 8px 1em
  background: white

  .title
    text-transform: uppercase
    font-weight: 100
    font-size: x-large
    padding: .3em 1em

  .content
    padding: 1em
</style>
