<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-page class="background settings">
    <transition
      appear
      enter-active-class="animated slideInDown"
    >
      <q-tabs
        active-color="accent"
        align="justify"
        :class="Dark.isActive ? 'text-dark-grey-light' : 'text-grey'"
        dense
        indicator-color="accent"
        inline-label
        narrow-indicator
      >
        <q-route-tab
          :to="{ name: 'system' }"
          :label="t('settings.menu.system')"
          icon="mdi-server"
          name="system"
        />
        <q-route-tab
          :to="{ name: 'audio' }"
          :label="t('settings.menu.audio')"
          icon="mdi-volume-high"
          name="audio"
        />
        <q-route-tab
          :to="{ name: 'network' }"
          :label="t('settings.menu.network')"
          icon="mdi-wifi"
          name="network"
        />
        <q-route-tab
          :to="{ name: 'emustation' }"
          icon="icon-emustation"
          label="EmulationStation"
          name="emustation"
          style="text-transform: none;"
        />
        <q-route-tab
          :to="{ name: 'scraper' }"
          icon="mdi-folder-multiple-image"
          label="Scraper"
          name="scraper"
        />
        <q-route-tab
          :to="{ name: 'kodi' }"
          icon="mdi-kodi"
          label="Kodi"
          name="kodi"
          v-if="isBoard(architecture.arch) && isKodiAvailable(architecture.arch)"
        />
<!--        <q-route-tab -->
<!--          :to="{ name: 'hyperion' }" -->
<!--          icon="mdi-lightbulb" -->
<!--          label="Hyperion" -->
<!--          name="hyperion" -->
<!--        /> -->
      </q-tabs>
    </transition>

    <q-tab-panels animated v-model="tab">
      <q-tab-panel name="system">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="audio">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="network">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="emustation">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="screenscraper">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="kodi">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="hyperion">
        <router-view/>
      </q-tab-panel>

    </q-tab-panels>
  </q-page>
</template>

<script lang="ts" setup>
import { Boards, BOARDS, isBoard } from 'src/utils/constants';
import { ref } from 'vue';
import { useSystemStore } from 'stores/configuration/system';
import { useUpdatesStore } from 'stores/configuration/updates';
import { useAudioStore } from 'stores/configuration/audio';
import { useEmulationstationStore } from 'stores/configuration/emulationstation';
import { useKodiStore } from 'stores/configuration/kodi';
import { useScraperStore } from 'stores/configuration/scraper';
import { useWifiStore } from 'stores/configuration/wifi';
import { useGlobalStore } from 'stores/configuration/global';
import { useHatStore } from 'stores/configuration/hat';
import { storeToRefs } from 'pinia';
import { useArchitectureStore } from 'stores/common/architecture';
import { useI18n } from 'vue-i18n';
import { Dark } from 'quasar';

const { t } = useI18n({ useScope: 'global' });

useSystemStore().fetchOptions();

useAudioStore().fetchOptions();
useUpdatesStore().fetchOptions();
useEmulationstationStore().fetchOptions();
useKodiStore().fetchOptions();
useScraperStore().fetchOptions();
useWifiStore().fetchOptions();
useGlobalStore().fetchOptions();
useHatStore().fetchOptions();

const architectureStore = useArchitectureStore();
architectureStore.fetch();
const { architecture } = storeToRefs(architectureStore);

const isKodiAvailable = (arch: Boards) => BOARDS.includes(arch);

const tab = ref<string>('system');
</script>

<style lang="sass">
.settings
  .q-tab-panels
    .line
      margin-bottom: 1em
      background: white

  @media(max-width: 700px)
    .settings
      .q-tab-panels
        .line
          .title
            text-align: right

.settings
  &:before
    content: "\F0493"

  .q-tabs
    background: white

  .q-tab-panels
    background: transparent
</style>
