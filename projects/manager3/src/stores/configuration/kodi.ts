/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import { KodiConfigOptionsResponse, KodiConfigResponse } from 'stores/configuration/kodi.d';
import { useTools } from 'composables/useTools';

const { displayableStringListToSelectOptions } = useTools();

export interface KodiStoreState {
  _baseUrl: string;
  _kodiOptions: KodiConfigOptionsResponse;
  kodi: KodiConfigResponse;
}

export const useKodiStore = defineStore('kodi', {
  state: () => ({
    _baseUrl: CONFIGURATION.kodi,
    _kodiOptions: {
      'network.waithost': {
        allowedChars: '',
      },
      'network.waitmode': {
        allowedStringList: [] as string[],
      },
      'network.waittime': {
        lowerValue: 0,
        higherValue: 2147483647,
      },
      videomode: {
        allowedStringList: [] as string[],
        displayableStringList: [] as string[],
      },
    },
    kodi: {
      enabled: {
        value: true,
      },
      atstartup: {
        value: false,
      },
      xbutton: {
        value: false,
      },
      videomode: {
        value: '',
      },
      'network.waitmode': {
        value: 'required',
      },
    },
  } as KodiStoreState),

  getters: {
    videoModeOptions: (state) => displayableStringListToSelectOptions(state._kodiOptions.videomode),
    waitModeOptions: (state) => state._kodiOptions['network.waitmode'],
  },
});
