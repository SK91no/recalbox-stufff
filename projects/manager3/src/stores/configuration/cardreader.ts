/**
 * @author Pit64
 */
import { defineStore } from 'pinia';
import { CONFIGURATION } from 'src/router/api.routes';
import {
  CardReaderConfigOptionsResponse,
  CardReaderConfigResponse,
} from 'stores/configuration/cardreader.d';

export interface CardReaderStoreState {
  _baseUrl: string;
  _cardReaderOptions: CardReaderConfigOptionsResponse;
  cardReader: CardReaderConfigResponse;
}

export const useCardReaderStore = defineStore('cardreader', {
  state: () => ({
    _baseUrl: CONFIGURATION.cardreader,
    _cardReaderOptions: {},
    cardReader: {
      consolemode: {
        value: false,
      },
      consolemodeexit: {
        value: 'hkstart',
      },
      consolemodevideo: {
        value: 'bios1',
      },
      consolemodesound: {
        value: 'bios',
      },
    },
  } as CardReaderStoreState),

  getters: {},
});
