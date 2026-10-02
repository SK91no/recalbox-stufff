/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { httpClient } from 'boot/axios';
import { AxiosInstance } from 'axios';

const HttpClientProviderStorePlugin = (): {_httpClientProvider: AxiosInstance} => ({
  _httpClientProvider: httpClient,
});

export default HttpClientProviderStorePlugin;
