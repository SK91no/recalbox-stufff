<template>
  <div class="consoleContainer">
    <q-img
      v-if="currentState.currentSystem"
      class="console"
      :src="currentState.currentSystem?.consolePath"
      spinner-color="white"
    />
  </div>
  <div class="header">
    <div class="title">
      <q-icon name="icon-emustation" size="md"/>
      {{ t('home.system.title') }}
    </div>
    <div class="controls">
      <q-btn flat rounded square icon="mdi-restart" text-color="warning" @click="serverStore.esReboot()">
        <q-tooltip
          class="bg-primary"
          :offset="[10, 10]"
          content-class="bg-primary"
          content-style="font-size: 16px"
        >
          {{ t('home.system.es.restart') }}
        </q-tooltip>
      </q-btn>
<!--      <q-btn-->
<!--        flat-->
<!--        rounded-->
<!--        square-->
<!--        icon="mdi-play"-->
<!--        :text-color="!serverStore.available ? 'positive' : 'secondary'"-->
<!--        @click="serverStore.esStart()"-->
<!--        :disable="serverStore.available"-->
<!--      >-->
<!--        <q-tooltip-->
<!--          class="bg-primary"-->
<!--          :offset="[10, 10]"-->
<!--          content-class="bg-primary"-->
<!--          content-style="font-size: 16px"-->
<!--        >-->
<!--          {{ t('home.system.es.start') }}-->
<!--        </q-tooltip>-->
<!--      </q-btn>-->
<!--      <q-btn-->
<!--        flat-->
<!--        rounded-->
<!--        square-->
<!--        icon="mdi-stop"-->
<!--        :text-color="serverStore.available ? 'negative' : 'secondary'"-->
<!--        @click="serverStore.esShutdown()"-->
<!--        :disable="!serverStore.available"-->
<!--      >-->
<!--        <q-tooltip-->
<!--          class="bg-primary"-->
<!--          :offset="[10, 10]"-->
<!--          content-class="bg-primary"-->
<!--          content-style="font-size: 16px"-->
<!--        >-->
<!--          {{ t('home.system.es.stop') }}-->
<!--        </q-tooltip>-->
<!--      </q-btn>-->
    </div>
  </div>
  <div class="informations">
    <AnimatedLines
      v-if="currentState.currentSystem?.metaData.colors[emulationstation['theme.region'].value]
      || currentState.currentSystem?.metaData.colors.eu"
      :colors="currentState.currentSystem?.metaData.colors[emulationstation['theme.region'].value]
      ?? currentState.currentSystem?.metaData.colors.eu"
    />
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
      class="server-off"
      v-else-if="currentState.currentSystem === null"
    >
      <template v-slot:content>
        <q-icon name="mdi-server-off"/>
      </template>
    </OverlayMessage>
    <q-img
      v-if="currentState.currentSystem"
      class="logo"
      :src="currentState.currentSystem?.logoPath"
      spinner-color="white"
      @click="redirect"
      fit="contain"
    />
    <transition
      appear
      enter-active-class="animated flipInX"
      v-if="currentState.currentSystem"
    >
      <q-list class="meta-list">
        <q-item v-if="currentState.currentSystem.metaData.system.manufacturer">
          <q-item-section top avatar>
            <q-avatar
              :color="Dark.isActive ? 'dark-grey-strong' : 'white'"
              square
              rounded
              icon="mdi-factory"
            />
          </q-item-section>
          <q-item-section>
            <q-item-label>{{ t('home.system.manufacturer') }}</q-item-label>
            <q-item-label caption>{{currentState.currentSystem.metaData.system.manufacturer}}</q-item-label>
          </q-item-section>
        </q-item>
        <q-item v-if="currentState.currentSystem.metaData.system.yearOfRelease">
          <q-item-section top avatar>
            <q-avatar
              :color="Dark.isActive ? 'dark-grey-strong' : 'white'"
              square
              rounded
              icon="mdi-calendar-month-outline"
            />
          </q-item-section>
          <q-item-section>
            <q-item-label>{{ t('home.system.yearOfRelease') }}</q-item-label>
            <q-item-label caption>{{currentState.currentSystem.metaData.system.yearOfRelease}}</q-item-label>
          </q-item-section>
        </q-item>
        <q-item v-if="currentState.currentSystem.metaData.system.cpu">
          <q-item-section top avatar>
            <q-avatar
              :color="Dark.isActive ? 'dark-grey-strong' : 'white'"
              square
              rounded
              icon="mdi-cpu-32-bit"
            />
          </q-item-section>
          <q-item-section>
            <q-item-label>{{ t('home.system.cpu') }}</q-item-label>
            <q-item-label caption>{{currentState.currentSystem.metaData.system.cpu}}</q-item-label>
          </q-item-section>
        </q-item>
        <q-item v-if="currentState.currentSystem.metaData.system.ram">
          <q-item-section top avatar>
            <q-avatar
              :color="Dark.isActive ? 'dark-grey-strong' : 'white'"
              square
              rounded
              icon="mdi-memory"
            />
          </q-item-section>
          <q-item-section>
            <q-item-label>{{ t('home.system.ram') }}</q-item-label>
            <q-item-label caption>{{currentState.currentSystem.metaData.system.ram}}</q-item-label>
          </q-item-section>
        </q-item>
        <q-item v-if="currentState.currentSystem.metaData.system.gpu">
          <q-item-section top avatar>
            <q-avatar
              :color="Dark.isActive ? 'dark-grey-strong' : 'white'"
              square
              rounded
              icon="mdi-expansion-card-variant"
            />
          </q-item-section>
          <q-item-section>
            <q-item-label>{{ t('home.system.gpu') }}</q-item-label>
            <q-item-label caption>{{currentState.currentSystem.metaData.system.gpu}}</q-item-label>
          </q-item-section>
        </q-item>
        <q-item v-if="currentState.currentSystem.metaData.system.soundChip">
          <q-item-section top avatar>
            <q-avatar
              :color="Dark.isActive ? 'dark-grey-strong' : 'white'"
              square
              rounded
              icon="mdi-toslink"
            />
          </q-item-section>
          <q-item-section>
            <q-item-label>{{ t('home.system.soundChip') }}</q-item-label>
            <q-item-label caption>{{currentState.currentSystem.metaData.system.soundChip}}</q-item-label>
          </q-item-section>
        </q-item>
      </q-list>
    </transition>
  </div>
