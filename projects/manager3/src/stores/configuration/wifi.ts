/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import { WifiConfigOptionsResponse, WifiConfigResponse } from 'stores/configuration/wifi.d';
import { useTools } from 'composables/useTools';

const { allowedStringListToSelectOptions } = useTools();

export interface WifiStoreState {
  _baseUrl: string;
  _wifiOptions: WifiConfigOptionsResponse;
  wifi: WifiConfigResponse;
}

export const useWifiStore = defineStore('wifi', {
  state: () => ({
    _baseUrl: CONFIGURATION.wifi,
    _wifiOptions: {
      region: {
        allowedStringList: [''],
      },
      priority: {
        lowerValue: 0,
        higherValue: 20,
      },
      netmask: {
        allowedChars: '',
      },
      ip: {
        allowedChars: '',
      },
      gateway: {
        allowedChars: '',
      },
      nameservers: {
        allowedChars: '',
      },
      connect: {
        value: false,
      },
    },
    wifi: {
      enabled: {
        value: false,
      },
      region: {
        value: 'JP',
      },
      priority: {},
      connect: {
        value: false,
      },
    },
  } as WifiStoreState),

  getters: {
    regionOptions: (state) => allowedStringListToSelectOptions(state._wifiOptions.region),
    priorityOptions: (state) => state._wifiOptions.priority,
  },
});
