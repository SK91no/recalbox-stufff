/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { StringListObject, SimpleStringListObject } from 'stores/types/misc';
import { Actions } from 'stores/monitoring.d';

export interface EmulationStationConfigResponse {
  bootongamelist: {
    exist: boolean;
    value: boolean;
  };
  filteradultgames: {
    exist: boolean;
    value: boolean;
  };
  gamelistonly: {
    exist: boolean;
    value: boolean;
  };
  forcebasicgamelistview: {
    exist: boolean;
    value: boolean;
  };
  menu: {
    exist: boolean;
    value: string;
  };
  'collection.lastplayed': {
    exist: boolean;
    value: boolean;
  };
  selectedsystem: {
    exist: boolean;
    value: string;
  };
  'collection.multiplayer': {
    exist: boolean;
    value: boolean;
  };
  'collection.allgames': {
    exist: boolean;
    value: boolean;
  };
  hidesystemview: {
    exist: boolean;
    value: boolean;
  };
  clock: {
    exist: boolean;
    value: boolean;
  };
  favoritesonly: {
    exist: boolean;
    value: boolean;
  };
  showgameclipclippingitem: {
    exist: boolean;
    value: boolean;
  };
  showgamecliphelpitems: {
    exist: boolean;
    value: boolean;
  };
  'screensaver.time': {
    exist: boolean;
    value: number;
  };
  'screensaver.type': {
    exist: boolean;
    value: string;
  };
  'theme.folder': {
    exist: boolean;
    value: string;
  };
  'collection.tate': {
    exist: boolean;
    value: boolean;
  };
  virtualarcade: {
    exist: boolean;
    value: boolean;
  };
  'virtualarcade.hideoriginals': {
    exist: boolean;
    value: boolean;
  };
  'collection.ports.hide': {
    exist: boolean;
    value: boolean;
  };
  'collection.lightgun.hide': {
    exist: boolean;
    value: boolean;
  };
  quicksystemselect: {
    exist: boolean;
    value: boolean;
  };
  showhelp: {
    exist: boolean;
    value: boolean;
  };
  'popoup.help': {
    exist: boolean;
    value: number;
  };
  'popoup.music': {
    exist: boolean;
    value: number;
  };
  'popoup.netplay': {
    exist: boolean;
    value: number;
  };
  systemsorting: {
    exist: boolean;
    value: string;
  };
  'theme.carousel': {
    exist: boolean;
    value: boolean;
  };
  'theme.transition': {
    exist: boolean;
    value: string;
  };
  'theme.region': {
    exist: boolean;
    value: string;
  };
  brightness: {
    exist: boolean;
    value: number;
  };
  showhidden: {
    exist: boolean;
    value: boolean;
  };
  showonlylatestversion: {
    exist: boolean;
    value: boolean;
  };
  hidenogames: {
    exist: boolean;
    value: boolean;
  };
  'arcade.view.enhanced': {
    exist: boolean;
    value: boolean;
  };
  'arcade.view.hideclones': {
    exist: boolean;
    value: boolean;
  };
  'arcade.view.hidebios': {
    exist: boolean;
    value: boolean;
  };
  'arcade.view.hidenonworking': {
    exist: boolean;
    value: boolean;
  };
  'arcade.usedatabasenames': {
    exist: boolean;
    value: boolean;
  };
  'virtualarcade.manufacturers': {
    exist: boolean;
    value: string;
  };
  debuglogs: {
    exist: boolean;
    value: boolean;
  };
  'pads.osd': {
    exist: boolean;
    value: boolean;
  };
  'pads.osd.type': {
    exist: boolean;
    value: string;
  };
  tateonly: {
    exist: boolean;
    value: boolean;
  };
  'battery.hidden': {
    exist: boolean;
    value: boolean;
  };
  onegameonerom: {
    exist: boolean;
    value: boolean;
  };
  rompreferredregion: {
    exist: boolean;
    value: string;
  };
  romsecondpreferredregion: {
    exist: boolean;
    value: string;
  };
  rompreferredfallback: {
    exist: boolean;
    value: string;
  };
  displaybyfilename: {
    exist: boolean;
    value: boolean;
  };
  'game.transition': {
    exist: boolean;
    value: string;
  };
  hideboardgames: {
    exist: boolean;
    value: boolean;
  };
  showonly3plusplayers: {
    exist: boolean;
    value: boolean;
  };
  yokoonly: {
    exist: boolean;
    value: boolean;
  };
}

