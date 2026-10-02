/**
 * @author Pit64
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import {
  FrontendConfigOptionsResponse,
  FrontendConfigResponse,
} from 'stores/configuration/frontend.d';

export interface FrontendStoreState {
  _baseUrl: string;
  _frontendOptions: FrontendConfigOptionsResponse;
  frontend: FrontendConfigResponse;
}

export const useFrontendStore = defineStore('frontend', {
  state: () => ({
    _baseUrl: CONFIGURATION.frontend,
    _frontendOptions: {},
    frontend: {
      enableeditfavorites: {
        value: true,
      },
      listfastmovenabled: {
        value: true,
      },
    },
  } as FrontendStoreState),

  getters: {},
});
