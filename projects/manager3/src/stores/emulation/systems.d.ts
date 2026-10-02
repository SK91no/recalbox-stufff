/**
 * @author Nicolas TESSIER aka Asthonishia
 */

export interface Inputs {
  pads: number;
  keyboard: number;
  mouse: number;
}

export interface Properties {
  hasLightgunSupport: boolean;
  isReadOnly: boolean;
  hasNetplay: boolean;
}

export interface System {
  name: string;
  fullName: string;
  uuid: string;
  manufacturer: string;
  releaseDate: number;
  romPath: string[];
  extensions: string;
  type: number;
  ignoredFiles: string;
  inputs: Inputs;
  properties: Properties;
  emulators: string[];
  themeFolder: string;
  themeRegion: string;
}

export enum Type {
  Unknown,
  Arcade,
  HomeConsole,
  HandheldConsole,
  Computer,
  FantasyConsole,
  GameEngine,
  Port,
  VirtualSystem,
  VirtualArcade,
}

export type SystemType = Record<string, string>

export type DeviceRequirement = Record<string, string>

export type EmulatorSpeed = Record<string, string>

export type EmulatorCompatibility = Record<string, string>

export interface Enumerations {
  systemTypes: SystemType[];
  deviceRequirement: DeviceRequirement[];
  emulatorSpeed: EmulatorSpeed[];
  emulatorCompatibility: EmulatorCompatibility[];
}

export interface SystemsResponse {
  enumerations: Enumerations;
  systems: System[]|[];
}
