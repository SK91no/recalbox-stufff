/**
 * @author Pit64
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import {
  PinballConfigOptionsResponse, PinballConfigResponse,
} from 'stores/emulation/pinball.d';

export interface PinballStoreState {
  _baseUrl: string;
  _pinballOptions: PinballConfigOptionsResponse;
  pinball: PinballConfigResponse;
}

export const usePinballStore = defineStore('pinball', {
  state: () => ({
    _baseUrl: CONFIGURATION.pinball,
    _pinballOptions: {
      'screen1.resolution': {
        allowedStringList: [''],
      },
      'screen2.resolution': {
        allowedStringList: [''],
      },
    },
    pinball: {
      screen1: {
        value: '',
      },
      'screen1.resolution': {
        value: '',
      },
      screen2: {
        value: '',
      },
      'screen2.resolution': {
        value: '',
      },
      cabinet: {
        value: false,
      },
    },
  } as PinballStoreState),

  getters: {
    'screen1.resolution': (state) => state._pinballOptions['screen1.resolution'],
    'screen2.resolution': (state) => state._pinballOptions['screen2.resolution'],
  },
});
