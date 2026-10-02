/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import {
  SystemConfigOptionsResponse,
  SystemConfigResponse,
} from 'stores/configuration/system.d';
import { useTools } from 'composables/useTools';

const { displayableStringListToSelectOptions, allowedStringListToSelectOptions } = useTools();

export interface SystemStoreState {
  _baseUrl: string;
  _systemOptions: SystemConfigOptionsResponse;
  system: SystemConfigResponse;
}

export const useSystemStore = defineStore('system', {
  state: () => ({
    _baseUrl: CONFIGURATION.system,
    _systemOptions: {
      language: {
        allowedStringList: [''],
        displayableStringList: [''],
      },
      kblayout: {
        allowedStringList: [''],
        displayableStringList: [''],
      },
      timezone: {
        allowedStringList: [''],
      },
      'emulators.specialkeys': {
        allowedStringList: [] as string[],
      },
      'es.videomode': {
        allowedStringList: [''],
        displayableStringList: [''],
      },
      'splash.length': {
        lowerValue: -1,
        higherValue: 300,
      },
      'splash.select': {
        allowedStringList: [] as string[],
      },
      'power.switch': {
        allowedStringList: [] as string[],
      },
      'secondminitft.type': {
        allowedStringList: [] as string[],
      },
      'secondminitft.resolution': {
        allowedStringList: [] as string[],
      },
      'secondminitft.backlightcontrol': {
        lowerValue: 0,
        higherValue: 100,
      },
      'externalscreen.prefered': {
        allowedChars: '',
      },
      'externalscreen.forceresolution': {
        allowedChars: '',
      },
      'externalscreen.forcefrequency': {
        allowedChars: '',
      },
      hostname: {
        allowedChars: '',
      },
    },
    system: {
      language: {
        value: 'fr_FR',
      },
      'fbcp.enabled': {
        value: false,
      },
      'splash.length': {
        value: -1,
      },
      'splash.select': {
        value: 'all',
      },
      'manager.enabled': {
        value: true,
      },
      'emulators.specialkeys': {
        value: 'default',
      },
      hostname: {
        value: 'RECALBOX',
      },
      'samba.enabled': {
        value: true,
      },
      'virtual-gamepads.enabled': {
        value: true,
      },
      'ssh.enabled': {
        value: true,
      },
      'secondminitft.enabled': {
        value: false,
      },
      kblayout: {
        value: 'us',
      },
      'power.switch': {
        value: '',
      },
      'es.videomode': {
        value: '',
      },
      'es.force43': {
        value: false,
      },
      'splash.enabled': {
        value: true,
      },
    },
  } as SystemStoreState),

  getters: {
    languageOptions: (state) => displayableStringListToSelectOptions(state._systemOptions.language),
    kblayoutOptions: (state) => displayableStringListToSelectOptions(state._systemOptions.kblayout),
    timezoneOptions: (state) => allowedStringListToSelectOptions(state._systemOptions.timezone),
    specialkeysOptions: (state) => state._systemOptions['emulators.specialkeys'],
    esVideomodeOptions: (state) => displayableStringListToSelectOptions(state._systemOptions['es.videomode']),
    splashLengthOptions: (state) => state._systemOptions['splash.length'],
    splashSelectOptions: (state) => state._systemOptions['splash.select'],
    powerSwitchOptions: (state) => state._systemOptions['power.switch'],
    secondminitftTypeOptions: (state) => state._systemOptions['secondminitft.type'],
    secondminitftResolutionOptions: (state) => state._systemOptions['secondminitft.resolution'],
    secondminitftBacklightDurationOptions: (state) => state._systemOptions['secondminitft.backlightcontrol'],
  },
});
