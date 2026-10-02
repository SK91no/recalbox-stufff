/**
 * @author Nicolas TESSIER aka Asthonishia
 */
export const getPath = (route: string, args: Record<string, string>) => {
  Object.keys(args).forEach((key) => {
    route = route.replace(`{${key}}`, args[key]);
  });
  return route;
};

export const GLOBAL = {
  architecture: '/architecture',
  status: '/status',
  versions: '/versions',
};

export const BIOS = {
  root: '/bios',
  all: '/bios',
  export: '/bios/download',
  upload: '/bios/upload/',
};

export const CONFIGURATION = {
  audio: '/configuration/audio',
  autorun: '/configuration/autorun',
  cardreader: '/configuration/cardreader',
  controllers: '/configuration/controllers',
  emustation: '/configuration/emulationstation',
  frontend: '/configuration/frontend',
  global: '/configuration/global',
  hat: '/configuration/hat',
  hyperion: '/configuration/hyperion',
  kodi: '/configuration/kodi',
  lircd: '/configuration/lircd',
  music: '/configuration/music',
  patreon: '/configuration/patron',
  pinball: '/configuration/pinball',
  scraper: '/configuration/scraper',
  screenshots: '/configuration/screenshots',
  system: '/configuration/system',
  systems: '/configuration/system/{systemName}',
  tate: '/configuration/tate',
  updates: '/configuration/updates',
  wifi: '/configuration/wifi',
  wifi2: '/configuration/wifi2',
  wifi3: '/configuration/wifi3',
};

export const SYSTEM = {
  es: {
    start: '/system/frontend/start',
    stop: '/system/frontend/stop',
    restart: '/system/frontend/restart',
  },
  reboot: '/system/reboot',
  shutdown: '/system/shutdown',
  supportArchive: '/system/supportarchive/generate',
  resetFactory: '/system/resetfactory',
};

export const MEDIA = {
  root: '/media',
  all: '/media',
  get: '/media/screenshot/',
  delete: '/media/',
  takeScreenshot: '/media/takescreenshot',
};

export const MONITORING = {
  cpuInfo: '/monitoring/cpuinfo',
  storageInfo: '/monitoring/storageinfo',
};

export const ROMS = {
  root: '/roms',
  all: '/roms',
  total: '/roms/total',
};

export const SYSTEMS = {
  root: '/systems',
  all: '/systems',
  roms: '/systems/{systemName}/roms',
  romsMetaData: '/systems/{systemName}/roms/metadata/info/{romPath}',
};

export const THEMES = {
  colorset: '/themes/{themeNameFolder}/colorset',
  gameclipview: '/themes/{themeNameFolder}/gameclipview',
  gamelistview: '/themes/{themeNameFolder}/gamelistview',
  iconset: '/themes/{themeNameFolder}/iconset',
  menuset: '/themes/{themeNameFolder}/menuset',
  systemview: '/themes/{themeNameFolder}/systemview',
};
