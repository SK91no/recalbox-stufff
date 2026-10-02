/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { store } from 'quasar/wrappers';
import { createPinia } from 'pinia';
import FetchStorePlugin from 'stores/plugins/fetchStorePlugin';
import FetchOptionsStorePlugin from 'stores/plugins/fetchOptionsStorePlugin';
import PostStorePlugin from 'stores/plugins/postStorePlugin';
import ApiProviderStorePlugin from 'stores/plugins/apiProviderStorePlugin';
import Api80ProviderStorePlugin from 'stores/plugins/api80ProviderStorePlugin';
import HttpClientProviderStorePlugin from 'stores/plugins/httpClientProviderStorePlugin';

/*
 * When adding new properties to stores, you should also
 * extend the `PiniaCustomProperties` interface.
 * @see https://pinia.vuejs.org/core-concepts/plugins.html#typing-new-store-properties
 */

/*
 * If not building with SSR mode, you can
 * directly export the Store instantiation;
 *
 * The function below can be async too; either use
 * async/await or return a Promise which resolves
 * with the Store instance.
 */

export default store((/* { ssrContext } */) => {
  const pinia = createPinia();

  pinia.use(ApiProviderStorePlugin);
  pinia.use(Api80ProviderStorePlugin);
  pinia.use(HttpClientProviderStorePlugin);
  pinia.use(FetchStorePlugin);
  pinia.use(FetchOptionsStorePlugin);
  pinia.use(PostStorePlugin);

  return pinia;
});
