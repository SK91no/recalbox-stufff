<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row">
    <div class="col col-xs-12 col-sm-12 col-md-6">
      <FormFragmentContainer title="settings.system.internationalization.title">
        <template v-slot:content>
          <WrappedSelect
            label="settings.system.internationalization.langSelect.label"
            :options="languageOptions"
            :getter="system.language"
            :setter="systemStore.post"
            apiKey="language"
            v-if="system.language"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.internationalization.langSelect.help') }}
            </template>
          </WrappedSelect>
          <WrappedSelect
            label="settings.system.internationalization.keyboardSelect.label"
            :options="kblayoutOptions"
            :getter="system.kblayout"
            :setter="systemStore.post"
            apiKey="kblayout"
            v-if="system.kblayout"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.internationalization.keyboardSelect.help') }}
            </template>
          </WrappedSelect>
          <WrappedSelect
            label="settings.system.internationalization.timeZoneSelect.label"
            :options="timezoneOptions"
            :getter="system.timezone"
            :setter="systemStore.post"
            apiKey="timezone"
            v-if="system.timezone"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.internationalization.timeZoneSelect.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.externalScreen.title" v-if="architecture.arch === 'x86_64'">
        <template v-slot:content>
          <WrappedTextInput
            label="settings.system.externalScreen.prefered.label"
            :getter="system['externalscreen.prefered']"
            :setter="systemStore.post"
            apiKey="externalscreen.prefered"
            v-if="system['externalscreen.prefered']"
            clearable
            help
          >
            <template v-slot:help>
              {{ t('settings.system.externalScreen.prefered.help.availableOptions') }}
              <br /><br />
              {{ t('settings.system.externalScreen.prefered.help.0') }}
            </template>
          </WrappedTextInput>
          <WrappedTextInput
            label="settings.system.externalScreen.forceresolution.label"
            :getter="system['externalscreen.forceresolution']"
            :setter="systemStore.post"
            apiKey="externalscreen.forceresolution"
            v-if="system['externalscreen.forceresolution']"
            clearable
            help
          >
            <template v-slot:help>
              {{ t('settings.system.externalScreen.forceresolution.help.availableOptions') }}
              <br /><br />
              {{ t('settings.system.externalScreen.forceresolution.help.0') }}
            </template>
          </WrappedTextInput>
          <WrappedTextInput
            label="settings.system.externalScreen.forcefrequency.label"
            :getter="system['externalscreen.forcefrequency']"
            :setter="systemStore.post"
            apiKey="externalscreen.forcefrequency"
            v-if="system['externalscreen.forcefrequency']"
            clearable
            help
          >
            <template v-slot:help>
              {{ t('settings.system.externalScreen.forcefrequency.help.availableOptions') }}
              <br /><br />
              {{ t('settings.system.externalScreen.forcefrequency.help.0') }}
            </template>
          </WrappedTextInput>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.splashScreen.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.system.splashScreen.enabled.label"
            :getter="system['splash.enabled']"
            :setter="systemStore.post"
            apiKey="splash.enabled"
            v-if="system['splash.enabled']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.splashScreen.enabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedSlider
            label="settings.system.splashScreen.splashLength.label"
            :getter="system['splash.length']"
            :setter="systemStore.post"
            apiKey="splash.length"
            v-if="system['splash.length']"
            :min="splashLengthOptions.lowerValue"
            :max="splashLengthOptions.higherValue"
            icon="mdi-timer-outline"
            :disable="!system['splash.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.splashScreen.splashLength.help.availableOptions') }}
              <ul>
                <li v-html="t('settings.system.splashScreen.splashLength.help.0')"></li>
                <li v-html="t('settings.system.splashScreen.splashLength.help.1')"></li>
                <li v-html="t('settings.system.splashScreen.splashLength.help.2')"></li>
              </ul>
            </template>
          </WrappedSlider>
          <WrappedSelect
            label="settings.system.splashScreen.splashSelect.label"
            :options="translatedSplashSelectOptions"
            :getter="system['splash.select']"
            :setter="systemStore.post"
            apiKey="splash.select"
            v-if="system['splash.select']"
            :disable="!system['splash.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.splashScreen.splashSelect.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.specialKey.title">
        <template v-slot:content>
          <WrappedSelect
            label="settings.system.specialKey.label"
            :options="translatedSpecialkeysOptions"
            :getter="system['emulators.specialkeys']"
            :setter="systemStore.post"
            apiKey="emulators.specialkeys"
            v-if="system['emulators.specialkeys']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.specialKey.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.updates.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.system.updates.enabled.label"
            :getter="updates.enabled"
            :setter="updateStore.post"
            apiKey="enabled"
            v-if="updates.enabled"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.updates.enabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedSelect
            label="settings.system.updates.type.label"
            :options="typeOptions"
            :getter="updates.type"
            :setter="updateStore.post"
            apiKey="type"
            v-if="updates.type"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.updates.type.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.debuglogs.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.system.debuglogs.label"
            :getter="emulationstation.debuglogs"
            :setter="emulationstationStore.post"
            apiKey="debuglogs"
            v-if="emulationstation.debuglogs"
            help
            warning
          >
            <template v-slot:help>
              {{ t('settings.system.debuglogs.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.hat.wpaf.title" v-if="isWpafAvailable(architecture.arch)">
        <template v-slot:content>
          <WrappedToggle
            label="settings.system.hat.wpaf.enabled.label"
            :getter="hat['wpaf.enabled']"
            :setter="hatStore.post"
            apiKey="wpaf.enabled"
            v-if="hat['wpaf.enabled']"
            help
            warning
          >
            <template v-slot:help>
              {{ t('settings.system.hat.wpaf.enabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedSelect
            label="settings.system.hat.wpaf.board.label"
            :options="translatedHatBoardOptions"
            :getter="hat['wpaf.board']"
            :setter="hatStore.post"
            apiKey="wpaf.board"
            v-if="hat['wpaf.board']"
            :disable="!hat['wpaf.enabled'].value"
            help
            warning
          >
            <template v-slot:help>
              {{ t('settings.system.hat.wpaf.board.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.brightness.title" v-if="isBrightnessAvailable(architecture.arch)">
        <template v-slot:content>
          <WrappedSlider
            label="settings.system.brightness.label"
            :options="brightnessOptions"
            :getter="emulationstation.brightness"
            :setter="emulationstationStore.post"
            apiKey="brightness"
            v-if="emulationstation.brightness"
            :mix="brightnessOptions.lowerValue"
            :max="brightnessOptions.higherValue"
            icon="mdi-brightness-6"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.brightness.help') }}
            </template>
          </WrappedSlider>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.minitft.title" v-if="isFbcpAvailable(architecture.arch)">
        <template v-slot:content>
          <WrappedToggle
            label="settings.system.minitft.label"
            :getter="system['fbcp.enabled']"
            :setter="systemStore.post"
            apiKey="fbcp.enabled"
            v-if="system['fbcp.enabled']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.minitft.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.power.switch.title" v-if="isPowerSwitchAvailable(architecture.arch)">
        <template v-slot:content>
          <WrappedSelect
            label="settings.system.power.switch.label"
            :options="translatedPowerSwitchOptions"
            :getter="system['power.switch']"
            :setter="systemStore.post"
            apiKey="power.switch"
            v-if="system['power.switch']"
            clearable
            help
          >
            <template v-slot:help>
              {{ t('settings.system.power.switch.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.battery.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.system.battery.hidden.label"
            :getter="emulationstation['battery.hidden']"
            :setter="emulationstationStore.post"
            apiKey="battery.hidden"
            v-if="emulationstation['battery.hidden']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.battery.hidden.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>
    </div>
    <div class="col col-xs-12 col-sm-12 col-md-6 col">
      <FormFragmentContainer title="settings.system.services.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.system.services.managerEnabled.label"
            :getter="system['manager.enabled']"
            :setter="systemStore.post"
            apiKey="manager.enabled"
            v-if="system['manager.enabled']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.services.managerEnabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.system.services.sambaEnabled.label"
            :getter="system['samba.enabled']"
            :setter="systemStore.post"
            apiKey="samba.enabled"
            v-if="system['samba.enabled']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.services.sambaEnabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.system.services.virtualGamepadsEnabled.label"
            :getter="system['virtual-gamepads.enabled']"
            :setter="systemStore.post"
            apiKey="virtual-gamepads.enabled"
            v-if="system['virtual-gamepads.enabled']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.services.virtualGamepadsEnabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.system.services.sshEnabled.label"
            :getter="system['ssh.enabled']"
            :setter="systemStore.post"
            apiKey="ssh.enabled"
            v-if="system['ssh.enabled']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.services.sshEnabled.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.system.screensaver.title">
        <template v-slot:content>
          <WrappedSlider
            label="settings.system.screensaver.time.label"
            :getter="emulationstation['screensaver.time']"
            :setter="emulationstationStore.post"
            apiKey="screensaver.time"
            v-if="emulationstation['screensaver.time']"
            :mix="screensaverTimeOptions.lowerValue"
            :max="screensaverTimeOptions.higherValue"
            icon="mdi-clock-start"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.screensaver.time.help') }}
            </template>
          </WrappedSlider>
          <WrappedSelect
            label="settings.system.screensaver.type.label"
            :options="translatedScreensaverTypeOptions"
            :getter="emulationstation['screensaver.type']"
            :setter="emulationstationStore.post"
            apiKey="screensaver.type"
            v-if="emulationstation['screensaver.type']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.screensaver.type.help') }}
            </template>
          </WrappedSelect>
          <WrappedMultipleSelect
            label="settings.system.screensaver.demo.systemlist.label"
            :options="demoSystemlistOptions"
            :getter="global['demo.systemlist']"
            :setter="globalStore.post"
            apiKey="demo.systemlist"
            v-if="global['demo.systemlist']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.screensaver.demo.systemlist.help') }}
            </template>
          </WrappedMultipleSelect>
          <WrappedSlider
            label="settings.system.screensaver.demo.duration.label"
            :getter="global['demo.duration']"
            :setter="globalStore.post"
            apiKey="demo.duration"
            v-if="global['demo.duration']"
            :min="demoDurationOptions.lowerValue"
            :max="demoDurationOptions.higherValue"
            icon="mdi-timer-outline"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.screensaver.demo.duration.help') }}
            </template>
          </WrappedSlider>
          <WrappedSlider
            label="settings.system.screensaver.demo.infoscreenduration.label"
            :getter="global['demo.infoscreenduration']"
            :setter="globalStore.post"
            apiKey="demo.infoscreenduration"
            v-if="global['demo.infoscreenduration']"
            :min="demoInfoscreendurationOptions.lowerValue"
            :max="demoInfoscreendurationOptions.higherValue"
            icon="mdi-timer-outline"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.screensaver.demo.infoscreenduration.help') }}
            </template>
          </WrappedSlider>
          <WrappedToggle
            label="settings.system.screensaver.gameclip.showhelpitems.label"
            :getter="emulationstation.showgamecliphelpitems"
            :setter="emulationstationStore.post"
            apiKey="showgamecliphelpitems"
            v-if="emulationstation.showgamecliphelpitems"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.screensaver.gameclip.showhelpitems.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.system.screensaver.gameclip.showclippingitem.label"
            :getter="emulationstation.showgameclipclippingitem"
            :setter="emulationstationStore.post"
            apiKey="showgameclipclippingitem"
            v-if="emulationstation.showgameclipclippingitem"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.screensaver.gameclip.showclippingitem.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.system.secondminitft.title" v-if="isSecondMiniTftAvailable(architecture.arch)">
        <template v-slot:content>
          <WrappedToggle
            label="settings.system.secondminitft.enabled.label"
            :getter="system['secondminitft.enabled']"
            :setter="systemStore.post"
            apiKey="secondminitft.enabled"
            v-if="system['secondminitft.enabled']"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.enabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedSelect
            label="settings.system.secondminitft.type.label"
            :options="translatedSecondminitftTypeOptions"
            :getter="system['secondminitft.type']"
            :setter="systemStore.post"
            apiKey="secondminitft.type"
            v-if="system['secondminitft.type']"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.type.help') }}
            </template>
          </WrappedSelect>
          <WrappedSelect
          label="settings.system.secondminitft.resolution.label"
            :options="translatedSecondminitftResolutionOptions"
            :getter="system['secondminitft.resolution']"
            :setter="systemStore.post"
            apiKey="secondminitft.resolution"
            v-if="system['secondminitft.resolution']"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.resolution.help.availableOptions') }}
              <ul>
                <li v-html="t('settings.system.secondminitft.resolution.help.0')"></li>
                <li v-html="t('settings.system.secondminitft.resolution.help.1')"></li>
              </ul>
            </template>
          </WrappedSelect>
          <WrappedToggle
            label="settings.system.secondminitft.imagestretchenabled.label"
            :getter="system['secondminitft.imagestretchenabled']"
            :setter="systemStore.post"
            apiKey="secondminitft.imagestretchenabled"
            v-if="system['secondminitft.imagestretchenabled']"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.imageenlargeenabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.system.secondminitft.imageenlargeenabled.label"
            :getter="system['secondminitft.imageenlargeenabled']"
            :setter="systemStore.post"
            apiKey="secondminitft.imageenlargeenabled"
            v-if="system['secondminitft.imageenlargeenabled']"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.imagealphaenabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.system.secondminitft.imagealphaenabled.label"
            :getter="system['secondminitft.imagealphaenabled']"
            :setter="systemStore.post"
            apiKey="secondminitft.imagealphaenabled"
            v-if="system['secondminitft.imagealphaenabled']"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.imagestretchenabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.system.secondminitft.imageignoreaspectenabled.label"
            :getter="system['secondminitft.imageignoreaspectenabled']"
            :setter="systemStore.post"
            apiKey="secondminitft.imageignoreaspectenabled"
            v-if="system['secondminitft.imageignoreaspectenabled']"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.imageignoreaspectenabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.system.secondminitft.disablevideoines.label"
            :getter="system['secondminitft.disablevideoines']"
            :setter="systemStore.post"
            apiKey="secondminitft.disablevideoines"
            v-if="system['secondminitft.disablevideoines']"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.disablevideoines.help') }}
            </template>
          </WrappedToggle>
          <WrappedSlider
            label="settings.system.secondminitft.backlightcontrol.label"
            :getter="system['secondminitft.backlightcontrol']"
            :setter="systemStore.post"
            apiKey="secondminitft.backlightcontrol"
            v-if="system['secondminitft.backlightcontrol']"
            :min="secondminitftBacklightDurationOptions.lowerValue"
            :max="secondminitftBacklightDurationOptions.higherValue"
            icon="mdi-brightness-6"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.screensaver.demo.duration.help') }}
            </template>
          </WrappedSlider>
          <WrappedToggle
            label="settings.system.secondminitft.usemarquee.label"
            :getter="system['secondminitft.usemarquee']"
            :setter="systemStore.post"
            apiKey="secondminitft.usemarquee"
            v-if="system['secondminitft.usemarquee']"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.usemarquee.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.system.secondminitft.sleepenabled.label"
            :getter="system['secondminitft.sleepenabled']"
            :setter="systemStore.post"
            apiKey="secondminitft.sleepenabled"
            v-if="system['secondminitft.sleepenabled']"
            :disable="!system['secondminitft.enabled'].value"
            help
          >
            <template v-slot:help>
              {{ t('settings.system.secondminitft.sleepenabled.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>
    </div>
  </div>
</template>

<script lang="ts" setup>
import WrappedSelect from 'components/ui-kit/WrappedSelect.vue';
import WrappedToggle from 'components/ui-kit/WrappedToggle.vue';
import FormFragmentContainer from 'components/ui-kit/FormFragmentContainer.vue';
import { useTools } from 'composables/useTools';
import { useSystemStore } from 'stores/configuration/system';
import { useUpdatesStore } from 'stores/configuration/updates';
import { useEmulationstationStore } from 'stores/configuration/emulationstation';
import { useHatStore } from 'stores/configuration/hat';
import { storeToRefs } from 'pinia';
import WrappedMultipleSelect from 'components/ui-kit/WrappedMultipleSelect.vue';
import { useGlobalStore } from 'stores/configuration/global';
import WrappedSlider from 'components/ui-kit/WrappedSlider.vue';
import WrappedTextInput from 'components/ui-kit/WrappedTextInput.vue';
import { useArchitectureStore } from 'stores/common/architecture';
import { useI18n } from 'vue-i18n';
import { Boards } from 'src/utils/constants';
import { computed } from 'vue';

const { t } = useI18n({ useScope: 'global' });
const { translatedAllowedStringListToSelectOptions } = useTools();
const {
  ODROID_GO2,
  ODROID_XU4,
  RG353X,
  RPI_1,
  RPI_3,
  RPI_4_64,
  RPI_5_64,
} = Boards;

const updateStore = useUpdatesStore();
updateStore.fetch();
const { updates, typeOptions } = storeToRefs(updateStore);

const systemStore = useSystemStore();
systemStore.fetch();
const {
  languageOptions,
  kblayoutOptions,
  timezoneOptions,
  specialkeysOptions,
  splashLengthOptions,
  splashSelectOptions,
  powerSwitchOptions,
  secondminitftTypeOptions,
  secondminitftResolutionOptions,
  secondminitftBacklightDurationOptions,
  system,
} = storeToRefs(systemStore);

const globalStore = useGlobalStore();
globalStore.fetch();
globalStore.fetchOptions();
const {
  demoInfoscreendurationOptions,
  demoDurationOptions,
  demoSystemlistOptions,
  global,
} = storeToRefs(globalStore);

const emulationstationStore = useEmulationstationStore();
emulationstationStore.fetch();
emulationstationStore.fetchOptions();
const {
  screensaverTimeOptions,
  screensaverTypeOptions,
  brightnessOptions,
  emulationstation,
} = storeToRefs(emulationstationStore);

const hatStore = useHatStore();
hatStore.fetch();
const {
  boardOptions,
  hat,
} = storeToRefs(hatStore);

const architectureStore = useArchitectureStore();
architectureStore.fetch();
const { architecture } = storeToRefs(architectureStore);

// eslint-disable-next-line @typescript-eslint/no-unused-vars
const isWpafAvailable = (arch: string) => false; // [RPI_3, RPI_4_64, RPI_5_64].includes(arch);
const isBrightnessAvailable = (arch: string) => [ODROID_GO2, RG353X].includes(arch as Boards);
const isFbcpAvailable = (arch: string) => [RPI_1, RPI_3, RPI_4_64, RPI_5_64].includes(arch as Boards);
const isPowerSwitchAvailable = (arch: string) => [RPI_1, RPI_3, RPI_4_64, RPI_5_64].includes(arch as Boards);
const isSecondMiniTftAvailable = (arch: string) => [ODROID_XU4, RPI_1, RPI_3, RPI_4_64, RPI_5_64].includes(arch as Boards);

const translatedSplashSelectOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.system.splashScreen.splashSelect.options',
  splashSelectOptions.value,
));

const translatedSpecialkeysOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.system.specialKey.options',
  specialkeysOptions.value,
));

const translatedPowerSwitchOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.system.power.switch.options',
  powerSwitchOptions.value,
));

const translatedScreensaverTypeOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.system.screensaver.type.options',
  screensaverTypeOptions.value,
));

const translatedSecondminitftTypeOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.system.secondminitft.type.options',
  secondminitftTypeOptions.value,
));

const translatedSecondminitftResolutionOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.system.secondminitft.resolution.options',
  secondminitftResolutionOptions.value,
));

const translatedHatBoardOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.system.hat.wpaf.board.options',
  boardOptions.value,
));
</script>

<style lang="sass" scoped>
.q-tab-panel > .row
  margin-bottom: 5em
</style>
