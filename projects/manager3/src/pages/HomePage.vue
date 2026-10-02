<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-page class="background home">
    <div class="header">
      <div class="background-container">
        <img v-if="!Dark.isActive" src="../assets/recalbox.svg" alt="Recalbox" />
        <img v-else src="../assets/recalbox-dark.svg" alt="Recalbox" />
      </div>
    </div>
    <div class="row stats">
      <div class="vertical-middle text-center preview self-center">
        {{ t('home.preview.title') }}
        <span class="arrow-right"><q-icon name="mdi-chevron-right"/></span>
        <span class="arrow-bottom"><q-icon name="mdi-chevron-down"/></span>
      </div>
      <q-separator vertical/>
      <q-separator/>
      <TotalStat
        v-bind="total"
        v-for="total in totals"
        :key="total.key"
      />
      <SystemStat
        v-bind="percent"
        v-for="percent in percents"
        :key="percent.key"
      />
    </div>

    <div class="row flex items-stretch emustation">
<!--      <div class="col social-medias">-->
<!--      </div>-->
      <div class="col system">
        <SystemView/>
      </div>
      <div class="col game">
        <GameView/>
      </div>
    </div>
  </q-page>
</template>

<script lang="ts" setup>
import { Dark } from 'quasar';
import { useMonitoringStore } from 'src/stores/common/monitoring';
import { HomeStat } from 'stores/types/misc';
import { computed } from 'vue';
import SystemStat from 'components/ui-kit/PercentStat.vue';
import TotalStat from 'components/ui-kit/TotalStat.vue';
import { useSystemsStore } from 'stores/emulation/systems';
import SystemView from 'components/home/SystemView.vue';
import GameView from 'components/home/GameView.vue';
import { useRomsStore } from 'stores/emulation/roms';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });

const { getSystemsListCount } = useSystemsStore();
useSystemsStore().fetch();

useRomsStore().getRomsCount();

const { getSharePercent } = useMonitoringStore();
useMonitoringStore().fetch();

const totals = computed<HomeStat[]>(() => [
  {
    key: 1,
    title: 'home.preview.systems',
    value: getSystemsListCount(),
    route: 'systems-parent',
  },
  {
    key: 2,
    title: 'home.preview.roms',
    value: useRomsStore().total,
  },
]);

const percents = computed<HomeStat[]>(() => [
  {
    key: 3,
    title: 'home.preview.sharePercent',
    value: getSharePercent(),
    percent: getSharePercent(),
  },
]);
</script>

<style lang="sass" scoped>
body.body--dark
  .stats
    background: $dark-grey-strong
    hr
      background: $dark-grey-medium
    .preview
      color: $dark-grey-light

  .home
    .header
      background-image: linear-gradient(to right, $dark-grey-strong, $dark-grey-light-medium)

    .system, .game, .system:after
      background: $dark-grey-medium

    .system
      border-right: 2px solid $dark-grey-strong

    .system:after
      border-top: 2px solid $dark-grey-strong
      border-right: 2px solid $dark-grey-strong

.home
  height: 100%

  &:before
    content: "\F056E"
    z-index: 1

  .header
    height: 5em
    background-image: linear-gradient(to right, $primary, $accent)
    -webkit-box-shadow: inset 0 0 15px 4px rgba($black, .29)
    box-shadow: inset 0 0 15px 4px rgba($black, .29)

    @keyframes animatedBackground
      from
        background-position: 0 0
      to
        background-position: 100% 0

    .background-container
      height: 100%
      background-image: url(../assets/controller-background.png)
      position: relative
      animation: animatedBackground 200s linear infinite alternate

      @keyframes logoslidein
        from
          right: -38rem
        to
          right: 1em

      img
        max-width: 310px
        position: absolute
        right: 1em
        bottom: 0
        animation-duration: .5s
        animation-name: logoslidein
        filter: drop-shadow(0 0 0.75rem rgba(0, 0, 0, 0.29))

  .stats
    background: white
    min-height: 212px
    position: relative

    .preview
      text-transform: uppercase
      font-weight: 100
      font-size: x-large
      color: $primary

    @media(max-width: 1023px)
      .preview
        width: 100%

      .arrow-right,
      .q-separator--vertical
        display: none

    @media(min-width: 1024px)
      .preview
        width: 15%

      .arrow-bottom,
      .q-separator--horizontal
        display: none

  .q-separator--vertical
    background: $rc-light-grey
    width: 2px
    min-width: 2px
    margin-bottom: 0

  .q-separator--horizontal
    background: $rc-light-grey
    width: 100%
    height: 2px

  .emustation
    height: calc(100vh - 212px - 5em)

    .social-medias,
    .system,
    .game
      position: relative
      display: flex
      flex-direction: column

    .system
      background: $rc-light-grey
      border-right: 2px solid white

      &:after
        display: block
        content: ''
        border-top: 2px solid white
        border-right: 2px solid white
        height: 50px
        width: 50px
        transform: rotate(45deg) translateY(-50%)
        position: absolute
        right: -8px
        background: $rc-light-grey
        margin-top: calc(50vh - 131px)
        z-index: 11

    .social-medias
      flex-basis: 20%
      background: white

    .system,
    .game
      flex-basis: 40%

    @media(max-width: 1400px)
      .social-medias
        display: none

@media(max-width: 950px)
  body.body--dark
    .home
      .emustation
        .system
          border-right: 0

        .game
          border-top: 2px solid $dark-grey-strong

  .home
    .emustation
      height: 100vh

      .system,
      .game
        flex-basis: 100%
        min-height: 250px

      .game
        border-top: 2px solid white

@media(max-width: 420px)
  .home
    .header
      .background-container
        img
          max-width: 200px

    .stats
      min-height: auto

      .q-separator--horizontal,
      .total,
      .percent
        display: none

    .emustation
      height: initial

      .system
        &:after
          display: none

        .header
          flex-direction: column

          .controls
            justify-content: space-between
</style>
