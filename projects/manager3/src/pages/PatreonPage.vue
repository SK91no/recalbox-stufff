<!--
@author Pit64
-->
<template>
  <q-page class="background patreon">
    <FormFragmentContainer title="patreon.title">
      <template v-slot:content>
        <WrappedTextInput
          label="patreon.privatekey.label"
          :getter="patreon.privatekey"
          :setter="patreonStore.post"
          apiKey="privatekey"
          v-if="patreon.privatekey"
          password
          help
        >
          <template v-slot:help>
            {{ t('patreon.privatekey.help') }}
          </template>
        </WrappedTextInput>
        <WrappedSelect
          label="patreon.scraper.source.label"
          :options="sourceOptions"
          :getter="scraper.source"
          :setter="scraperStore.post"
          apiKey="source"
          v-if="scraper.source"
          help
        >
          <template v-slot:help>
            {{ t('patreon.scraper.source.help') }}
          </template>
        </WrappedSelect>
        <WrappedToggle
          label="patreon.scraper.auto.label"
          :getter="scraper.auto"
          :setter="scraperStore.post"
          apiKey="auto"
          v-if="scraper.auto"
          help
        >
          <template v-slot:help>
            {{ t('patreon.scraper.auto.help') }}
          </template>
        </WrappedToggle>
        <WrappedToggle
          label="patreon.music.remoteplaylist.enable.label"
          :getter="music['remoteplaylist.enable']"
          :setter="musicStore.post"
          apiKey="remoteplaylist.enable"
          v-if="music['remoteplaylist.enable']"
          help
        >
          <template v-slot:help>
            {{ t('patreon.music.remoteplaylist.enable.help') }}
          </template>
        </WrappedToggle>
      </template>
    </FormFragmentContainer>
  </q-page>
</template>

<script lang="ts" setup>
import FormFragmentContainer from 'components/ui-kit/FormFragmentContainer.vue';
import WrappedTextInput from 'components/ui-kit/WrappedTextInput.vue';
import WrappedSelect from 'components/ui-kit/WrappedSelect.vue';
import WrappedToggle from 'components/ui-kit/WrappedToggle.vue';
import { storeToRefs } from 'pinia';
import { usePatreonStore } from 'stores/configuration/patreon';
import { useMusicStore } from 'stores/configuration/music';
import { useScraperStore } from 'stores/configuration/scraper';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });

const patreonStore = usePatreonStore();
patreonStore.fetch();
const {
  patreon,
} = storeToRefs(patreonStore);

const musicStore = useMusicStore();
musicStore.fetch();
const {
  music,
} = storeToRefs(musicStore);

const scraperStore = useScraperStore();
scraperStore.fetch();
scraperStore.fetchOptions();
const {
  sourceOptions,
  scraper,
} = storeToRefs(scraperStore);
</script>

<style lang="sass">
.patreon
  padding: 16px 8px

  &:before
    content: "\F0882"
</style>