export interface EmulationStationConfigOptionsResponse {
  bootongamelist: {
    type: string;
  };
  filteradultgames: {
    type: string;
    value: boolean;
  };
  gamelistonly: {
    type: string;
    value: boolean;
  };
  forcebasicgamelistview: {
    type: string;
    value: boolean;
  };
  menu: SimpleStringListObject;
  'collection.lastplayed': {
    type: string;
    value: boolean;
  };
  selectedsystem: StringListObject;
  'collection.multiplayer': {
    type: string;
    value: boolean;
  };
  'collection.allgames': {
    type: string;
    value: boolean;
  };
  hidesystemview: {
    type: string;
    value: boolean;
  };
  clock: {
    type: string;
    value: boolean;
  };
  favoritesonly: {
    type: string;
    value: boolean;
  };
  showgameclipclippingitem: {
    type: string;
    value: boolean;
  };
  showgamecliphelpitems: {
    type: string;
    value: boolean;
  };
  'screensaver.time': {
    type: string;
    lowerValue: number;
    higherValue: number;
  };
  'screensaver.type': SimpleStringListObject;
  'theme.folder': StringListObject;
  'collection.tate': {
    type: string;
    value: boolean;
  };
  virtualarcade: {
    type: string;
    value: boolean;
  };
  'virtualarcade.hideoriginals': {
    type: string;
    value: boolean;
  };
  'collection.ports.hide': {
    type: string;
    value: boolean;
  };
  'collection.lightgun.hide': {
    type: string;
    value: boolean;
  };
  quickselectsystem: {
    type: string;
    value: boolean;
  };
  showhelp: {
    type: string;
    value: boolean;
  };
  'popoup.help': {
    type: string;
    lowerValue: number;
    higherValue: number;
  };
  'popoup.music': {
    type: string;
    lowerValue: number;
    higherValue: number;
  };
  'popoup.netplay': {
    type: string;
    lowerValue: number;
    higherValue: number;
  };
  systemsorting: SimpleStringListObject;
  'theme.carousel': {
    type: string;
    value: boolean;
  };
  'theme.transition': SimpleStringListObject;
  'theme.region': SimpleStringListObject;
  brightness: {
    type: string;
    lowerValue: number;
    higherValue: number;
  };
  showhidden: {
    type: string;
    value: boolean;
  };
  showonlylatestversion: {
    type: string;
    value: boolean;
  };
  hidenogames: {
    type: string;
    value: boolean;
  };
  'arcade.view.enhanced': {
    type: string;
    value: boolean;
  };
  'arcade.view.hideclones': {
    type: string;
    value: boolean;
  };
  'arcade.view.hidebios': {
    type: string;
    value: boolean;
  };
  'arcade.view.hidenonworking': {
    type: string;
    value: boolean;
  };
  'arcade.usedatabasenames': {
    type: string;
    value: boolean;
  };
  'virtualarcade.manufacturers': StringListObject;
  debuglogs: {
    type: string;
    value: boolean;
  };
  'pads.osd': {
    type: string;
    value: boolean;
  };
  'pads.osd.type': SimpleStringListObject;
  tateonly: {
    type: string;
    value: boolean;
  };
  'battery.hidden': {
    type: string;
    value: boolean;
  };
  onegameonerom: {
    type: string;
    value: boolean;
  };
  rompreferredregion: StringListObject;
  romsecondpreferredregion: StringListObject;
  rompreferredfallback: SimpleStringListObject;
  displaybyfilename: {
    type: string;
    value: boolean;
  };
  'game.transition': SimpleStringListObject;
  hideboardgames: {
    type: string;
    value: boolean;
  };
  showonly3plusplayers: {
    type: string;
    value: boolean;
  };
  yokoonly: {
    type: string;
    value: boolean;
  };
}

export interface CurrentSystemMetaDataColor {
  band1: string;
  band2: string;
  band3: string;
  band4: string;
}

export interface CurrentSystemMetaDataSystem {
  manufacturer?: string;
  yearOfRelease?: string;
  cpu?: string;
  ram?: string;
  gpu?: string;
  soundChip?: string;
  colors?: string;
  os?: string;
  rom?: string;
  resolution?: string;
  unitsSold?: string;
  bestSellingGame?: string;
  author?: string;
  sound?: string;
  display?: string;
  website?: string;
  builtInFeatures?: string;
  video?: string;
  developers?: string;
  device?: string;
  cartSize?: string;
  cartridgeRom?: string;
  screen?: string;
  displayAbility?: string;
  storage?: string;
  media?: string;
  fcram?: string;
  vram?: string;
  vmuSize?: string;
  memory?: string;
  sprites?: string;
  numberGames?: string;
  sram?: string;
  videoRam?: string;
  audioRam?: string;
  dram?: string;
  writtenIn?: string;
  graphics?: string;
  lowResolutions?: string;
  programMemory?: string;
  diskSpace?: string;
  ports?: string;
  bsX?: string;
  drive?: string;
  createdBy?: string;
  portedOnLinuxBy?: string;
  type?: string;
  pcCompatibility?: string;
  necessarySoft?: string;
  '64ddCpu'?: string;
  cpu2?: string;
  designer?: string;
  mediumResolutions?: string;
  origin?: string;
  englishTranslation?: string;
  highResolutions?: string;
  bestLaserdiscGamesAre?: string;
  size?: string;
  weight?: string;
}

export interface CurrentSystemMetaDataColors {
  [key: string]: CurrentSystemMetaDataColor;
}

export interface CurrentSystemMetaData {
  system: CurrentSystemMetaDataSystem;
  colors: CurrentSystemMetaDataColors;
}

export interface CurrentSystem {
  name: string;
  systemId: string;
  logoPath: string;
  consolePath: string;
  gamePath: string;
  metaData: CurrentSystemMetaData;
}

export interface RomMetaData {
  name: string;
  synopsys: string;
  publisher: string;
  developer: string;
  emulator: string;
  core: string;
  ratio: string;
  releaseDate: number;
  lastPlayed: number;
  regions: string;
  playCount: number;
  crc32: string;
  adult: boolean;
  favorite: boolean;
  hidden: boolean;
  rating: number;
  players: {
    min: number;
    max: number;
  };
  genres: {
    free: string;
    normalized: number;
  };
  availableMedia: {
    hasImage: boolean;
    hasThumbnail: boolean;
    hasVideo: boolean;
  };
}

export interface CurrentRom {
  name: string;
  imagePath: string;
  thumbnailPath: string;
  videoPath: string;
  developer: string;
  publisher: string;
  players: string;
  region: string;
  genre: string;
  metaData: RomMetaData|null;
}

export interface EmulationStationCurrentState {
  currentSystem: CurrentSystem|null,
  currentRom: CurrentRom|null,
  currentAction: Actions;
}
