/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { api } from 'boot/axios';
import { AxiosInstance } from 'axios';

const ApiProviderStorePlugin = (): {_apiProvider: AxiosInstance} => ({
  _apiProvider: api,
});

export default ApiProviderStorePlugin;
