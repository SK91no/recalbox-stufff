/**
 * @author Pit64
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import {
  LircdConfigOptionsResponse,
  LircdConfigResponse,
} from 'stores/configuration/lircd.d';

export interface LircdStoreState {
  _baseUrl: string;
  _lircdOptions: LircdConfigOptionsResponse;
  lircd: LircdConfigResponse;
}

export const useLircdStore = defineStore('lircd', {
  state: () => ({
    _baseUrl: CONFIGURATION.lircd,
    _lircdOptions: {},
    lircd: {
      enabled: {
        value: true,
      },
    },
  } as LircdStoreState),
});
