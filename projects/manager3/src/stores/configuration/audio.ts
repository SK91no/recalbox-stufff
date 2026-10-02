/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { AudioConfigOptionsResponse, AudioConfigResponse } from 'stores/configuration/audio.d';
import { CONFIGURATION } from 'src/router/api.routes';
import { useTools } from 'composables/useTools';

const { displayableStringListToSelectOptions } = useTools();

export interface AudioStoreState {
  _baseUrl: string;
  _audioOptions: AudioConfigOptionsResponse;
  audio: AudioConfigResponse;
}

export const useAudioStore = defineStore('audio', {
  state: () => ({
    _baseUrl: CONFIGURATION.audio,
    _audioOptions: {
      device: {
        allowedStringList: [] as string[],
        displayableStringList: [] as string[],
      },
      volume: {
        lowerValue: 0,
        higherValue: 100,
      },
      'music.volume': {
        lowerValue: 0,
        higherValue: 100,
      },
      mode: {
        allowedStringList: [] as string[],
      },
    },
    audio: {
      volume: {
        value: 90,
      },
      bgmusic: {
        value: true,
      },
    },
  } as AudioStoreState),

  getters: {
    deviceOptions: (state) => displayableStringListToSelectOptions(state._audioOptions.device),
    volumeOptions: (state) => state._audioOptions.volume,
    musicVolumeOptions: (state) => state._audioOptions['music.volume'],
    modeOptions: (state) => state._audioOptions.mode,
  },
});
