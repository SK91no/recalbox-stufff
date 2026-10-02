/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { useTools } from 'src/composables/useTools';
import { CONFIGURATION } from 'src/router/api.routes';
import {
  GlobalConfigOptionsResponse,
  GlobalConfigResponse,
} from 'stores/configuration/global.d';

const { allowedStringListToSelectOptions, displayableStringListToSelectOptions } = useTools();

export interface GlobalStoreState {
  _baseUrl: string;
  _globalOptions: GlobalConfigOptionsResponse;
  global: GlobalConfigResponse;
}

export const useGlobalStore = defineStore('global', {
  state: () => ({
    _baseUrl: CONFIGURATION.global,
    _globalOptions: {
      ratio: {
        allowedStringList: [] as string[],
      },
      shaderset: {
        allowedStringList: [] as string[],
      },
      'shaderset.file': {
        allowedStringList: [''],
      },
      'translate.from': {
        allowedStringList: [''],
      },
      'translate.to': {
        allowedStringList: [''],
      },
      videomode: {
        allowedStringList: [''],
        displayableStringList: [''],
      },
      'demo.infoscreenduration': {
        lowerValue: 5,
        higherValue: 30,
      },
      'demo.duration': {
        lowerValue: 30,
        higherValue: 600,
      },
      'demo.systemlist': {
        allowedStringList: [] as string[],
        displayableStringList: [''],
      },
      'netplay.relay': {
        allowedStringList: [] as string[],
      },
      softpatching: {
        allowedStringList: [] as string[],
      },
      'netplay.port': {
        lowerValue: 1,
        higherValue: 65535,
      },
      'netplay.nickname': {
        allowedChars: '',
      },
      'lightgun.luminosity': {
        lowerValue: 0,
        higherValue: 3,
      },
      autoblitter: {
        allowedStringList: [] as string[],
      },
      crthandheldformat: {
        allowedStringList: [] as string[],
      },
    },
    global: {
      recalboxoverlays: {
        value: true,
      },
      videomode: {
        value: '',
      },
      shaderset: {
        value: 'none',
      },
      integerscale: {
        value: false,
      },
      shaders: {
        value: '',
      },
      ratio: {
        value: 'auto',
      },
      smooth: {
        value: true,
      },
      rewind: {
        value: true,
      },
      autosave: {
        value: false,
      },
      retroachievements: {
        value: false,
      },
      'retroachievements.hardcore': {
        value: false,
      },
      'retroachievements.username': {
        value: '',
      },
      'retroachievements.password': {
        value: '',
      },
      'demo.systemlist': {
        value: '3do,amigacd32,atari2600,atari5200,atari7800,daphne,fbneo,fds,gamegear,gba,lynx,mame,mastersystem,megadrive,neogeo,nes,ngpc,pcengine,sega32x,sg1000,snes',
      },
      translate: {
        value: true,
      },
      'translate.from': {
        value: 'auto',
      },
      'translate.to': {
        value: 'auto',
      },
      'translate.apikey': {
        value: 'RECALBOX',
      },
      'translate.url': {
        value: '',
      },
      netplay: {
        value: true,
      },
      'netplay.nickname': {
        value: '',
      },
      'netplay.port': {
        value: 55435,
      },
      'netplay.relay': {
        value: 'none',
      },
      'netplay.lobby': {
        value: 'http://lobby.libretro.com/list/',
      },
      quitpresstwice: {
        value: false,
      },
      hidepreinstalledgames: {
        value: false,
      },
      softpatching: {
        value: 'disable',
      },
      showfps: {
        value: false,
      },
      'demo.duration': {
        value: 90,
      },
      'demo.infoscreenduration': {
        value: 6,
      },
      reducelatency: {
        value: false,
      },
      runahead: {
        value: false,
      },
      'show.savestate.before.run': {
        value: false,
      },
      hdmode: {
        value: false,
      },
      widescreenmode: {
        value: false,
      },
      rumble: {
        value: false,
      },
      'lightgun.luminosity': {
        value: 0,
      },
      autoblitter: {
        value: 'recalbox',
      },
      crthandheldformat: {
        value: 'fullscreen',
      },
    },
  } as GlobalStoreState),

  getters: {
    ratioOptions: (state) => state._globalOptions.ratio,
    shadersetOptions: (state) => state._globalOptions.shaderset,
    shadersetfileOptions: (state) => allowedStringListToSelectOptions(state._globalOptions['shaderset.file']),
    translateFromOptions: (state) => allowedStringListToSelectOptions(state._globalOptions['translate.from']),
    translateToOptions: (state) => allowedStringListToSelectOptions(state._globalOptions['translate.to']),
    videomodeOptions: (state) => allowedStringListToSelectOptions(state._globalOptions.videomode),
    demoInfoscreendurationOptions: (state) => state._globalOptions['demo.infoscreenduration'],
    demoDurationOptions: (state) => state._globalOptions['demo.duration'],
    demoSystemlistOptions: (state) => displayableStringListToSelectOptions(state._globalOptions['demo.systemlist']),
    netplayRelayOptions: (state) => state._globalOptions['netplay.relay'],
    softpatchingOptions: (state) => state._globalOptions.softpatching,
    autoblitterOptions: (state) => state._globalOptions.autoblitter,
  },
});
