<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row systems">
    <div class="col col-xs-12 col-sm-12 col-md-12 table-container">
      <q-table
        grid
        card-container-class="card-container"
        card-class="bg-secondary text-white card"
        :rows="SystemsList"
        :columns="columns"
        row-key="name"
        :filter="table.filter"
        hide-header
        square
        flat
        dense
        :pagination="table.pagination"
      >
        <template v-slot:top-left>
          <q-btn
            :color="filterButtons.hasNetplay ? 'accent' : Dark.isActive ? '$dark-grey-light' : 'light-blue'"
            @click="filterButtons.hasNetplay = !filterButtons.hasNetplay"
            flat
            icon="mdi-lan-connect"
            round
            size="md"
          >
            <q-tooltip
              class="bg-primary"
              :offset="[10, 10]"
              content-class="bg-primary"
              content-style="font-size: 16px"
            >
              {{ t(filterButtons.hasNetplay ? 'emulation.systems.tooltips.all.label': 'emulation.systems.tooltips.hasNetplay.label') }}
            </q-tooltip>
          </q-btn>
        </template>
        <template v-slot:top-right>
          <q-input
            :placeholder="t('general.tables.searchLabel')"
            debounce="300"
            dense
            standout="bg-primary text-white"
            v-model="table.filter"
            class="search"
          >
            <template v-slot:prepend>
              <q-icon name="mdi-magnify"/>
            </template>
            <template v-slot:append v-if="table.filter !== ''">
              <q-icon
                name="mdi-close"
                @click="table.filter = ''"
                class="cursor-pointer"
              />
            </template>
          </q-input>
        </template>

        <template v-slot:item="props">
          <div
            @click="() => router.push(
              { name: 'systems-system', params: { system: props.row.name }}
            )"
            class="q-pa-xs col-xs-12 col-sm-6 col-md-2 col-lg-2 grid-style-transition"
          >
            <q-card class="no-box-shadow no-border-radius card">
              <q-card-section>
                <div class="background"></div>
                <q-img
                  :src="`${api}/systems/${props.row.name}/resource/${props.row.themeRegion}/svg/logo`"
                  spinner-color="$light-blue"
                  :ratio="16/9"
                  fit="contain"
                  loading="lazy"
                />
                <div class="fullname">{{props.row.fullName}}</div>
              </q-card-section>
            </q-card>
          </div>
        </template>

        <template v-slot:no-data="{ icon, message }">
          <div class="full-width row flex-center text-accent q-gutter-md q-ma-md">
            <q-icon name="mdi-emoticon-sad-outline" size="2em"/>
            <span>
              {{ `${t('general.tables.noDataMessage')} ${message}` }}
            </span>
            <q-icon :name="table.filter ? 'mdi-layers-search-outline' : icon" size="2em"/>
          </div>
        </template>
      </q-table>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { useSystemsStore } from 'stores/emulation/systems';
import { System } from 'stores/emulation/systems.d';
import { computed, reactive, ref } from 'vue';
import { storeToRefs } from 'pinia';
import { apiUrl } from 'boot/axios';
import { useI18n } from 'vue-i18n';
import { useRouter } from 'vue-router';
import type { QTableColumn } from 'quasar';
import { Dark } from 'quasar';

const router = useRouter();
const { t } = useI18n({ useScope: 'global' });

const systemsStore = useSystemsStore();
const { filteredSystemsList } = storeToRefs(systemsStore);

const api: string|undefined = apiUrl;
const columns = computed<QTableColumn<System>[]>(() => [
  {
    name: 'desc',
    required: true,
    label: '',
    align: 'left',
    field: (row: System) => row.fullName,
    sortable: true,
  },
]);

const table = ref({
  filter: '',
  pagination: {
    rowsPerPage: 24,
  },
});

const filterButtons = reactive({
  hasNetplay: false,
});

const SystemsList = computed(() => {
  let list = filteredSystemsList.value;

  if (filterButtons.hasNetplay) {
    list = list.filter((filteredSystem) => filteredSystem.properties.hasNetplay);
  }
  return list;
});
</script>

<style lang="sass">
body.body--dark
  .systems
    .table-container
      background: $dark-grey-strong

      .card-container
        padding: 0 12px

        .card
          background: $dark-grey-medium
          color: $dark-grey-light

.systems
  .table-container
    margin: 0 8px 74px
    background: white
    width: calc(100% - 16px)

    .card-container
      padding: 0 12px

      .card
        background: $rc-light-grey
        color: $light-blue
        text-align: center
        text-transform: uppercase
        cursor: pointer
        overflow: hidden
        border-radius: 5px!important

        .q-card__section
          padding: .5em 2.5em

          .q-img__image
            transition: filter .2s ease
            filter: saturate(0)
            opacity: .5

          .fullname, .background
            position: absolute
            left: 0
            right: 0
            transition: height .2s ease, top .35s ease

          .background
            top: 0
            height: 0
            background: rgba($primary, .4)

          .fullname
            top: -100%
            color: white
            font-weight: 700

        &:hover
          .q-card__section
            .q-img__image
              filter: saturate(1)
              opacity: 1

            .background
              height: 100%

            .fullname
              top: 0

  @media(max-width: 700px)
    .q-table__control, .search
      width: 100%
</style>
