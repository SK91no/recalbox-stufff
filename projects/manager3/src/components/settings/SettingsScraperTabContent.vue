<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row">
    <div class="col col-xs-12 col-sm-12 col-md-6">
      <FormFragmentContainer title="settings.scraper.settings.title">
        <template v-slot:content>
          <div class="col col-xs-12 col-sm-12 col-md-12">
            <WrappedSelect
              label="settings.scraper.settings.getNameFrom.title"
              :options="translatedGetNameFromOptions"
              :getter="scraper.getnamefrom"
              :setter="scraperStore.post"
              apiKey="getnamefrom"
              v-if="scraper.getnamefrom"
              help
            >
              <template v-slot:help>
                {{ t('settings.scraper.settings.getNameFrom.help') }}
              </template>
            </WrappedSelect>
          </div>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.scraper.screenscraper.medias.title">
        <template v-slot:content>
          <div class="col col-xs-12 col-sm-12 col-md-12">
            <WrappedSelect
              label="settings.scraper.screenscraper.medias.media.title"
              :options="translatedMediaFromOptions"
              :getter="scraper['screenscraper.media']"
              :setter="scraperStore.post"
              apiKey="screenscraper.media"
              v-if="scraper['screenscraper.media']"
              clearable
              help
            >
              <template v-slot:help>
                {{ t('settings.scraper.screenscraper.medias.media.help') }}
              </template>
            </WrappedSelect>
            <WrappedSelect
              label="settings.scraper.screenscraper.medias.thumbnail.title"
              :options="translatedThumbnailOptions"
              :getter="scraper['screenscraper.thumbnail']"
              :setter="scraperStore.post"
              apiKey="screenscraper.thumbnail"
              v-if="scraper['screenscraper.thumbnail']"
              clearable
              help
            >
              <template v-slot:help>
                {{ t('settings.scraper.screenscraper.medias.thumbnail.help') }}
              </template>
            </WrappedSelect>
            <WrappedSelect
              label="settings.scraper.screenscraper.medias.video.title"
              :options="translatedVideoOptions"
              :getter="scraper['screenscraper.video']"
              :setter="scraperStore.post"
              apiKey="screenscraper.video"
              v-if="scraper['screenscraper.video']"
              clearable
              help
            >
              <template v-slot:help>
                {{ t('settings.scraper.screenscraper.medias.video.help') }}
              </template>
            </WrappedSelect>
            <WrappedToggle
              label="settings.scraper.screenscraper.medias.maps.title"
              :getter="scraper['screenscraper.maps']"
              :setter="scraperStore.post"
              apiKey="screenscraper.maps"
              v-if="scraper['screenscraper.maps']"
              help
            >
              <template v-slot:help>
                {{ t('settings.scraper.screenscraper.medias.maps.help') }}
              </template>
            </WrappedToggle>
            <WrappedToggle
              label="settings.scraper.screenscraper.medias.manual.title"
              :getter="scraper['screenscraper.manual']"
              :setter="scraperStore.post"
              apiKey="screenscraper.manual"
              v-if="scraper['screenscraper.manual']"
              help
            >
              <template v-slot:help>
                {{ t('settings.scraper.screenscraper.medias.manual.help') }}
              </template>
            </WrappedToggle>
            <WrappedToggle
              label="settings.scraper.screenscraper.medias.p2k.title"
              :getter="scraper['screenscraper.p2k']"
              :setter="scraperStore.post"
              apiKey="screenscraper.p2k"
              v-if="scraper['screenscraper.p2k']"
              help
            >
              <template v-slot:help>
                {{ t('settings.scraper.screenscraper.medias.p2k.help') }}
              </template>
            </WrappedToggle>
          </div>
        </template>
      </FormFragmentContainer>
    </div>
    <div class="col col-xs-12 col-sm-12 col-md-6">
      <FormFragmentContainer title="settings.scraper.screenscraper.auth.title">
        <template v-slot:content>
          <div class="col col-xs-12 col-sm-12 col-md-12">
            <WrappedTextInput
              label="settings.scraper.screenscraper.auth.user.title"
              :getter="scraper['screenscraper.user']"
              :setter="scraperStore.post"
              apiKey="screenscraper.user"
              v-if="scraper['screenscraper.user']"
              help
            >
              <template v-slot:help>
                {{ t('settings.scraper.screenscraper.auth.user.help') }}
              </template>
            </WrappedTextInput>
            <WrappedTextInput
              label="settings.scraper.screenscraper.auth.password.title"
              :getter="scraper['screenscraper.password']"
              :setter="scraperStore.post"
              apiKey="screenscraper.password"
              v-if="scraper['screenscraper.password']"
              password
              help
            >
            <template v-slot:help>
              {{ t('settings.scraper.screenscraper.auth.password.help') }}
            </template>
            </WrappedTextInput>
          </div>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.scraper.screenscraper.i18n.title">
        <template v-slot:content>
          <div class="col col-xs-12 col-sm-12 col-md-12">
            <WrappedSelect
              label="settings.scraper.screenscraper.i18n.language.title"
              :options="languageOptions"
              :getter="scraper['screenscraper.language']"
              :setter="scraperStore.post"
              apiKey="screenscraper.language"
              v-if="scraper['screenscraper.language']"
              help
            >
            <template v-slot:help>
              {{ t('settings.scraper.screenscraper.i18n.language.help') }}
            </template>
            </WrappedSelect>
            <WrappedSelect
              label="settings.scraper.screenscraper.i18n.region.title"
              :options="translatesRegionOptions"
              :getter="scraper['screenscraper.region']"
              :setter="scraperStore.post"
              apiKey="screenscraper.region"
              v-if="scraper['screenscraper.region']"
              help
            >
            <template v-slot:help>
              {{ t('settings.scraper.screenscraper.i18n.region.help') }}
            </template>
            </WrappedSelect>
            <WrappedSelect
              label="settings.scraper.screenscraper.i18n.regionPriority.title"
              :options="translatedRegionPriorityOptions"
              :getter="scraper['screenscraper.regionPriority']"
              :setter="scraperStore.post"
              apiKey="screenscraper.regionPriority"
              v-if="scraper['screenscraper.regionPriority']"
              help
            >
              <template v-slot:help>
                {{ t('settings.scraper.screenscraper.i18n.regionPriority.help') }}
              </template>
            </WrappedSelect>
          </div>
        </template>
      </FormFragmentContainer>
    </div>
  </div>
