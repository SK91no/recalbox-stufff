<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-btn
    flat
    icon="mdi-keyboard"
    @click="show(true)"
    :disable="!system['virtual-gamepads.enabled']?.value"
  />
</template>

<script lang="ts" setup>
import { useQuasar } from 'quasar';
import { useRouter } from 'vue-router';
import { useI18n } from 'vue-i18n';
import { useSystemStore } from 'stores/configuration/system';
import { storeToRefs } from 'pinia';

const $q = useQuasar();
const router = useRouter();
const { t } = useI18n({ useScope: 'global' });

const systemStore = useSystemStore();
systemStore.fetch();
const { system } = storeToRefs(systemStore);

const show = (display: boolean) => {
  $q.bottomSheet({
    message: t('general.virtualDevices.title'),
    class: 'virtual-devices-menu',
    grid: display,
    actions: [
      {
        label: t('general.virtualDevices.keyboard'),
        icon: 'mdi-keyboard-outline',
        id: 'keyboard',
        route: 'virtual-devices-keyboard',
      },
      {
        label: t('general.virtualDevices.gamepad'),
        icon: 'mdi-gamepad-variant-outline',
        id: 'gamepad',
        route: 'virtual-devices-gamepad',
      },
      {
        label: t('general.virtualDevices.trackpad'),
        icon: 'mdi-trackpad',
        id: 'trackpad',
        route: 'virtual-devices-trackpad',
      },
    ],
  }).onOk((button) => {
    const routeData = router.resolve({ name: button.route });
    window.open(routeData.href, '_blank');
  });
};
</script>

<style lang="sass">
#virtual-devices-button
  border-radius: 4px

.virtual-devices-menu
  width: inherit

  .q-bottom-sheet__item
    min-width: 140px
</style>
