/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { SYSTEM } from 'src/router/api.routes';
import { ServerResponse } from 'stores/common/server.d';

export interface ServerStoreState {
  available: boolean;
  server: ServerResponse;
}

export const useServerStore = defineStore('server', {
  state: () => ({
    available: false,
    server: {
      supportArchive: {},
    },
  } as ServerStoreState),

  actions: {
    async reboot(): Promise<void> {
      try {
        await this._api80Provider.post(SYSTEM.reboot);
      } catch (error) {
        console.error(error);
      }
    },
    async shutdown(): Promise<void> {
      try {
        await this._api80Provider.post(SYSTEM.shutdown);
      } catch (error) {
        console.error(error);
      }
    },

    async esStart(): Promise<void> {
      try {
        await this._api80Provider.post(SYSTEM.es.start);
        this.available = true;
      } catch (error) {
        console.error(error);
      }
    },
    async esShutdown(): Promise<void> {
      try {
        await this._api80Provider.post(SYSTEM.es.stop);
        this.available = false;
      } catch (error) {
        console.error(error);
      }
    },
    async esReboot(): Promise<void> {
      try {
        await this._api80Provider.post(SYSTEM.es.restart);
        this.available = true;
      } catch (error) {
        console.error(error);
      }
    },
    async supportArchive(): Promise<void> {
      try {
        const response = await this._api80Provider.get(SYSTEM.supportArchive);
        this.server = response.data;
      } catch (error) {
        console.error(error);
      }
    },
    async resetFactory(): Promise<void> {
      try {
        await this._api80Provider.post(SYSTEM.resetFactory);
        this.available = true;
      } catch (error) {
        // eslint-disable-next-line no-console
        console.log(error);
      }
    },
  },
});
