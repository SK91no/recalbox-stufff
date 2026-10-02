<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-fab
    ref="menu"
    flat
    square
    color="secondary"
    direction="down"
    icon="mdi-cog"
    vertical-actions-align="right"
  >
    <q-fab-action
      @click="openResetFactoryConfirm()"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-history"
      :label="t('general.server.resetFactory.title')"
      label-position="left"
      square
    />
    <q-fab-action
      @click="supportArchive()"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-bash"
      :label="t('general.server.supportArchive.title')"
      label-position="left"
      square
    />
    <q-fab-action
      @click="() => openVersions = true"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-source-branch"
      label="Versions"
      label-position="left"
      square
    />
    <q-fab-action
      @click="reboot()"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-restart"
      :label="t('general.server.reboot.title')"
      label-position="left"
      square
    />
    <q-fab-action
      @click="shutdown()"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-stop"
      :label="t('general.server.shutdown.title')"
      label-position="left"
      square
    />
    <q-fab-action
      @click="esReboot()"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-restart"
      :label="t('home.system.es.restart')"
      label-position="left"
      square
    />
    <q-fab-action
      @click="esShutdown()"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-stop"
      :label="t('home.system.es.stop')"
      label-position="left"
      square
    />
  </q-fab>
  <q-dialog
    transition-hide="slide-down"
    transition-show="slide-up"
    v-model="openVersions"
    class="versions-dialog"
  >
    <q-card
      class="bg-primary text-white background versions-dialog-card"
    >
      <q-card-section>
        <VersionsList/>
      </q-card-section>
    </q-card>
  </q-dialog>
</template>

<script lang="ts" setup>
import { onClickOutside } from '@vueuse/core';
import { Dark, QFab, useQuasar } from 'quasar';
import { ref } from 'vue';
import VersionsList from 'components/layout/VersionsList.vue';
import { useVersionsStore } from 'stores/common/versions';
import { useServerStore } from 'stores/common/server';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });
const {
  supportArchive, reboot, shutdown, esReboot, esShutdown,
} = useServerStore();

const $q = useQuasar();

useVersionsStore().fetch();

const openVersions = ref<boolean>(false);
const menu = ref<InstanceType<typeof QFab> | null>(null);

onClickOutside(menu, () => {
  if (menu.value) {
    menu.value.hide();
  }
});

const openResetFactoryConfirm = () => {
  $q.dialog({
    class: 'delete-dialog-card',
    message: t('general.server.resetFactory.confirmation'),
    cancel: true,
    persistent: true,
    transitionHide: 'flip-up',
    transitionShow: 'flip-down',
  }).onOk(() => {
    useServerStore().resetFactory();
  });
};
</script>

<style lang="sass">
.versions-dialog
  .versions-dialog-card
    border-left: 6px solid $accent

.q-page-container
  #help-button
    .q-btn--fab-mini:hover
      .q-icon:before
        color: $accent

  #help-button.q-fab--opened
    .q-focus-helper
      background: currentColor
      opacity: .15
</style>
