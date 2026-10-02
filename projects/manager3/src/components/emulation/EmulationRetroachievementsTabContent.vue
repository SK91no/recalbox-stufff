<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <FormFragmentContainer title="emulation.global.retroachievements.title">
    <template v-slot:content>
      <WrappedToggle
        label="emulation.global.retroachievements.activate.label"
        :getter="global.retroachievements"
        :setter="globalStore.post"
        apiKey="retroachievements"
        v-if="global.retroachievements"
        help
      >
        <template v-slot:help>
          {{ t('emulation.global.retroachievements.activate.help') }}
        </template>
      </WrappedToggle>

      <q-separator/>

      <WrappedTextInput
        label="emulation.global.retroachievements.username.label"
        :getter="global['retroachievements.username']"
        :setter="globalStore.post"
        apiKey="retroachievements.username"
        v-if="global['retroachievements.username']"
        help
      >
        <template v-slot:help>
          {{ t('emulation.global.retroachievements.username.help') }}
        </template>
      </WrappedTextInput>
      <WrappedTextInput
        label="emulation.global.retroachievements.password.label"
        :getter="global['retroachievements.password']"
        :setter="globalStore.post"
        apiKey="retroachievements.password"
        v-if="global['retroachievements.password']"
        password
        help
      >
        <template v-slot:help>
          {{ t('emulation.global.retroachievements.password.help') }}
        </template>
      </WrappedTextInput>

      <q-separator/>

      <WrappedToggle
        label="emulation.global.retroachievements.hardcore.label"
        warning
        :getter="global['retroachievements.hardcore']"
        :setter="globalStore.post"
        apiKey="retroachievements.hardcore"
        v-if="global['retroachievements.hardcore']"
        help
      >
        <template v-slot:help>
          <div v-html="t('emulation.global.retroachievements.hardcore.help')"></div>
        </template>
      </WrappedToggle>
    </template>
  </FormFragmentContainer>
</template>

<script lang="ts" setup>
import WrappedTextInput from 'components/ui-kit/WrappedTextInput.vue';
import WrappedToggle from 'components/ui-kit/WrappedToggle.vue';
import { useGlobalStore } from 'stores/configuration/global';
import { storeToRefs } from 'pinia';
import FormFragmentContainer from 'components/ui-kit/FormFragmentContainer.vue';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });

const globalStore = useGlobalStore();
globalStore.fetch();
const { global } = storeToRefs(globalStore);
</script>

<style lang="sass">

</style>
