<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row">
    <div class="col col-xs-12 col-sm-12 col-md-12">
      <FormFragmentContainer title="settings.network.server.title">
        <template v-slot:content>
          <WrappedTextInput
            label="settings.network.server.hostname.label"
            :getter="system.hostname"
            :setter="systemStore.post"
            apiKey="hostname"
            v-if="system.hostname"
            help
          >
            <template v-slot:help>
              {{ t('settings.network.server.hostname.help') }}
            </template>
          </WrappedTextInput>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.network.wifi.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.network.wifi.enabled.label"
            :getter="wifi.enabled"
            :setter="wifiStore.post"
            apiKey="enabled"
            v-if="wifi.enabled"
            help
          >
            <template v-slot:help>
              {{ t('settings.network.wifi.enabled.help') }}
            </template>
          </WrappedToggle>

          <WrappedToggle
            label="settings.network.wifi.connect.label"
            :getter="wifi.connect"
            :setter="wifiStore.post"
            apiKey="connect"
            v-if="wifi.connect"
            help
          >
            <template v-slot:help>
              {{ t('settings.network.wifi.connect.help') }}
            </template>
          </WrappedToggle>

          <WrappedSlider
            label="settings.network.wifi.priority.title"
            :getter="wifi.priority"
            :setter="wifiStore.post"
            apiKey="priority"
            v-if="wifi.priority"
            :min="priorityOptions.lowerValue"
            :max="priorityOptions.higherValue"
            icon="mdi-priority-high"
            help
          >
            <template v-slot:help>
              {{ t('settings.network.wifi.priority.help') }}
            </template>
          </WrappedSlider>

          <q-separator/>

          <WrappedSelect
            label="settings.network.wifi.region.title"
            :options="regionOptions"
            :getter="wifi.region"
            :setter="wifiStore.post"
            apiKey="region"
            v-if="wifi.region"
            help
          >
            <template v-slot:help>
              {{ t('settings.network.wifi.region.help') }}
            </template>
          </WrappedSelect>

          <q-separator/>

          <WrappedTextInput
            label="settings.network.wifi.ip.title"
            :getter="wifi.ip"
            :setter="wifiStore.post"
            apiKey="ip"
            v-if="wifi.ip"
            help
          >
            <template v-slot:help>
              {{ t('settings.network.wifi.ip.help') }}
            </template>
          </WrappedTextInput>

          <WrappedTextInput
            label="settings.network.wifi.netmask.title"
            :getter="wifi.netmask"
            :setter="wifiStore.post"
            apiKey="netmask"
            v-if="wifi.netmask"
            help
          >
            <template v-slot:help>
              {{ t('settings.network.wifi.netmask.help') }}
            </template>
          </WrappedTextInput>

          <WrappedTextInput
            label="settings.network.wifi.gateway.title"
            :getter="wifi.gateway"
            :setter="wifiStore.post"
            apiKey="gateway"
            v-if="wifi.gateway"
            help
          >
            <template v-slot:help>
              {{ t('settings.network.wifi.gateway.help') }}
            </template>
          </WrappedTextInput>

          <WrappedTextInput
            label="settings.network.wifi.nameservers.title"
            :getter="wifi.nameservers"
            :setter="wifiStore.post"
            apiKey="nameservers"
            v-if="wifi.nameservers"
            help
          >
            <template v-slot:help>
              {{ t('settings.network.wifi.nameservers.help') }}
            </template>
          </WrappedTextInput>

          <q-separator/>

          <q-splitter v-model="splitterModel">
            <template v-slot:before>
              <q-tabs
                active-color="primary"
                class="text-grey"
                indicator-color="accent"
                inline-label
                v-model="tab"
                vertical
              >
                <q-tab
                  icon="mdi-numeric-1-circle"
                  name="wifi_1"
                />
                <q-tab
                  icon="mdi-numeric-2-circle"
                  name="wifi_2"
                />
                <q-tab
                  icon="mdi-numeric-3-circle"
                  name="wifi_3"
                />
              </q-tabs>
            </template>

            <template v-slot:after>
              <q-tab-panels
                animated
                swipeable
                transition-next="jump-up"
                transition-prev="jump-up"
                v-model="tab"
                vertical
              >
                <q-tab-panel name="wifi_1">
                  <WrappedTextInput
                    label="settings.network.wifi.ssid.title"
                    :getter="wifi.ssid"
                    :setter="wifiStore.post"
                    apiKey="ssid"
                    v-if="wifi.ssid"
                    help
                  >
                    <template v-slot:help>
                      {{ t('settings.network.wifi.ssid.help') }}
                    </template>
                  </WrappedTextInput>
                  <WrappedTextInput
                    label="settings.network.wifi.key.title"
                    :getter="wifi.key"
                    :setter="wifiStore.post"
                    apiKey="key"
                    v-if="wifi.key"
                    password
                    help
                    >
                    <template v-slot:help>
                      {{ t('settings.network.wifi.key.help') }}
                    </template>
                  </WrappedTextInput>
                </q-tab-panel>

                <q-tab-panel name="wifi_2">
                  <WrappedTextInput
                    label="settings.network.wifi.ssid.title"
                    :getter="wifi2.ssid"
                    :setter="wifi2Store.post"
                    v-if="wifi2.ssid"
                    help
                    api-key="ssid"
                  >
                    <template v-slot:help>
                      {{ t('settings.network.wifi.ssid.help') }}
                    </template>
                  </WrappedTextInput>
                  <WrappedTextInput
                    label="settings.network.wifi.key.title"
                    :getter="wifi2.key"
                    :setter="wifi2Store.post"
                    v-if="wifi2.key"
                    password
                    help
                    api-key="key"
                  >
                    <template v-slot:help>
                      {{ t('settings.network.wifi.key.help') }}
                    </template>
                  </WrappedTextInput>
                </q-tab-panel>

                <q-tab-panel name="wifi_3">
                  <WrappedTextInput
                    label="settings.network.wifi.ssid.title"
                    :getter="wifi3.ssid"
                    :setter="wifi3Store.post"
                    v-if="wifi3.ssid"
                    help
                    api-key="ssid"
                  >
                    <template v-slot:help>
                      {{ t('settings.network.wifi.ssid.help') }}
                    </template>
                  </WrappedTextInput>
                  <WrappedTextInput
                    label="settings.network.wifi.key.title"
                    :getter="wifi3.key"
                    :setter="wifi3Store.post"
                    v-if="wifi3.key"
                    password
                    help
                    api-key="key"
                  >
                    <template v-slot:help>
                      {{ t('settings.network.wifi.key.help') }}
                    </template>
                  </WrappedTextInput>
                </q-tab-panel>
              </q-tab-panels>
            </template>

          </q-splitter>
        </template>
      </FormFragmentContainer>
    </div>
  </div>
