/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { useTools } from 'src/composables/useTools';
import { CONFIGURATION } from 'src/router/api.routes';
import {
  UpdatesConfigOptionsResponse,
  UpdatesConfigResponse,
} from 'stores/configuration/updates.d';

const { displayableStringListToSelectOptions } = useTools();

export interface UpdatesStoreState {
  _baseUrl: string;
  _updatesOptions: UpdatesConfigOptionsResponse;
  updates: UpdatesConfigResponse;
}

export const useUpdatesStore = defineStore('updates', {
  state: () => ({
    _baseUrl: CONFIGURATION.updates,
    _updatesOptions: {
      type: {
        allowedStringList: [''],
        displayableStringList: [''],
      },
    },
    updates: {
      enabled: {
        value: true,
      },
      type: {
        value: 'stable',
      },
    },
  } as UpdatesStoreState),

  getters: {
    typeOptions: (state) => {
      const options = displayableStringListToSelectOptions(state._updatesOptions.type);

      return options.map((option) => ({
        ...option,
        value: option.value.toLowerCase(),
      }));
    },
  },
});
