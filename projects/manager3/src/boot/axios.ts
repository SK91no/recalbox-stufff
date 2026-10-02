/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { boot } from 'quasar/wrappers';
import axios, { AxiosInstance } from 'axios';
import { Loading, Notify } from 'quasar';
import {
  BIOS,
  MEDIA,
  SYSTEM,
} from 'src/router/api.routes';
import { i18n } from 'boot/i18n';

declare module '@vue/runtime-core' {
  interface ComponentCustomProperties {
    $axios: AxiosInstance;
  }
}

let clientApiUrl = process.env.API_URL;
let clientApi80Url = process.env.API_URL80;

if (process.env.API_URL === '/api') {
  clientApiUrl = `${window.location.protocol}//${window.location.host}:81${process.env.API_URL}`;
  clientApi80Url = `${window.location.protocol}//${window.location.host}${process.env.API_URL}`;
}

const apiUrl = clientApiUrl;

// Be careful when using SSR for cross-request state pollution
// due to creating a Singleton instance here;
// If any client changes this (global) instance, it might be a
// good idea to move this instance creation inside of the
// "export default () => {}" function below (which runs individually
// for each client)
const api: AxiosInstance = axios.create({ baseURL: clientApiUrl });
const api80: AxiosInstance = axios.create({ baseURL: clientApi80Url });
const httpClient: AxiosInstance = axios.create();

export default boot(({ app, router }): void => {
  // for use inside Vue files (Options API) through this.$axios and this.$api

  app.config.globalProperties.$axios = axios;
  // ^ ^ ^ this will allow you to use this.$axios (for Vue Options API form)
  //       so you won't necessarily have to import axios in each vue file

  app.config.globalProperties.$api = api;
  // ^ ^ ^ this will allow you to use this.$api (for Vue Options API form)
  //       so you can easily perform requests against your app's API

  app.config.globalProperties.$api80 = api80;
  // ^ ^ ^ this will allow you to use this.$api80 (for Vue Options API form)
  //       so you can easily perform requests against your app's API

  api.interceptors.request.use((config) => {
    // change some state in your store here
    // Do something before request is sent
    if (config.url === BIOS.all) {
      Loading.show({
        message: i18n.global.t('emulation.bios.loader'),
      });
    } else if (
      config.url?.includes('/region')
      || config.url?.includes('/metadata/info')
    ) {
      // allow disabling loader on requesting home region and metadata
    } else {
      Loading.show();
    }

    return config;
  }, (error) => {
    // Do something with request error
    Notify.create({
      message: error,
      type: 'negative',
      icon: 'mdi-alert-outline',
    });

    Loading.hide();
    return Promise.reject(error);
  });

  api80.interceptors.request.use((config) => {
    // change some state in your store here
    // Do something before request is sent
    Loading.show();

    return config;
  }, (error) => {
    // Do something with request error
    Notify.create({
      message: error,
      type: 'negative',
      icon: 'mdi-alert-outline',
    });

    Loading.hide();
    return Promise.reject(error);
  });

  // Add Notify Toasters on current axios requests
  api.interceptors.response.use((response) => {
    let message = i18n.global.t('general.notify.updateSuccess');
    let icon = 'mdi-check-bold';
    let actions;
    Loading.hide();

    if (response.config.url === MEDIA.takeScreenshot) {
      message = i18n.global.t('general.notify.screenshots.screenshotTaken');
      icon = 'mdi-folder-multiple-image';
      actions = [
        {
          label: i18n.global.t('general.notify.screenshots.view'),
          color: 'white',
          handler: () => router.push(
            { name: 'screenshots' },
          ),
        },
      ];
    }

    if (response.config.method === 'delete' && response.config.url?.includes(MEDIA.delete)) {
      message = i18n.global.t('general.notify.mediaDeleted');
      icon = 'mdi-image-remove';
    }

    if (response.config.method === 'post' || response.config.method === 'delete') {
      Notify.create({
        message,
        type: 'positive',
        icon,
        actions,
      });
    }

    return response;
  }, (error) => {
    const { code } = error;

    Loading.hide();

    Notify.create({
      message: i18n.global.t(`general.notify.${code}`),
      type: 'negative',
      icon: 'mdi-check-bold',
    });

    return Promise.reject(error);
  });

  // Add Notify Toasters on current axios requests
  api80.interceptors.response.use((response) => {
    let message = i18n.global.t('general.notify.updateSuccess');
    let icon = 'mdi-check-bold';
    let timeout = 5000;
    let closeBtn = false;
    let html = false;
    let actions;

    Loading.hide();

    if (response.config.url === SYSTEM.es.start) {
      message = i18n.global.t('general.notify.esStart');
      icon = 'mdi-play';
    }
    if (response.config.url === SYSTEM.es.stop) {
      message = i18n.global.t('general.notify.esShutdown');
      icon = 'mdi-stop';
    }
    if (response.config.url === SYSTEM.es.restart) {
      message = i18n.global.t('general.notify.esReboot');
      icon = 'mdi-restart';
    }
    if (response.config.url === SYSTEM.supportArchive) {
      message = i18n.global.t('general.notify.support.label');
      icon = 'mdi-check-bold';
      timeout = 0;
      closeBtn = false;
      html = true;
      actions = [
        {
          label: i18n.global.t('general.notify.support.archiveDownload'),
          color: 'white',
          handler: () => window.open(clientApi80Url + response.data.supportArchive),
        },
        {
          icon: 'close',
          color: 'white',
          round: true,
          handler: () => { /* ... */ },
        },
      ];
    }
    if (response.config.url === SYSTEM.resetFactory) {
      message = i18n.global.t('general.notify.support.resetFactory');
      icon = 'mdi-history';
    }

    if (response.config.method === 'get' || response.config.method === 'post') {
      Notify.create({
        message,
        type: 'positive',
        icon,
        timeout,
        closeBtn,
        html,
        actions,
      });
    }

    return response;
  }, (error) => {
    Loading.hide();

    Notify.create({
      message: i18n.global.t(`general.notify.${error.response.status}`),
      type: 'negative',
      icon: 'mdi-check-bold',
    });

    return Promise.reject(error);
  });
});

export {
  api,
  api80,
  apiUrl,
  httpClient,
};