</template>

<script lang="ts" setup>
import WrappedTextInput from 'components/ui-kit/WrappedTextInput.vue';
import WrappedToggle from 'components/ui-kit/WrappedToggle.vue';
import WrappedSelect from 'components/ui-kit/WrappedSelect.vue';
import WrappedSlider from 'components/ui-kit/WrappedSlider.vue';
import { useSystemStore } from 'stores/configuration/system';
import { useWifiStore } from 'stores/configuration/wifi';
import { useWifi2Store } from 'stores/configuration/wifi2';
import { useWifi3Store } from 'stores/configuration/wifi3';
import { ref } from 'vue';
import { storeToRefs } from 'pinia';
import FormFragmentContainer from 'components/ui-kit/FormFragmentContainer.vue';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });

const systemStore = useSystemStore();
systemStore.fetch();
const { system } = storeToRefs(systemStore);

const wifiStore = useWifiStore();
wifiStore.fetch();
const { wifi, regionOptions, priorityOptions } = storeToRefs(wifiStore);

const wifi2Store = useWifi2Store();
wifi2Store.fetch();
const { wifi2 } = storeToRefs(wifi2Store);

const wifi3Store = useWifi3Store();
wifi3Store.fetch();
const { wifi3 } = storeToRefs(wifi3Store);

const tab = ref<string>('wifi_1');
const splitterModel = ref<number>(20);
</script>

<style lang="sass" scoped>
.q-tab-panel > .row
  margin-bottom: 5em
</style>
