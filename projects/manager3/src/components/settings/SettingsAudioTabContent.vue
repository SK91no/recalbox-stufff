<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row">
    <div class="col col-xs-12 col-sm-12 col-md-12">
      <FormFragmentContainer title="settings.audio.backgroundMusic.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.audio.backgroundMusic.toggleButtonLabel"
            help
            warning
            :getter="audio.bgmusic"
            :setter="audioStore.post"
            apiKey="bgmusic"
            v-if="audio.bgmusic"
          >
            <template v-slot:help>
              {{ t('settings.audio.backgroundMusic.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.audio.audioOutput.title">
        <template v-slot:content>
          <WrappedSelect
            label="settings.audio.audioOutput.select.options.label"
            :options="deviceOptions"
            :getter="audio.device"
            :setter="audioStore.post"
            apiKey="device"
            v-if="audio.device"
            help
          >
            <template v-slot:help>
              {{ t('settings.audio.audioOutput.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.audio.volume.title">
        <template v-slot:content>
          <WrappedSlider
            :getter="audio.volume"
            :setter="audioStore.post"
            apiKey="volume"
            v-if="audio.volume"
            :min="volumeOptions.lowerValue"
            :max="volumeOptions.higherValue"
            icon="mdi-volume-high"
            help
          >
            <template v-slot:help>
              {{ t('settings.audio.volume.help') }}
            </template>
          </WrappedSlider>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.audio.music.volume.title">
        <template v-slot:content>
          <WrappedSlider
            :getter="audio['music.volume']"
            :setter="audioStore.post"
            apiKey="music.volume"
            v-if="audio['music.volume']"
            :mix="musicVolumeOptions.lowerValue"
            :max="musicVolumeOptions.higherValue"
            icon="mdi-volume-high"
            help
          >
            <template v-slot:help>
              {{ t('settings.audio.music.volume.help') }}
            </template>
          </WrappedSlider>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.audio.mode.title">
        <template v-slot:content>
          <WrappedSelect
            label="settings.audio.mode.label"
            :options="translatedModeOptions"
            :getter="audio.mode"
            :setter="audioStore.post"
            apiKey="mode"
            v-if="audio.mode"
            help
          >
            <template v-slot:help>
              {{ t('settings.audio.mode.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>
    </div>
  </div>
</template>

<script lang="ts" setup>
import FormFragmentContainer from 'components/ui-kit/FormFragmentContainer.vue';
import WrappedSelect from 'components/ui-kit/WrappedSelect.vue';
import WrappedToggle from 'components/ui-kit/WrappedToggle.vue';
import { useTools } from 'composables/useTools';
import { useAudioStore } from 'stores/configuration/audio';
import { storeToRefs } from 'pinia';
import WrappedSlider from 'components/ui-kit/WrappedSlider.vue';
import { useI18n } from 'vue-i18n';
import { computed } from 'vue';

const { translatedAllowedStringListToSelectOptions } = useTools();

const { t } = useI18n({ useScope: 'global' });

const audioStore = useAudioStore();
audioStore.fetch();
audioStore.fetchOptions();
const {
  deviceOptions,
  volumeOptions,
  musicVolumeOptions,
  modeOptions,
  audio,
} = storeToRefs(audioStore);

const translatedModeOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.audio.mode.options',
  modeOptions.value,
));
</script>
