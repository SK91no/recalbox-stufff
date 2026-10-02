/**
 * @author Pit64
 */
import { defineStore } from 'pinia';
import { GLOBAL } from 'src/router/api.routes';
import { ArchitectureResponse } from 'stores/common/architecture.d';

export interface ArchitectureStoreState {
  _baseUrl: string;
  architecture: ArchitectureResponse;
}

export const useArchitectureStore = defineStore('architecture', {
  state: () => ({
    _baseUrl: GLOBAL.architecture,
    architecture: {},
  } as ArchitectureStoreState),
});
