/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { BIOS } from 'src/router/api.routes';
import { Bios, BiosResponse } from 'stores/emulation/bios.d';

export interface BiosStoreState {
  _baseUrl: string;
  bios: BiosResponse;
}

export const useBiosStore = defineStore('bios', {
  state: () => ({
    _baseUrl: BIOS.all,
    bios: {},
  } as BiosStoreState),

  getters: {
    biosList: (state) => {
      const list: Bios[] = [];

      Object.keys(state.bios).forEach((systemName) => {
        Object.keys(state.bios[systemName].biosList).forEach((biosName) => {
          const biosList = state.bios[systemName].biosList[biosName];
          biosList.system = state.bios[systemName].fullName;
          list.push(biosList);
        });
      });

      return list;
    },
  },
});
