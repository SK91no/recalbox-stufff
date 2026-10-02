/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { apiUrl } from 'boot/axios';
import { defineStore } from 'pinia';
import {
  CONFIGURATION,
  getPath,
  GLOBAL,
  SYSTEMS,
} from 'src/router/api.routes';
import { useTools } from 'src/composables/useTools';
import { systemsMetaData } from 'src/utils/systemsMetaData';
import { useSystemsStore } from 'stores/emulation/systems';
import {
  EmulationStationConfigOptionsResponse,
  EmulationStationConfigResponse,
  EmulationStationCurrentState,
  RomMetaData,
} from 'stores/configuration/emulationstation.d';
import { Actions, EventResponse } from 'stores/common/monitoring.d';

const { allowedStringListToSelectOptions } = useTools();

const { displayableStringListToSelectOptions } = useTools();
const api: string|undefined = apiUrl;

export interface EmulationStationStoreState {
  _baseUrl: string;
  _emulationstationOptions: EmulationStationConfigOptionsResponse;
  emulationstation: EmulationStationConfigResponse;
  currentState: EmulationStationCurrentState;
}

export const useEmulationstationStore = defineStore('emulationstation', {
  state: () => ({
    _baseUrl: CONFIGURATION.emustation,
    _emulationstationOptions: {
      'screensaver.type': {
        allowedStringList: [] as string[],
      },
      'screensaver.time': {
        lowerValue: 0,
        higherValue: 30,
      },
      brightness: {
        lowerValue: 0,
        higherValue: 8,
      },
      'popoup.help': {
        lowerValue: 0,
        higherValue: 10,
      },
      'popoup.music': {
        lowerValue: 0,
        higherValue: 10,
      },
      'popoup.netplay': {
        lowerValue: 0,
        higherValue: 10,
      },
      systemsorting: {
        allowedStringList: [] as string[],
      },
      'virtualarcade.manufacturers': {
        allowedStringList: [] as string[],
      },
      'pads.osd.type': {
        allowedStringList: [] as string[],
      },
      menu: {
        allowedStringList: [] as string[],
      },
      selectedsystem: {
        allowedStringList: [] as string[],
        displayableStringList: [] as string[],
      },
      'theme.folder': {
        allowedStringList: [] as string[],
        displayableStringList: [] as string[],
      },
      'theme.transition': {
        allowedStringList: [] as string[],
      },
      'theme.region': {
        allowedStringList: [] as string[],
      },
      rompreferredregion: {
        allowedStringList: [] as string[],
      },
      romsecondpreferredregion: {
        allowedStringList: [] as string[],
      },
      rompreferredfallback: {
        allowedStringList: [] as string[],
      },
      'game.transition': {
        allowedStringList: [] as string[],
      },
    },
    emulationstation: {
      menu: {
        value: 'default',
      },
      selectedsystem: {
        value: 'favorites',
      },
      bootongamelist: {
        value: false,
      },
      hidesystemview: {
        value: false,
      },
      gamelistonly: {
        value: false,
      },
      forcebasicgamelistview: {
        value: false,
      },
      debuglogs: {
        value: false,
      },
      'screensaver.time': {
        value: 5,
      },
      'screensaver.type': {
        value: 'dim',
      },
      'popoup.help': {
        value: 10,
      },
      'popoup.music': {
        value: 5,
      },
      'popoup.netplay': {
        value: 8,
      },
      'theme.carousel': {
        value: true,
      },
      'theme.transition': {
        value: 'slide',
      },
      'theme.folder': {
        value: 'recalbox-next',
      },
      'theme.region': {
        value: 'eu',
      },
      brightness: {
        value: 7,
      },
      clock: {
        value: false,
      },
      showhelp: {
        value: true,
      },
      showgamecliphelpitems: {
        value: true,
      },
      showgameclipclippingitem: {
        value: true,
      },
      quicksystemselect: {
        value: true,
      },
      filteradultgames: {
        value: true,
      },
      favoritesonly: {
        value: false,
      },
      showhidden: {
        value: false,
      },
      showonlylatestversion: {
        value: false,
      },
      hidenogames: {
        value: false,
      },
      'collection.lastplayed': {
        value: false,
      },
      'collection.multiplayer': {
        value: false,
      },
      'collection.allgames': {
        value: false,
      },
      'collection.tate': {
        value: false,
      },
      'virtualarcade.manufacturers': {
        value: '',
      },
      virtualarcade: {
        value: false,
      },
      'virtualarcade.hideoriginals': {
        value: true,
      },
      'arcade.usedatabasenames': {
        value: true,
      },
      'arcade.view.enhanced': {
        value: true,
      },
      'arcade.view.hidebios': {
        value: false,
      },
      'arcade.view.hideclones': {
        value: false,
      },
      'arcade.view.hidenonworking': {
        value: false,
      },
      'battery.hidden': {
        value: false,
      },
      'pads.osd': {
        value: false,
      },
      'pads.osd.type': {
        value: '',
      },
      onegameonerom: {
        value: false,
      },
      rompreferredregion: {
        value: 'eu',
      },
      romsecondpreferredregion: {
        value: 'wor',
      },
      rompreferredfallback: {
        value: 'World > Europe > USA > Japan',
      },
      displaybyfilename: {
        value: false,
      },
      'game.transition': {
        value: 'slide',
      },
      hideboardgames: {
        value: false,
      },
      showonly3plusplayers: {
        value: false,
      },
      yokoonly: {
        value: false,
      },
    },
    currentState: {
      currentSystem: null,
      currentRom: null,
    },
  } as EmulationStationStoreState),

  getters: {
    menuOptions: (state) => state._emulationstationOptions.menu,
    selectedsystemOptions: (state) => displayableStringListToSelectOptions(state._emulationstationOptions.selectedsystem),
    screensaverTimeOptions: (state) => state._emulationstationOptions['screensaver.time'],
    screensaverTypeOptions: (state) => state._emulationstationOptions['screensaver.type'],
    themeFolderOptions: (state) => displayableStringListToSelectOptions(state._emulationstationOptions['theme.folder']),
    themeTransitionOptions: (state) => state._emulationstationOptions['theme.transition'],
    themeRegionOptions: (state) => state._emulationstationOptions['theme.region'],
    brightnessOptions: (state) => state._emulationstationOptions.brightness,
    popupHelpOptions: (state) => state._emulationstationOptions['popoup.help'],
    popupMusicOptions: (state) => state._emulationstationOptions['popoup.music'],
    popupNetplayOptions: (state) => state._emulationstationOptions['popoup.netplay'],
    systemSortingOptions: (state) => state._emulationstationOptions.systemsorting,
    virtualArcadeManufacturersOptions: (state) => state._emulationstationOptions['virtualarcade.manufacturers'],
    padsOsdTypeOptions: (state) => state._emulationstationOptions['pads.osd.type'],
    romPreferredRegionOptions: (state) => state._emulationstationOptions.rompreferredregion,
    romSecondPreferredRegionOptions: (state) => state._emulationstationOptions.romsecondpreferredregion,
    romPreferredFallbackOptions: (state) => allowedStringListToSelectOptions(state._emulationstationOptions.rompreferredfallback),
    gameTransitionOptions: (state) => state._emulationstationOptions['game.transition'],
  },

  actions: {
    resetCurrentSystem() {
      if (this.currentState.currentSystem) {
        this.currentState.currentSystem = null;
      }
    },
    async updateStatus(status: EventResponse) {
      const systemsStore = useSystemsStore();
      const { systems } = systemsStore;

      let { currentSystem } = this.currentState;
      let currentRom = null;

      // Add Kodi in the systems list here to make it visible on MQTT result
      systems.systems[systems.systems.length] = {
        name: 'kodi',
        fullName: 'Kodi',
        uuid: '',
        manufacturer: '',
        releaseDate: 0,
        romPath: [''],
        extensions: '',
        type: 8,
        ignoredFiles: '',
        inputs: {
          pads: 0,
          keyboard: 0,
          mouse: 0,
        },
        properties: {
          hasLightgunSupport: false,
          isReadOnly: false,
          hasNetplay: false,
        },
        emulators: [],
        themeFolder: 'kodi',
        themeRegion: 'eu',
      };

      if (status.System) {
        const storeSystem = systems.systems.filter((system) => system.name === status.System.SystemId)[0];
        if (this.currentState.currentSystem && storeSystem.name !== this.currentState.currentSystem.systemId) {
          this.resetCurrentSystem();
        }
        if (storeSystem) {
          currentSystem = {
            logoPath: `${api}/systems/${storeSystem.name.replace('\u00a0', '')}/resource/${storeSystem.themeRegion}/svg/logo`,
            consolePath: `${api}/systems/${storeSystem.name.replace('\u00a0', '')}/resource/${storeSystem.themeRegion}/svg/console`,
            gamePath: `${api}/systems/${storeSystem.name.replace('\u00a0', '')}/resource/${storeSystem.themeRegion}/svg/game`,
            name: status.System.System,
            systemId: status.System.SystemId,
            metaData: systemsMetaData[status.System.SystemId.replace('\u00a0', '')],
          };
          if (([Actions.runGame, Actions.runDemo].includes(status.Action)) && status.Game) {
            const encodedGamePath = encodeURIComponent(status.Game.GamePath);
            const metaData = await this.fetchRomMetaData(status.System.SystemId, encodedGamePath);
            currentRom = {
              name: status.Game.Game,
              imagePath: `${api}/systems/${storeSystem.name.replace('\u00a0', '')}/roms/metadata/image/${encodedGamePath}`,
              thumbnailPath: `${api}/systems/${storeSystem.name.replace('\u00a0', '')}/roms/metadata/thumbnail/${encodedGamePath}`,
              videoPath: `${api}/systems/${storeSystem.name.replace('\u00a0', '')}/roms/metadata/video/${encodedGamePath}`,
              developer: status.Game.Developer,
              publisher: status.Game.Publisher,
              players: status.Game.Players,
              region: status.Game.Region,
              genre: status.Game.Genre,
              metaData,
            };
          }
        }
      }

      const currentState = {
        currentAction: status.Action,
        currentSystem,
        currentRom,
      };

      this.$patch({
        currentState,
      });
    },
    async fetchRomMetaData(systemName: string, encodedGamePath: string): Promise<RomMetaData> {
      let result = null;
      try {
        const response = await this._apiProvider.get(
          getPath(SYSTEMS.romsMetaData, { systemName, romPath: encodedGamePath }),
        );
        result = response.data;
      } catch (error) {
        console.error(error);
      }
      return result;
    },
    async fetchStatus() {
      try {
        const response = await this._apiProvider.get(GLOBAL.status);
        const status = response.data;

        this.updateStatus(status);
      } catch (error) {
        console.error(error);
      }
    },
  },
});
