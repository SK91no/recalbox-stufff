/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { PiniaCustomProperties, PiniaPluginContext } from 'pinia';

const FetchStorePlugin = (context: PiniaPluginContext): void | Partial<PiniaCustomProperties> => {
  const allowedStores = [
    'architecture',
    'audio',
    'autorun',
    'bios',
    'controllers',
    'emulationstation',
    'frontend',
    'global',
    'hat',
    'hyperion',
    'kodi',
    'lircd',
    'media',
    'monitoring',
    'music',
    'patreon',
    'pinball',
    'roms',
    'scraper',
    'system',
    'systems',
    'tate',
    'updates',
    'versions',
    'wifi',
    'wifi2',
    'wifi3',
    'system',
    'updates',
  ];

  if (allowedStores.includes(context.store.$id)) {
    return {
      async fetch() {
        try {
          const response = await context.store._apiProvider.get(context.store._baseUrl);
          const currentState = context.store.$state[context.store.$id];
          const apiData = response.data;

          // Merge API data with default values
          const mergedData = { ...apiData };
          Object.keys(apiData).forEach((key) => {
            const defaultValue = currentState[key]?.value;
            // Use default value if exist is false and we have a default value
            if (apiData[key]?.exist === false && defaultValue !== undefined) {
              mergedData[key].value = defaultValue;
            }
          });

          context.store[context.store.$id] = mergedData;
        } catch (error) {
          console.error(error);
        }
      },
    };
  }

  return undefined;
};

export default FetchStorePlugin;
