/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { defineStore } from 'pinia';
import { MEDIA } from 'src/router/api.routes';
import { date } from 'quasar';
import { MediasResponse, Screenshot, MediaType } from 'stores/common/media.d';
import { apiUrl } from 'boot/axios';

export interface MediaStoreState {
  _baseUrl: string;
  media: MediasResponse;
}

export const useMediaStore = defineStore('media', {
  state: () => ({
    _baseUrl: MEDIA.all,
    media: {
      mediaList: {},
    },
  } as MediaStoreState),

  getters: {
    screenshots: (state) => {
      const result: Screenshot[] = [];

      Object.keys(state.media.mediaList).forEach((key): void => {
        // The screenshots directory have some text file, don't parse them
        if (key.includes('.png') || key.includes('.gif') || key.includes('.jpg') || key.includes('.svg')) {
          let formattedDate: string;
          let sortDate: string; // To sort screenshots by date starting by the year
          if (key.includes('screenshot-')) {
            // Check screenshots created via current manager
            const name = key.substring(11).substring(0, 24);
            formattedDate = date.formatDate(
              date.extractDate(name, 'YYYY-MM-DDTHH-mm-ss'), // "2023-04-03T07-51-41-443Z"
              'DD/MM/YYYY - HH:mm:ss',
            );
            sortDate = date.formatDate(
              date.extractDate(name, 'YYYY-MM-DDTHH-mm-ss'),
              'YYYYMMDDHHmmss',
            );
          } else {
            // Check screenshots created in-game (HK + L1)
            const name = key.substring(key.length - 17).substring(0, key.length);
            formattedDate = date.formatDate(
              date.extractDate(name, 'YYMMDD-HHmmss'), // "230403-075141"
              'DD/MM/YYYY - HH:mm:ss',
            );
            sortDate = date.formatDate(
              date.extractDate(name, 'YYMMDD-HHmmss'),
              'YYYYMMDDHHmmss',
            );
          }
          result.push({
            name: key,
            path: apiUrl + MEDIA.get + key,
            date: formattedDate,
            sort: sortDate,
            type: MediaType.image,
          });
        }

        // Do the videos
        if (key.includes('.mkv') || key.includes('.mp4') || key.includes('.avi') || key.includes('.webm')) {
          const name = key.substring(key.length - 17).substring(0, key.length);
          const formattedDate = date.formatDate(
            date.extractDate(name, 'YYMMDD-HHmmss'), // "230403-075141"
            'DD/MM/YYYY - HH:mm:ss',
          );
          const sortDate = date.formatDate(
            date.extractDate(name, 'YYMMDD-HHmmss'),
            'YYYYMMDDHHmmss',
          );
          let mediaType = MediaType.mp4;
          if (key.includes('.webm')) {
            mediaType = MediaType.webm;
          } else if (key.includes('.avi')) {
            mediaType = MediaType.xMsvideo;
          }
          result.push({
            name: key,
            path: apiUrl + MEDIA.get + key,
            date: formattedDate,
            sort: sortDate,
            type: mediaType,
          });
        }
      });

      return result;
    },
  },

  actions: {
    async takeScreenshot() {
      try {
        const response = await this._apiProvider.post(MEDIA.takeScreenshot);
        this.media = response.data;
      } catch (error) {
        console.error(error);
      }
    },
    async delete(screenshotName: string) {
      try {
        const response = await this._apiProvider.delete(MEDIA.delete + screenshotName);
        this.media = response.data;
      } catch (error) {
        console.error(error);
      }
    },
  },
});
