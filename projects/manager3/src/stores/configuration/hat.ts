/**
 * @author Pit64
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import {
  HatConfigOptionsResponse,
  HatConfigResponse,
} from 'stores/configuration/hat.d';

export interface HatStoreState {
  _baseUrl: string;
  _hatOptions: HatConfigOptionsResponse;
  hat: HatConfigResponse;
}

export const useHatStore = defineStore('hat', {
  state: () => ({
    _baseUrl: CONFIGURATION.hat,
    _hatOptions: {
      'wpaf.board': {
        allowedStringList: [''],
      },
    },
    hat: {
      'wpaf.enabled': {
        value: false,
      },
    },
  } as HatStoreState),

  getters: {
    boardOptions: (state) => state._hatOptions['wpaf.board'],
  },
});
