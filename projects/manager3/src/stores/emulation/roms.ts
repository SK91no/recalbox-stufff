/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { getPath, ROMS, SYSTEMS } from 'src/router/api.routes';
import { Rom } from 'src/stores/emulation/roms.d';

export interface RomsStoreState {
  roms: Rom[]|[],
  total: number,
}

export const useRomsStore = defineStore('roms', {
  state: () => ({
    roms: [],
    total: 0,
  } as RomsStoreState),

  actions: {
    async getRomsCount() {
      try {
        const response = await this._apiProvider.get(ROMS.total);
        this.total = response.data.total;
      } catch (error) {
        console.error(error);
      }
    },
    async fetchBySystem(system: string) {
      try {
        const response = await this._apiProvider.get(getPath(SYSTEMS.roms, { systemName: system }));
        this.roms = response.data.roms;
      } catch (error) {
        console.error(error);
      }
    },
  },
});
