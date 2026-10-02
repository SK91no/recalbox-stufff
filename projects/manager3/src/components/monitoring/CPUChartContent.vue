<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="chart-cpu">
    <apexchart
      :options="options"
      :series="data"
      height="300"
    />
  </div>
</template>

<script lang="ts" setup>
import { computed, toRefs } from 'vue';
import { date, getCssVar, useQuasar } from 'quasar';
import { useI18n } from 'vue-i18n';

const $q = useQuasar();

const { t } = useI18n({ useScope: 'global' });

const props = defineProps({
  data: { type: Array, required: true },
});

const { data } = toRefs(props);

const options = computed(() => ({
  chart: {
    type: 'line',
    animations: { enabled: false, easing: 'linear', dynamicAnimation: { speed: 1000 } },
    toolbar: { show: false },
    zoom: { enabled: false },
    foreColor: $q.dark.isActive ? '#3C435A' : '',
  },
  dataLabels: { enabled: false },
  tooltip: { enabled: false },
  stroke: {
    show: true, width: 2, colors: [getCssVar('accent'), getCssVar('positive')],
  },
  markers: { size: 0 },
  yaxis: [
    {
      min: 0,
      max: 100,
      decimalsInFloat: 0,
      title: {
        text: t('monitoring.cpuVsMemory.temperature.title'), style: { color: getCssVar('accent') },
      },
    },
    {
      opposite: true,
      min: 0,
      max: 100,
      decimalsInFloat: 0,
      title: {
        text: t('monitoring.cpuVsMemory.memory.title'), style: { color: getCssVar('positive') },
      },
    },
  ],
  xaxis: {
    axisBorder: {
      show: true,
      color: $q.dark.isActive ? '#3C435A' : '',
    },
    axisTicks: {
      show: true,
      color: $q.dark.isActive ? '#3C435A' : '',
    },
    labels: {
      formatter(value: number, timestamp: number) {
        return date.formatDate(timestamp, 'HH:mm:ss');
      },
      trim: true,
    },
    tooltip: { enabled: false },
  },
  grid: {
    borderColor: $q.dark.isActive ? '#3C435A' : '#F4F5F7',
    show: true,
    strokeDashArray: 0,
    position: 'back',
    xaxis: { lines: { show: true } },
    yaxis: { lines: { show: true } },
    row: { opacity: 0 },
    column: { opacity: 0 },
  },
  legend: { show: false },
}));
</script>

<style lang="sass">
body.body--dark
  .chart-cpu
    background: $dark-grey-strong

.chart-cpu
  width: 100%
  background: white
</style>
