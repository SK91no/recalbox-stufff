/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { PiniaCustomProperties, PiniaPluginContext } from 'pinia';

const FetchOptionsStorePlugin = (context: PiniaPluginContext): void | Partial<PiniaCustomProperties> => {
  const allowedStores = [
    'audio',
    'autorun',
    'controllers',
    'emulationstation',
    'global',
    'hat',
    'kodi',
    'pinball',
    'scraper',
    'system',
    'tate',
    'updates',
    'wifi',
    'wifi2',
    'wifi3',
  ];

  if (allowedStores.includes(context.store.$id)) {
    return {
      async fetchOptions() {
        try {
          const response = await context.store._apiProvider.options(context.store._baseUrl);
          context.store[`_${context.store.$id}Options`] = response.data;
        } catch (error) {
          console.error(error);
        }
      },
    };
  }

  return undefined;
};

export default FetchOptionsStorePlugin;
