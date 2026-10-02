<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row versions">
    <div class="col-12" style="padding: 0 1em;">
      <div class="row">
        <div class="col col-xs-12 col-sm-12 col-md-6">
          <div
            @click="copy(`Linux : ${versions.linux}`)"
            class="row system-versions"
          >
            <div class="col col-xs-3 col-md-4 self-center" style="padding-left: .5em;">
              Linux
            </div>
            <div class="col col-xs-9 col-md-8">
              <q-chip
                color="secondary"
                dense
                icon="mdi-source-branch"
                square
                text-color="white"
              >
                {{versions.linux}}
              </q-chip>
            </div>
          </div>
          <div
            @click="copy(`Recalbox : ${versions.recalbox}`)"
            class="row system-versions"
          >
            <div class="col col-xs-3 col-md-4 self-center" style="padding-left: .5em;">
              Recalbox
            </div>
            <div class="col col-xs-9 col-md-8">
              <q-chip
                color="secondary"
                dense
                icon="mdi-source-branch"
                square
                text-color="white"
              >
                {{versions.recalbox}}
              </q-chip>
            </div>
          </div>
        </div>
        <div
          @click="copy(`WebAPI : ${versions.webapi}`)"
          class="col col-xs-12 col-sm-12 col-md-6"
        >
          <div class="row system-versions">
            <div class="col col-xs-3 col-md-4 self-center" style="padding-left: .5em;">
              WebAPI
            </div>
            <div class="col col-xs-9 col-md-8">
              <q-chip
                color="secondary"
                dense
                icon="mdi-source-branch"
                square
                text-color="white"
              >
                {{versions.webapi}}
              </q-chip>
            </div>
          </div>
          <div
            @click="copy(`RetroArch : ${versions.libretro.retroarch}`)"
            class="row system-versions"
          >
            <div class="col col-xs-3 col-md-4 self-center" style="padding-left: .5em;">
              RetroArch
            </div>
            <div class="col col-xs-9 col-md-8">
              <q-chip
                color="secondary"
                dense
                icon="mdi-source-branch"
                square
                text-color="white"
              >
                {{versions.libretro.retroarch}}
              </q-chip>
            </div>
          </div>
        </div>
      </div>
      <q-separator style="margin-top: 1em; margin-bottom: 1em"/>
    </div>

    <LongDataList
      :data="versions.libretro.cores"
      v-if="versions.libretro.cores"
    />

  </div>
</template>

<script lang="ts" setup>
import LongDataList from 'components/ui-kit/LongDataList.vue';
import { useTools } from 'composables/useTools';
import { useVersionsStore } from 'stores/common/versions';
import { storeToRefs } from 'pinia';

const { copy } = useTools();

const versionsStore = useVersionsStore();
const { versions } = storeToRefs(versionsStore);
</script>

<style lang="sass">
.versions
  .system-versions,
  .version-line
    &:hover
      background: $secondary
      cursor: pointer

      .q-chip
        background: $primary !important
        color: $accent !important
</style>
