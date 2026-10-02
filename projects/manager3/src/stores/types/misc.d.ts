import 'pinia';
import { AxiosInstance } from 'axios';
import { Router } from 'vue-router';

declare module 'pinia' {
  export interface PiniaCustomProperties {
    readonly router: Router;
    readonly _apiProvider: AxiosInstance;
    readonly _api80Provider: AxiosInstance;
    readonly _httpClientProvider: AxiosInstance;
    post: (data: Record<string, unknown>) => Promise<void>;
    fetch: () => Promise<void>;
    fetchOptions(): Promise<void>;
  }
}

export interface BiosColumn {
  name: string;
  align: string;
  label: string;
  field: string;
  sortable: boolean;
  required?: boolean|undefined;
}

export interface GlobalMenuLink {
  title: string;
  icon: string;
  route: string;
  exact?: boolean;
}

export interface SimpleStringListObject {
  type: string;
  allowedStringList: string[];
}

export interface StringListObject extends SimpleStringListObject {
  displayableStringList: string[];
}

export interface HomeStat {
  key: number;
  title: string;
  value: number;
  route?: string;
  percent?: number;
}

export interface SelectOption {
  label: string;
  value: string | number;
}

export interface BiosEntry {
  system: string;
  displayFileName: string;
  currentMd5: string;
  md5List: string[];
  lightStatus: 'Red' | 'Yellow' | 'Green';
}

export interface URL {
  [key: string]: string|undefined;
}

export interface RomEntry {
  favorite: boolean;
  name: string;
  publisher: string;
  developer: string;
  genre: string;
  players: number;
  rating: number;
}

export interface Item {
  recalbox: string;
  icon: string;
  mount: string;
  used: number;
  label?: string;
}
