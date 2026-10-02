/**
 * @author Pit64
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import {
  AutorunConfigOptionsResponse,
  AutorunConfigResponse,
} from 'stores/configuration/autorun.d';

export interface AutorunStoreState {
  _baseUrl: string;
  _autorunOptions: AutorunConfigOptionsResponse;
  autorun: AutorunConfigResponse;
}

export const useAutorunStore = defineStore('autorun', {
  state: () => ({
    _baseUrl: CONFIGURATION.autorun,
    _autorunOptions: {},
    autorun: {
      enabled: {
        value: false,
      },
      uuid: {
        value: '',
      },
      gamepath: {
        value: '',
      },
    },
  } as AutorunStoreState),

  getters: {},
});
