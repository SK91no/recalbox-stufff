<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-btn
    :color="warning ? 'warning' : (Dark.isActive ? 'dark-grey-light' : 'light-blue')"
    @click="() => helpOpen = true"
    flat
    :icon="warning ? 'mdi-alert' : 'mdi-help-circle-outline'"
    round
    size="md"
    :class="toggle ? 'help-button' : ''"
  >
    <q-tooltip
      class="bg-primary"
      :offset="[10, 10]"
      content-class="bg-primary"
      content-style="font-size: 16px"
    >
      {{ t('general.helpButton.tooltip') }}
    </q-tooltip>
  </q-btn>
  <q-dialog transition-hide="slide-down" transition-show="slide-up" v-model="helpOpen">
    <q-card :class="`${className} background help-dialog`">
      <q-card-section class="text-justify" style="white-space: pre-line;">
        <slot name="help"></slot>
      </q-card-section>
    </q-card>
  </q-dialog>
</template>

<script lang="ts" setup>
import { ref, toRefs } from 'vue';
import { useI18n } from 'vue-i18n';
import { Dark } from 'quasar';

const { t } = useI18n({ useScope: 'global' });

const helpOpen = ref<boolean>(false);

const props = defineProps({
  warning: { type: Boolean },
  toggle: { type: Boolean },
});

const { warning, toggle } = toRefs(props);

const className = `bg-primary text-white ${warning.value ? 'help-dialog-card-warning' : 'help-dialog-card'}`;
</script>

<style lang="sass">
body.body--dark
  .help-dialog
    background: $dark-grey-strong!important

    &:before
      color: $dark-grey-stronger

@keyframes helpDialogSlidein
  from
    right: -10rem
    bottom: -10rem

  to
    right: -30px
    bottom: 0

.help-dialog
  overflow: hidden!important
  &:before
    font-size: 14em
    bottom: 0
    right: -30px
    color: $secondary
    opacity: .3
    animation-duration: .5s
    animation-name: helpDialogSlidein

.help-button
  margin-bottom: 1em
  margin-left: 6px

.help-dialog-card
  border-left: 6px solid $accent

  &:before
    content: "\F0625"

.help-dialog-card-warning
  border-left: 6px solid $warning

  &:before
    content: "\F0026"
</style>
