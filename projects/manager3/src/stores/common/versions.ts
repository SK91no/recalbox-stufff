/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { GLOBAL } from 'src/router/api.routes';
import { VersionsResponse } from 'stores/common/versions.d';

export interface VersionsStoreState {
  _baseUrl: string;
  versions: VersionsResponse;
}

export const useVersionsStore = defineStore('versions', {
  state: () => ({
    _baseUrl: GLOBAL.versions,
    versions: {},
  } as VersionsStoreState),
});
