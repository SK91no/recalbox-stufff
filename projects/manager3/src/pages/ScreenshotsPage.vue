<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-page class="background screenshots">
    <div class="row">
      <div class="col col-xs-12 col-sm-12 col-md-12 table-container">
        <q-table
          :grid="displayGrid"
          :rows="screenshots"
          :columns="columns"
          row-key="name"
          hide-pagination
          :pagination="initialPagination"
        >
          <template v-slot:item="screenshot">
            <div
              class="q-pa-xs col-xs-12 col-sm-6 col-md-4 col-lg-3"
              v-if="screenshot.row.type === MediaType.image"
            >
              <q-card @click="open(MediaType.image, screenshot.row.path)" class="screenshot" flat rounded>
                <q-card-section horizontal>
                  <q-img :src="screenshot.row.path" class="col" loading="lazy">
                    <div
                      class="absolute-bottom text-white justify-between row items-start"
                      style="padding: .5em"
                    >
                      <span class="self-center">
                        <q-icon
                          name="mdi-calendar-clock"
                          style="font-size: 1.5em; margin-right: .25em;"
                        />{{ screenshot.row.date }}
                      </span>
                      <div>
                        <q-btn
                          class="float-right"
                          color="negative"
                          flat icon="mdi-delete"
                          round
                          @click.stop="openDeleteConfirm(screenshot.row.name)"
                        />
                        <q-btn
                          class="float-right"
                          color="white"
                          flat icon="mdi-download"
                          round
                          @click.stop="() => openURL(screenshot.row.path)"
                        />
                      </div>
                    </div>
                  </q-img>
                </q-card-section>
              </q-card>
            </div>
            <div
              class="q-pa-xs col-xs-12 col-sm-6 col-md-4 col-lg-3"
              v-if="screenshot.row.type === MediaType.mp4
              || screenshot.row.type === MediaType.xMsvideo
              || screenshot.row.type === MediaType.webm"
            >
              <q-card @click="open(screenshot.row.type, screenshot.row.path)" class="screenshot" flat rounded>
                <q-card-section horizontal>
                  <video class="col" autoplay loop muted playsinline preload="metadata">
                    <source :src="screenshot.row.path" type="video/mp4" />
                  </video>
                  <div
                    class="absolute-bottom text-white justify-between row items-start"
                    style="padding: .5em"
                  >
                    <span class="self-center">
                      <q-icon
                        name="mdi-calendar-clock"
                        style="font-size: 1.5em; margin-right: .25em;"
                      />{{ screenshot.row.date }}
                    </span>
                    <div>
                      <q-btn
                        class="float-right"
                        color="negative"
                        flat icon="mdi-delete"
                        round
                        @click.stop="openDeleteConfirm(screenshot.row.name)"
                      />
                      <q-btn
                        class="float-right"
                        color="white"
                        flat icon="mdi-download"
                        round
                        @click.stop="() => openURL(screenshot.row.path)"
                      />
                    </div>
                  </div>
                </q-card-section>
              </q-card>
            </div>
          </template>
        </q-table>
      </div>

      <q-dialog v-model="openImage" full-height full-width>
        <q-card>
          <q-card-section horizontal>
            <q-img :src="mediaPath" @click="openImage = false" class="opened"/>
          </q-card-section>
        </q-card>
      </q-dialog>

      <q-dialog v-model="openVideo" full-height full-width>
        <q-card>
          <q-card-section horizontal style="height: 100%">
            <video
              controls
              style="height: 100%; margin: 0 auto"
              @click="openVideo = false"
              class="opened"
              autoplay
              loop
              muted
              playsinline
              preload="metadata"
            >
              <source :src="mediaPath" :type="videoType" />
            </video>
          </q-card-section>
        </q-card>
      </q-dialog>
    </div>
  </q-page>
</template>

<script lang="ts" setup>
import { ref } from 'vue';
import { useMediaStore } from 'stores/common/media';
import { storeToRefs } from 'pinia';
import { useI18n } from 'vue-i18n';
import { openURL, useQuasar } from 'quasar';
import { MediaType } from 'stores/common/media.d';

const { t } = useI18n({ useScope: 'global' });

const $q = useQuasar();

const mediaStore = useMediaStore();
mediaStore.fetch();
const { screenshots } = storeToRefs(mediaStore);

const openImage = ref(false);
const openVideo = ref(false);
const mediaPath = ref('');
const videoType = ref('');

const displayGrid = ref(true);

const open = (type: string, url: string) => {
  if (type === MediaType.image) {
    mediaPath.value = url;
    openImage.value = true;
  } else if (type === MediaType.mp4 || type === MediaType.xMsvideo || type === MediaType.webm) {
    mediaPath.value = url;
    openVideo.value = true;
    videoType.value = type;
  }
};

const openDeleteConfirm = (name: string) => {
  $q.dialog({
    class: 'delete-dialog-card',
    message: t('emulation.screenshots.dialogs.remove.text'),
    cancel: true,
    persistent: true,
    transitionHide: 'flip-up',
    transitionShow: 'flip-down',
  }).onOk(() => {
    mediaStore.delete(name);
  });
};

const columns = [
  {
    name: 'sort',
    label: 'sort',
    field: 'sort',
    sortable: true,
  },
];

const initialPagination = {
  sortBy: 'sort',
  descending: false,
  rowsPerPage: 9999,
};
</script>

<style lang="sass">
body.body--dark
  .screenshots
    .screenshot
      .absolute-bottom
        background: rgba($dark-grey-strong, .47) !important

.screenshots
  padding: 0 8px 100px 8px

  &:before
    content: "\F0104"

  .screenshot
    overflow: hidden
    cursor: pointer

    .q-img__image
      transition: transform .2s
      -webkit-transition: -webkit-transform .2s

    &:hover
      .q-img__image
        transform: scale(1.1)
        -webkit-transform: scale(1.1)

    .absolute-bottom
      background: rgba($primary, .47) !important

.delete-dialog-card
  border-left: 6px solid $accent

@media(max-width: 500px)
  .screenshots
    padding: 0 8px
</style>
