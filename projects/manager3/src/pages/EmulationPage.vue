<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-page class="background emulation">
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
          :to="{ name: 'global' }"
          :label="t('emulation.menu.global')"
          icon="mdi-sony-playstation"
          name="global"
        />
        <q-route-tab
          :to="{ name: 'netplay' }"
          :label="t('emulation.menu.netplay')"
          icon="mdi-lan-connect"
          name="netplay"
        />
        <q-route-tab
          :to="{ name: 'retroachievements' }"
          :label="t('emulation.menu.retroachievements')"
          icon="mdi-trophy"
          name="retroachievements"
        />
        <q-route-tab
          :to="{ name: 'bios' }"
          :label="t('emulation.menu.bios')"
          icon="mdi-heart-pulse"
          name="bios"
        />
        <q-route-tab
          :to="{ name: 'systems-parent' }"
          :label="t('emulation.menu.systems')"
          icon="mdi-television-classic"
          name="systems"
        />
        <q-route-tab
          :to="{ name: 'controllers' }"
          :label="t('emulation.menu.controllers')"
          icon="mdi-gamepad-variant-outline"
          name="controllers"
        />
      </q-tabs>
    </transition>

    <q-tab-panels v-model="tab">
      <q-tab-panel name="global">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="netplay">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="bios">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="systems-parent">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="controllers">
        <router-view/>
      </q-tab-panel>

      <q-tab-panel name="screenshots">
        <router-view/>
      </q-tab-panel>

    </q-tab-panels>
  </q-page>
</template>

<script lang="ts" setup>
import { useGlobalStore } from 'stores/configuration/global';
import { ref } from 'vue';
import { useControllersStore } from 'stores/emulation/controllers';
import { useEmulationstationStore } from 'stores/configuration/emulationstation';
import { useSystemStore } from 'stores/configuration/system';
import { useTateStore } from 'stores/configuration/tate';
import { useI18n } from 'vue-i18n';
import { Dark } from 'quasar';

const { t } = useI18n({ useScope: 'global' });

useGlobalStore().fetchOptions();
useControllersStore().fetchOptions();
useEmulationstationStore().fetchOptions();
useSystemStore().fetchOptions();
useTateStore().fetchOptions();

const tab = ref<string>('global');
</script>

<style lang="sass">
.emulation
  &:before
    content: "\F02B4"

  .q-tabs
    background: white

  .q-tab-panels
    background: transparent
</style>
