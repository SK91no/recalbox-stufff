<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row system">
    <div class="col col-xs-12 col-sm-12 col-md-12 table-container">
      <q-table
        :columns="columns"
        :rows="roms"
        :filter="table.filter"
        :pagination="table.pagination"
        binary-state-sort
        dense
        flat
        row-key="name"
        square
      >
        <template v-slot:top-left>
          <q-btn
            color="accent"
            square
            icon="mdi-chevron-left"
            size="md"
            flat
            @click="() => router.go(-1)"
            style="margin-right: 1em;"
          >
            {{t('general.return')}}
          </q-btn>
          <q-img
            class="logo"
            :src="logoUrl"
            spinner-color="white"
          />
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
<!--          <q-btn class="q-ml-md" color="accent" icon="mdi-plus-thick" outline/>-->
        </template>

        <template v-slot:header="props">
          <q-tr :props="props">
            <q-th
              :key="col.name"
              :props="props"
              v-for="col in props.cols"
            >
              {{ col.label }}
            </q-th>
<!--            <q-th auto-width/>-->
<!--            <q-th auto-width/>-->
          </q-tr>
        </template>

        <template v-slot:body="props">
          <q-tr :props="props">
            <q-td
              :key="col.name"
              :props="props"
              v-for="col in props.cols"
            >
              <span v-if="col.name === 'favorite'">
                <q-icon
                  :name="col.value ? 'mdi-star' : 'mdi-star-outline'"
                  color="accent"
                  size="2em"
                />
              </span>
              <span v-else-if="col.name === 'rating'">
                <q-rating
                  :model-value="col.value * 5"
                  class="no-shadow"
                  color="accent"
                  readonly
                  size="1em"
                  icon="star"
                  icon-half="star_half"
                />
              </span>
              <span v-else-if="col.name === 'genre'">
                {{ col.value?.replaceAll(',', ', ') }}
              </span>
              <span v-else>
                {{ col.value }}
              </span>
            </q-td>
<!--            <q-td auto-width>-->
<!--              <q-btn color="primary" dense flat icon="mdi-pencil" round size="md"/>-->
<!--            </q-td>-->
<!--            <q-td auto-width>-->
<!--              <q-btn-->
<!--                color="negative"-->
<!--                dense-->
<!--                flat-->
<!--                icon="mdi-delete"-->
<!--                round size="md"-->
<!--              />-->
<!--            </q-td>-->
          </q-tr>
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
import { QTableColumn } from 'quasar';
import { useRomsStore } from 'stores/emulation/roms';
import { useSystemsStore } from 'stores/emulation/systems';
import { RomEntry } from 'stores/types/misc';
import { computed, ref } from 'vue';
import { useI18n } from 'vue-i18n';
import { useRouter, useRoute } from 'vue-router';
import { apiUrl } from 'boot/axios';
import { storeToRefs } from 'pinia';

const systemsStore = useSystemsStore();
const { systems } = systemsStore;
const router = useRouter();
const route = useRoute();
const system = computed(() => route.params.system as string);
const { t } = useI18n({ useScope: 'global' });

// eslint-disable-next-line @typescript-eslint/no-explicit-any
let logoUrl: any = null;

if (systems.systems.length === 0) {
  systemsStore.fetch();
}

if (systems.systems.length > 0) {
  logoUrl = computed(() => {
    const found = systems.systems.find((s) => s.name === system.value);
    return found
      ? `${apiUrl}/systems/${found.name}/resource/${found.themeRegion}/svg/logo`
      : '';
  });
}

const romsStore = useRomsStore();
romsStore.fetchBySystem(system.value);

const columns = computed((): QTableColumn<RomEntry>[] => [
  {
    name: 'favorite',
    align: 'left',
    label: t('emulation.roms.columns.favorite'),
    field: 'favorite',
    sortable: true,
  },
  {
    name: 'nom',
    required: true,
    align: 'left',
    label: t('emulation.roms.columns.name'),
    field: 'name',
    sortable: true,
  },
  {
    name: 'editeur',
    align: 'left',
    label: t('emulation.roms.columns.publisher'),
    field: 'publisher',
    sortable: true,
  },
  {
    name: 'developpeur',
    align: 'left',
    label: t('emulation.roms.columns.developer'),
    field: 'developer',
    sortable: true,
  },
  {
    name: 'genre',
    align: 'left',
    label: t('emulation.roms.columns.genre'),
    field: 'genre',
    sortable: true,
  },
  {
    name: 'joueurs',
    align: 'center',
    label: t('emulation.roms.columns.players'),
    field: 'players',
    sortable: true,
  },
  {
    name: 'rating',
    align: 'center',
    label: t('emulation.roms.columns.rating'),
    field: 'rating',
    sortable: true,
  },
]);
const table = ref({
  filter: '',
  pagination: {
    rowsPerPage: 20,
    descending: false,
    sortBy: 'nom',
  },
});

const { roms } = storeToRefs(romsStore);
</script>

<style lang="sass">
.system
  .table-container
    margin: 0 8px 74px
    width: calc(100% - 16px)

    .q-table__top
      margin-bottom: 10px

    .logo
      width: 100px
      max-height: 50px

    @media(max-width: 700px)
      .search
        flex: 5

body.body--dark
  .q-table
    color: $dark-white

  .q-table__bottom
    .q-table__control
      color: $dark-white

      .q-btn
        .q-icon
          color: $dark-white

  .q-select__dropdown-icon
    color: $dark-white
</style>
