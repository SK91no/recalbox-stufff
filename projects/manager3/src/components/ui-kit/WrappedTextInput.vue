<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-form autocomplete="off">
    <q-input
      :name="label"
      :label="t(label)"
      dense
      standout="bg-primary text-white"
      v-model="text"
      v-bind="$attrs"
      class="q-mb-md"
      :type="isPwd && password ? 'password' : 'text'"
      :mask="mask"
    >
      <template v-slot:append v-if="password">
        <q-icon
          :name="isPwd ? 'mdi-eye-off' : 'mdi-eye'"
          class="cursor-pointer"
          @click="isPwd = !isPwd"
        />
      </template>
      <template v-slot:after v-if="help">
        <HelpButton :warning="warning">
          <template v-slot:help>
            <slot name="help"></slot>
          </template>
        </HelpButton>
      </template>
    </q-input>
  </q-form>
</template>

<script lang="ts" setup>
import { toRefs, ref, computed } from 'vue';
import HelpButton from 'components/ui-kit/HelpButton.vue';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });

const props = defineProps({
  label: { type: String, required: true },
  help: { type: Boolean, default: false },
  warning: { type: Boolean },
  setter: { type: Function, required: true },
  getter: { type: Object, required: true },
  apiKey: { type: String, required: true },
  password: { type: Boolean },
  mask: { type: String },
});

const {
  label, help, warning, getter, setter, apiKey, password,
} = toRefs(props);

const timeout = ref();
const isPwd = ref<boolean>(true);

const text = computed({
  get: () => getter.value.value,
  set: (value) => {
    clearTimeout(timeout.value);

    timeout.value = setTimeout(() => {
      setter.value({ [apiKey.value]: value });
    }, 700);
  },
});
</script>