</template>

<script lang="ts" setup>
import AnimatedLines from 'components/ui-kit/AnimatedLines.vue';
import AnimatedSleepEmote from 'components/ui-kit/AnimatedSleepEmote.vue';
import OverlayMessage from 'components/ui-kit/OverlayMessage.vue';
import { storeToRefs } from 'pinia';
import { useEmulationstationStore } from 'stores/configuration/emulationstation';
import { useServerStore } from 'stores/common/server';
import { Actions } from 'stores/common/monitoring.d';
import { useI18n } from 'vue-i18n';
import { useRouter } from 'vue-router';
import { Dark } from 'quasar';

const { t } = useI18n({ useScope: 'global' });

const router = useRouter();

const serverStore = useServerStore();
const emulationStationStore = useEmulationstationStore();

const { currentState, emulationstation } = storeToRefs(emulationStationStore);

serverStore.available = true;
const redirect = () => {
  if (currentState.value.currentSystem?.systemId === 'imageviewer') {
    router.push(
      { name: 'screenshots' },
    );
  } else {
    router.push(
      { name: 'systems-system', params: { system: currentState.value.currentSystem?.systemId } },
    );
  }
};
</script>

<style lang="sass" scoped>
body.body--dark
  .header
    border-bottom: 2px solid $dark-grey-strong

    .title
      color: $dark-grey-light

      .q-icon
        color: $dark-grey-light

  .server-off
    .q-icon
      color: $dark-grey-strong

  .consoleContainer
    .console
      opacity: .2

.consoleContainer
  position: absolute
  bottom: 1em
  width: 50%
  max-height: 40%
  transform: translate(50%)

  .console
    filter: saturate(0)
    opacity: .1

.header
  display: flex
  justify-content: space-between
  gap: 1em
  padding: .2em
  border-bottom: 2px solid white

  .title
    display: flex
    align-items: center
    text-transform: uppercase
    font-weight: 400
    color: $accent
    margin-left: 1em

    .q-icon
      margin-right: .2em

  .controls
    display: flex
    justify-content: flex-end
    gap: .5em

.informations
  position: relative
  display: flex
  flex-direction: column
  padding: 1em
  height: 100%
  overflow: hidden

  .meta-list
    border-color: white
    border-radius: 5px

  .logo
    z-index: 100
    cursor: pointer
    width: 50%
    margin: 0 auto 2em auto
    filter: saturate(50%)
    opacity: .4
    transition: opacity .3s
    height: 80px

    &:hover
      filter: initial
      opacity: 1

.q-icon
  color: $light-blue
  font-size: 9em

@media(max-width: 700px)
  .consoleContainer
    display: none
</style>
