<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <FormFragmentContainer title="emulation.global.netplay.title">
    <template v-slot:content>
      <WrappedToggle
        label="emulation.global.netplay.activate.label"
        :getter="global.netplay"
        :setter="globalStore.post"
        apiKey="netplay"
        v-if="global.netplay"
        help
      >
        <template v-slot:help>
          {{ t('emulation.global.netplay.activate.help') }}
        </template>
      </WrappedToggle>
      <WrappedTextInput
        label="emulation.global.netplay.nickname.label"
        warning
        :getter="global['netplay.nickname']"
        :setter="globalStore.post"
        apiKey="netplay.nickname"
        v-if="global['netplay.nickname']"
        help
      >
        <template v-slot:help>
          {{ t('emulation.global.netplay.nickname.help') }}
        </template>
      </WrappedTextInput>
      <WrappedTextInput
        label="emulation.global.netplay.port.label"
        :getter="global['netplay.port']"
        :setter="globalStore.post"
        apiKey="netplay.port"
        v-if="global['netplay.port']"
        help
      >
        <template v-slot:help>
          {{ t('emulation.global.netplay.port.help') }}
        </template>
      </WrappedTextInput>
      <WrappedSelect
        label="emulation.global.netplay.relay.label"
        :options="translatedNetplayRelayOptions"
        :getter="global['netplay.relay']"
        :setter="globalStore.post"
        apiKey="netplay.relay"
        v-if="global['netplay.relay']"
        help
      >
        <template v-slot:help>
          {{ t('emulation.global.netplay.relay.help') }}
        </template>
      </WrappedSelect>
      <WrappedTextInput
        label="emulation.global.netplay.lobby.label"
        :getter="global['netplay.lobby']"
        :setter="globalStore.post"
        apiKey="netplay.lobby"
        v-if="global['netplay.lobby']"
        help
      >
        <template v-slot:help>
          {{ t('emulation.global.netplay.lobby.help') }}
        </template>
      </WrappedTextInput>
    </template>
  </FormFragmentContainer>
</template>

<script lang="ts" setup>
import WrappedToggle from 'components/ui-kit/WrappedToggle.vue';
import WrappedTextInput from 'components/ui-kit/WrappedTextInput.vue';
import WrappedSelect from 'components/ui-kit/WrappedSelect.vue';
import { useTools } from 'composables/useTools';
import { useGlobalStore } from 'stores/configuration/global';
import { storeToRefs } from 'pinia';
import FormFragmentContainer from 'components/ui-kit/FormFragmentContainer.vue';
import { useI18n } from 'vue-i18n';
import { computed } from 'vue';

const { t } = useI18n({ useScope: 'global' });
const { translatedAllowedStringListToSelectOptions } = useTools();

const globalStore = useGlobalStore();
globalStore.fetch();
globalStore.fetchOptions();
const {
  netplayRelayOptions,
  global,
} = storeToRefs(globalStore);

const translatedNetplayRelayOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'emulation.global.netplay.relay.options',
  netplayRelayOptions.value,
));
</script>

<style lang="sass">

</style>
