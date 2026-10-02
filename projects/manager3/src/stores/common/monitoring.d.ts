/**
 * @author Pit64
 */
export interface Emulator {
  Emulator: string;
  Core: string;
}

export interface Game {
  Game: string;
  GamePath: string;
  IsFolder: boolean;
  ImagePath: string;
  ThumbnailPath: string;
  VideoPath: string;
  Developer: string;
  Publisher: string;
  Players: string;
  Region: string;
  Genre: string;
  GenreId: string;
  Favorite: boolean;
  Hidden: boolean;
  Adult: boolean;
  SelectedEmulator: Emulator;
}

export interface System {
  System: string;
  SystemId: string;
  DefaultEmulator: Emulator;
}

export enum Actions {
  configurationChanged = 'configurationchanged',
  systemBrowsing = 'systembrowsing',
  gamelistBrowsing = 'gamelistbrowsing',
  runGame = 'rungame',
  endGame = 'endgame',
  runDemo = 'rundemo',
  endDemo = 'enddemo',
  sleep = 'sleep',
  start = 'start',
  stop = 'stop',
  shutdown = 'shutdown',
  reboot = 'reboot',
  wakeup = 'wakeup',
  quit = 'quit',
  relaunch = 'relaunch',
  scrapStart = 'scrapstart',
  scrapStop = 'scrapstop',
  scrapGame = 'scrapgame',
  startGameClip = 'startgameclip',
  runKodi = 'runkodi',
}

export interface EventResponse {
  Action: Actions;
  Parameter: string;
  Version: string;
  System: System;
  Game: Game;
}

export interface Storage {
  recalbox: string;
  devicebus: string;
  devicetype: string;
  devicemodel: string;
  mount: string;
  filesystem: string;
  filesystemtype: string;
  size: number;
  used: number;
}

export interface StoragesResponse {
  storages: Record<string, Storage>;
}

export type Cpu = Record<string, {
  consumption: number[];
}>

export interface Memory {
  total: number;
  free: number[];
  available: number[];
}

export interface Temperature {
  unit: '°C',
  temperatures: number[];
}

export interface MonitoringResponse {
  timestamp: string;
  cpus: Cpu,
  memory: Memory;
  temperature: Temperature;
}
