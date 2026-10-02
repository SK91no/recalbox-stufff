<template>
  <div class="header">
    <div class="title">
      <q-icon name="icon-emustation" size="md"/>
      {{ t('home.game.title') }}
    </div>
    <div class="controls">
      <q-btn
        v-if="currentState.currentRom?.metaData"
        flat
        rounded
        square
        icon="mdi-information"
        @click="() => infoOpen = true"
        :color="Dark.isActive ? 'dark-grey-light' : ''"
      >
        <q-tooltip
          class="bg-primary"
          :offset="[10, 10]"
          content-class="bg-primary"
          content-style="font-size: 16px"
        >
          {{ t('home.game.metaData.iconLabel') }}
        </q-tooltip>
      </q-btn>
<!--      <q-btn-->
<!--        v-if="currentState.currentRom"-->
<!--        flat-->
<!--        rounded-->
<!--        square-->
<!--        icon="mdi-stop"-->
<!--        text-color="negative"-->
<!--        disable-->
<!--      >-->
<!--        <q-tooltip-->
<!--          class="bg-primary"-->
<!--          :offset="[10, 10]"-->
<!--          content-class="bg-primary"-->
<!--          content-style="font-size: 16px"-->
<!--        >-->
<!--          {{ t('home.game.stop') }}-->
<!--        </q-tooltip>-->
<!--      </q-btn>-->
    </div>
  </div>
  <div class="informations">
    <OverlayMessage
      background
      v-if="currentState.currentAction === Actions.sleep
      || currentState.currentAction === Actions.runDemo
      || currentState.currentAction === Actions.startGameClip"
    >
      <template v-slot:content>
        <AnimatedSleepEmote />
      </template>
    </OverlayMessage>
    <OverlayMessage
      class="sleep-mode"
      v-else-if="currentState.currentRom === null"
    >
      <template v-slot:content>
        <q-icon name="mdi-ghost-off-outline"/>
      </template>
    </OverlayMessage>
    <div v-if="currentState.currentRom !== null" class="screen">
      <transition
        appear
        enter-active-class="animated slideInUp"
      >
        <q-img
          v-if="currentState.currentRom?.metaData?.availableMedia.hasImage"
          :src="currentState.currentRom?.imagePath"
          spinner-color="white"
          fit="contain"
        />
      </transition>
      <div
        class="image-placeholder"
        v-if="!currentState.currentRom?.metaData?.availableMedia.hasImage"
      >
        <transition
          appear
          enter-active-class="animated slideInUp"
        >
          <q-img
            v-if="currentState.currentSystem"
            class="console"
            :src="currentState.currentSystem?.consolePath"
            spinner-color="white"
          />
        </transition>
        <transition
          appear
          enter-active-class="animated slideInRight"
        >
          <div class="title">{{ currentState.currentRom?.metaData?.name }}</div>
        </transition>
      </div>
    </div>
  </div>
  <q-dialog
    v-if="currentState.currentRom?.metaData"
    transition-hide="slide-down"
    transition-show="slide-up"
    v-model="infoOpen"
  >
    <q-card class="bg-primary text-white info-dialog-card background info-dialog">
      <q-card-section class="text-justify" style="white-space: pre-line;">
        <div class="text-uppercase text-bold">{{ currentState.currentRom?.metaData?.name }}</div>
        <hr />
        <div>{{ currentState.currentRom?.metaData?.synopsys }}</div>
        <hr />
        <table>
          <tbody>
            <tr
              v-if="currentState.currentRom?.metaData?.publisher"
            >
              <td>{{ t('home.game.metaData.modal.publisher') }}</td>
              <td class="value">{{ currentState.currentRom?.metaData?.publisher }}</td>
            </tr>
            <tr
              v-if="currentState.currentRom?.metaData?.developer"
            >
              <td>{{ t('home.game.metaData.modal.developer') }}</td>
              <td class="value">{{ currentState.currentRom?.metaData?.developer }}</td>
            </tr>
            <tr
              v-if="currentState.currentRom?.metaData?.releaseDate"
            >
              <td>{{ t('home.game.metaData.modal.releaseDate') }}</td>
              <td class="value">{{ new Date(currentState.currentRom?.metaData?.releaseDate * 1000).toLocaleDateString(locale) }}</td>
            </tr>
            <tr
              v-if="currentState.currentRom?.metaData?.rating"
            >
              <td>{{ t('home.game.metaData.modal.rating') }}</td>
              <td class="value">
                <q-rating
                  :model-value="currentState.currentRom?.metaData?.rating * 5"
                  class=“no-shadow”
                  color="accent"
                  readonly
                  size="1em"
                  icon="star"
                  icon-half="star_half"
                />
              </td>
            </tr>
            <tr>
              <td>{{ t('home.game.metaData.modal.players') }}</td>
              <td class="value">
                {{
                  currentState.currentRom?.metaData?.players.min === currentState.currentRom?.metaData?.players.max
                    ? currentState.currentRom?.metaData?.players.min
                    : `${currentState.currentRom?.metaData?.players.min} - ${currentState.currentRom?.metaData?.players.max}`
                }}
              </td>
            </tr>
            <tr>
              <td>{{ t('home.game.metaData.modal.genres') }}</td>
              <td class="value">{{ currentState.currentRom?.metaData?.genres.free }}</td>
            </tr>
          </tbody>
        </table>
      </q-card-section>
    </q-card>
  </q-dialog>
</template>

<script lang="ts" setup>
import AnimatedSleepEmote from 'components/ui-kit/AnimatedSleepEmote.vue';
import OverlayMessage from 'components/ui-kit/OverlayMessage.vue';
import { storeToRefs } from 'pinia';
import { useEmulationstationStore } from 'stores/configuration/emulationstation';
import { Actions } from 'stores/common/monitoring.d';
import { ref } from 'vue';
import { useI18n } from 'vue-i18n';
import { Dark } from 'quasar';

const { locale, t } = useI18n({ useScope: 'global' });

const emulationStationStore = useEmulationstationStore();
const { currentState } = storeToRefs(emulationStationStore);

const infoOpen = ref<boolean>(false);
</script>

<style lang="sass" scoped>
body.body--dark
  .header
    border-bottom: 2px solid $dark-grey-strong

    .title
      color: $dark-grey-light

      .q-icon
        color: $dark-grey-light

  .sleep-mode
    .q-icon
      color: $dark-grey-strong

.value
  padding-left: 2em

.header
  display: flex
  justify-content: space-between
  gap: 1em
  margin-bottom: 1em
  padding: .2em
  border-bottom: 2px solid white

  .title
    display: flex
    align-items: center
    text-transform: uppercase
    font-weight: 400
    color: $accent
    min-height: 36px
    margin-left: 1em

    .q-icon
      margin-right: .2em

  .controls
    z-index: 2

.informations
  position: relative
  display: flex
  justify-content: center
  padding: 0 1em
  height: 100%
  align-items: center
  z-index: 1

  .screen
    position: relative
    width: 50%
    filter: drop-shadow(0 0 0.75rem rgba(0, 0, 0, 0.29))

.info-dialog-card
  border-left: 6px solid $accent

  &:before
    content: "\F02FC"

@keyframes helpDialogSlidein
  from
    right: -10rem
    bottom: -10rem

  to
    right: -30px
    bottom: 0

.info-dialog
  overflow: hidden!important
  &:before
    font-size: 14em
    bottom: 0
    right: -30px
    color: $secondary
    opacity: .3
    animation-duration: .5s
    animation-name: helpDialogSlidein

.q-icon
  color: $light-blue
  font-size: 9em
</style>
