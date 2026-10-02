/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { PiniaCustomProperties, PiniaPluginContext } from 'pinia';

const PostStorePlugin = (context: PiniaPluginContext): void | Partial<PiniaCustomProperties> => {
  const allowedStores = [
    'audio',
    'autorun',
    'controllers',
    'emulationstation',
    'frontend',
    'global',
    'hat',
    'hyperion',
    'kodi',
    'lircd',
    'music',
    'patreon',
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
      async post(data: Record<string, unknown>) {
        try {
          const response = await context.store._apiProvider.post(context.store._baseUrl, data);
          context.store[context.store.$id] = response.data;
        } catch (error) {
          console.error(error);
        }
      },
    };
  }

  return undefined;
};

export default PostStorePlugin;
