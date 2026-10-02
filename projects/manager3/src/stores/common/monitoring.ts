/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { MONITORING } from 'src/router/api.routes';
import { MonitoringResponse, StoragesResponse } from 'stores/common/monitoring.d';

export interface MonitoringStoreState {
  _baseUrl: string;
  monitoring: StoragesResponse;
  metrics: {
    temperatures: number[][];
    memory: number[][];
    cores: { x: string; y: number }[];
  };
}

export const useMonitoringStore = defineStore('monitoring', {
  state: () => ({
    _baseUrl: MONITORING.storageInfo,
    monitoring: {
      storages: {},
    },
    metrics: {
      temperatures: [],
      memory: [],
      cores: [],
    },
  } as MonitoringStoreState),

  actions: {
    getFilteredStorages() {
      return Object.fromEntries(Object.entries(this.monitoring.storages).filter(
        (value, key) => this.monitoring.storages[key].recalbox !== 'unknown'
          && this.monitoring.storages[key].recalbox !== 'system',
      ));
    },
    getSharePercent() {
      if (Object.keys(this.monitoring.storages).length > 0) {
        const { storages } = this.monitoring;
        let result: {used: number, size: number} = { used: 0, size: 0 };

        Object.keys(storages).map((key): {used: number, size: number} => {
          const s = storages[key];

          if (s.recalbox === 'share') {
            result = { used: s.used, size: s.size };
          }
          return { used: 0, size: 0 };
        });
        return Math.round((100 * result.used) / result.size);
      }
      return 0;
    },
    updateMonitoring(message: MonitoringResponse) {
      const temperatures = [
        ...this.metrics.temperatures,
        [
          (new Date(message.timestamp)).getTime(),
          parseFloat((message.temperature.temperatures.shift() ?? 0).toFixed(0)),
        ],
      ];

      const memory = [
        ...this.metrics.memory,
        [
          (new Date(message.timestamp)).getTime(),
          parseFloat(
            (((message.memory.total - (message.memory.available.shift() ?? 0)) / message.memory.total) * 100).toFixed(0),
          ),
        ],
      ];

      const cores: { x: string, y: number }[] = Object.keys(message.cpus).map(
        (core): { x: string, y: number } => ({
          x: `Core ${core}`,
          y: parseFloat((message.cpus[core].consumption.shift() ?? 0).toFixed(0)),
        }),
      );

      this.$patch({
        metrics: {
          cores,
          temperatures: temperatures.slice(-30),
          memory: memory.slice(-30),
        },
      });
    },
  },
});
