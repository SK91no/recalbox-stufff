<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row">
    <div class="col col-xs-12 col-sm-12 col-md-12">
      <FormFragmentContainer title="settings.hyperion.settings.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.hyperion.settings.enableHyperion.label"
            :getter="hyperion.enabled"
            :setter="hyperionStore.post"
            apiKey="enabled"
            v-if="hyperion.enabled"
            help
          >
            <template v-slot:help>
              {{ t('settings.hyperion.settings.enableHyperion.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>
    </div>
  </div>
</template>

<script lang="ts" setup>
import WrappedToggle from 'components/ui-kit/WrappedToggle.vue';
import { useHyperionStore } from 'stores/configuration/hyperion';
import { storeToRefs } from 'pinia';
import FormFragmentContainer from 'components/ui-kit/FormFragmentContainer.vue';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });

const hyperionStore = useHyperionStore();
hyperionStore.fetch();
const { hyperion } = storeToRefs(hyperionStore);
</script>
