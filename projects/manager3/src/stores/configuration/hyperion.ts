/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import { HyperionConfigResponse } from 'stores/configuration/hyperion.d';

export interface HyperionStoreState {
  _baseUrl: string;
  hyperion: HyperionConfigResponse;
}

export const useHyperionStore = defineStore('hyperion', {
  state: () => ({
    _baseUrl: CONFIGURATION.hyperion,
    hyperion: {
      enabled: {
        value: false,
      },
    },
  } as HyperionStoreState),
});
