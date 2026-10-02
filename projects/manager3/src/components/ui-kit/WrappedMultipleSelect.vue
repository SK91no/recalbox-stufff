<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-select
    :name="label"
    :label="t(label)"
    :options="options"
    class="q-mb-md"
    standout="bg-primary text-white"
    v-model="value"
    v-bind="$attrs"
    :clearable="clearable"
    multiple
    dense
    option-value="value"
    option-label="label"
    emit-value
    map-options
  >
    <template v-slot:selected-item />
    <template v-slot:option="{ itemProps, opt, selected, toggleOption }">
      <q-item v-bind="itemProps">
        <q-item-section>
          <q-item-label v-html="opt.label" />
        </q-item-section>
        <q-item-section side>
          <q-checkbox
            :model-value="selected"
            @update:model-value="toggleOption(opt.value)"
            color="accent"
          />
        </q-item-section>
      </q-item>
    </template>
    <template v-slot:no-option>
      <q-item>
        <q-item-section>
          {{ t('components.wrappedSelect.noResults') }}
        </q-item-section>
      </q-item>
    </template>
    <template v-slot:after v-if="help">
      <HelpButton :warning="warning">
        <template v-slot:help>
          <slot name="help"></slot>
        </template>
      </HelpButton>
    </template>
  </q-select>
</template>

<script lang="ts" setup>
import { SelectOption } from 'stores/types/misc';
import { computed, PropType, toRefs } from 'vue';
import HelpButton from 'components/ui-kit/HelpButton.vue';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });

const props = defineProps({
  label: { type: String, required: true },
  help: { type: Boolean, default: false },
  warning: { type: Boolean },
  options: { type: Array as PropType<SelectOption[]>, required: true, default: () => [] },
  setter: { type: Function, required: true },
  getter: { type: Object, required: true },
  apiKey: { type: String, required: true },
  clearable: { type: Boolean, default: false },
});

const {
  label, help, warning, options, getter, setter, apiKey,
} = toRefs(props);

const value = computed({
  get: () => getter?.value.value,
  set: (selected) => setter.value({ [apiKey?.value]: selected === null ? null : selected }),
});
</script>
