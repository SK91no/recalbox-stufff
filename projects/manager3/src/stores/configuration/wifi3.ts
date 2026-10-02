/**
 * @author Pit64
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import { WifiBaseConfigOptionsResponse, WifiBaseConfigResponse } from 'stores/configuration/wifi.d';

export interface Wifi3StoreState {
  _baseUrl: string;
  _wifi3Options: WifiBaseConfigOptionsResponse;
  wifi3: WifiBaseConfigResponse;
}

export const useWifi3Store = defineStore('wifi3', {
  state: () => ({
    _baseUrl: CONFIGURATION.wifi3,
    _wifi3Options: {
    },
    wifi3: {},
  } as Wifi3StoreState),

  getters: {},
});
