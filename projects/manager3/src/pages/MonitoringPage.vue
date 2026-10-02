<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-page class="background monitoring">
    <div class="row cpuChart">
      <CPUChartContent :data="JSON.parse(JSON.stringify(areaChartData))"/>
    </div>
    <div class="row charts-container">
      <div class="col cores-usage">
        <CoresChartContent :data="cores"/>
      </div>
      <div class="col disks-usage">
        <StoragesChartContent :data="monitoringStore.getFilteredStorages()"/>
      </div>
    </div>
  </q-page>
</template>

<script lang="ts" setup>
import { useMonitoringStore } from 'stores/common/monitoring';
import { storeToRefs } from 'pinia';
import { computed } from 'vue';
import CPUChartContent from 'components/monitoring/CPUChartContent.vue';
import CoresChartContent from 'components/monitoring/CoresChartContent.vue';
import StoragesChartContent from 'components/monitoring/StoragesChartContent.vue';

const monitoringStore = useMonitoringStore();
const { metrics } = storeToRefs(monitoringStore);

monitoringStore.fetch();

const cores = computed<object[]>(() => [{
  data: metrics.value.cores,
}]);
const areaChartData = computed<object[]>(() => [
  { data: metrics.value.temperatures },
  { data: metrics.value.memory },
]);
</script>

<style lang="sass" scoped>
body.body--dark
  .monitoring
    .charts-container
      .col
        background: $dark-grey-strong

.monitoring
  &:before
    content: "\F076A"

  .cpuChart
    position: relative

  .charts-container
    position: relative
    padding: 16px 8px

    .col
      background: white
      margin: 0 8px 1em

  .cores-usage
    flex: 2

  .disks-usage
    flex: 1
    padding: .5em

@media(max-width: 950px)
  .monitoring
    .charts-container
      flex-direction: column
</style>