</template>

<script lang="ts" setup>
import FormFragmentContainer from 'components/ui-kit/FormFragmentContainer.vue';
import { useTools } from 'composables/useTools';
import { useScraperStore } from 'stores/configuration/scraper';
import { storeToRefs } from 'pinia';
import WrappedSelect from 'components/ui-kit/WrappedSelect.vue';
import WrappedTextInput from 'components/ui-kit/WrappedTextInput.vue';
import WrappedToggle from 'components/ui-kit/WrappedToggle.vue';
import { useI18n } from 'vue-i18n';
import { computed } from 'vue';

const { t } = useI18n({ useScope: 'global' });
const { translatedAllowedStringListToSelectOptions } = useTools();

const scraperStore = useScraperStore();
scraperStore.fetch();
const {
  scraper,
  getNameFromOptions,
  languageOptions,
  regionOptions,
  mediaOptions,
  thumbnailOptions,
  videoOptions,
  regionPriorityOptions,
} = storeToRefs(scraperStore);

const translatedGetNameFromOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.scraper.settings.getNameFrom.options',
  getNameFromOptions.value,
));

const translatedMediaFromOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.scraper.screenscraper.medias.media.options',
  mediaOptions.value,
));

const translatedThumbnailOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.scraper.screenscraper.medias.thumbnail.options',
  thumbnailOptions.value,
));

const translatedVideoOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.scraper.screenscraper.medias.video.options',
  videoOptions.value,
));

const translatesRegionOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'regions.list',
  regionOptions.value
));

const translatedRegionPriorityOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.scraper.screenscraper.i18n.regionPriority.options',
  regionPriorityOptions.value,
));
</script>

<style lang="sass" scoped>
.q-tab-panel > .row
  margin-bottom: 5em
</style>
