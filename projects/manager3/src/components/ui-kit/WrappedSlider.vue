<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-item>
    <q-item-section side v-if="icon">
      <q-icon color="accent" :name="icon" />
    </q-item-section>
    <q-item-section>
      <q-badge
        :color="`${Dark.isActive ? 'dark-grey-strong' : 'white'}`"
        class="text-primary"
        v-if="label"
        style="padding: 2px 0"
      >
        {{ t(label) }}
      </q-badge>
      <q-slider
        :name="label"
        :min="min"
        :step="step"
        :max="max"
        color="accent"
        label
        :model-value="value"
        @change="selected => {value = selected}"
        :disable="disable"
      />
    </q-item-section>
    <q-item-section side v-if="help" style="padding-left: 6px">
      <HelpButton :warning="warning" v-if="help">
        <template v-slot:help>
          <slot name="help"></slot>
        </template>
      </HelpButton>
    </q-item-section>
  </q-item>
</template>

<script lang="ts" setup>
import { computed, toRefs } from 'vue';
import HelpButton from 'components/ui-kit/HelpButton.vue';
import { useI18n } from 'vue-i18n';
import { Dark } from 'quasar';

const { t } = useI18n({ useScope: 'global' });

const props = defineProps({
  label: { type: String },
  help: { type: Boolean, default: false },
  warning: { type: Boolean },
  setter: { type: Function, required: true },
  getter: { type: Object, required: true },
  apiKey: { type: String, required: true },
  min: { type: Number, default: 0 },
  step: { type: Number, default: 1 },
  max: { type: Number, default: 100 },
  icon: { type: String },
  disable: { type: Boolean, default: false },
});

const {
  label, help, warning, getter, setter, apiKey, min, step, max, disable,
} = toRefs(props);

const value = computed({
  get: () => getter?.value.value,
  set: (selected) => setter.value({ [apiKey?.value]: selected === null ? 0 : selected }),
});
</script>

<style lang="sass" scoped>
.q-item
  padding: 0

  .q-slider
    z-index: 0
</style>
