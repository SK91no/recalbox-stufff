<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-btn
    padding="16px"
    flat
    square
    icon="mdi-theme-light-dark"
    color="white"
    id="virtual-devices-button"
    @click="toggleMode"
    style="margin-right: 1em; border-radius: 4px;"
  >
    <q-tooltip class="bg-primary" anchor="top middle" self="bottom middle" :offset="[10, 10]">
      {{ t(getTitle()) }}
    </q-tooltip>
  </q-btn>
</template>

<script lang="ts" setup>
import { onMounted } from 'vue';
import { useI18n } from 'vue-i18n';
import { useQuasar, Dark, LocalStorage } from 'quasar';

const $q = useQuasar();

const { t } = useI18n({ useScope: 'global' });

const getTitle = () => ($q.dark.isActive ? 'darkModeMenu.lightMode' : 'darkModeMenu.darkMode');

const persistMode = () => {
  const mode = $q.dark.isActive ? 'dark' : 'light';
  LocalStorage.set('mode', mode);
};

const toggleMode = () => {
  Dark.toggle();
  persistMode();
};

onMounted(() => {
  Dark.set(LocalStorage.getItem('mode') === 'dark');
});
</script>
