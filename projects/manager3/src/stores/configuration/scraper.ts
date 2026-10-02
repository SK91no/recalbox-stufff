/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import { useTools } from 'composables/useTools';
import {
  ScraperConfigOptionsResponse,
  ScraperConfigResponse,
} from 'stores/configuration/scraper.d';

const { displayableStringListToSelectOptions } = useTools();

export interface ScraperStoreState {
  _baseUrl: string;
  _scraperOptions: ScraperConfigOptionsResponse;
  scraper: ScraperConfigResponse;
}

export const useScraperStore = defineStore('scraper', {
  state: () => ({
    _baseUrl: CONFIGURATION.scraper,
    _scraperOptions: {
      'screenscraper.media': {
        allowedStringList: [] as string[],
      },
      getnamefrom: {
        allowedStringList: [] as string[],
      },
      'screenscraper.language': {
        allowedStringList: [''],
        displayableStringList: [''],
      },
      'screenscraper.region': {
        allowedStringList: [''],
        displayableStringList: [''],
      },
      'screenscraper.thumbnail': {
        allowedStringList: [] as string[],
      },
      'screenscraper.video': {
        allowedStringList: [] as string[],
      },
      source: {
        allowedStringList: [''],
        displayableStringList: [''],
      },
      'screenscraper.regionPriority': {
        allowedStringList: [] as string[],
      },
    },
    scraper: {
      getnamefrom: {
        value: '0',
      },
      extractregionfromfilename: {
        value: '1',
      },
      source: {
        value: 'ScreenScraper',
      },
      auto: {
        value: true,
      },
      'screenscraper.user': {
        value: '',
      },
      'screenscraper.password': {
        value: '',
      },
      'screenscraper.manual': {
        value: false,
      },
      'screenscraper.maps': {
        value: false,
      },
      'screenscraper.p2k': {
        value: false,
      },
      'screenscraper.regionPriority': {
        value: 'DetectedRegion',
      },
      'screenscraper.language': {
        value: 'fr',
      },
      'screenscraper.region': {
        value: 'wor',
      },
    },
  } as ScraperStoreState),

  getters: {
    mediaOptions: (state) => state._scraperOptions['screenscraper.media'],
    languageOptions: (state) => displayableStringListToSelectOptions(state._scraperOptions['screenscraper.language']),
    regionOptions: (state) => state._scraperOptions['screenscraper.region'],
    thumbnailOptions: (state) => state._scraperOptions['screenscraper.thumbnail'],
    videoOptions: (state) => state._scraperOptions['screenscraper.video'],
    getNameFromOptions: (state) => state._scraperOptions.getnamefrom,
    sourceOptions: (state) => displayableStringListToSelectOptions(state._scraperOptions.source),
    regionPriorityOptions: (state) => state._scraperOptions['screenscraper.regionPriority'],
  },
});
