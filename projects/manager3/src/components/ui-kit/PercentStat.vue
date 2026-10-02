<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <transition
    appear
    enter-active-class="animated flipInX"
  >
    <div class="col q-pa-md statContainer percent">
      <div class="badge">
        <q-knob
          :thickness="1"
          class="percent"
          :color="Dark.isActive ? 'dark-grey-medium' : 'secondary'"
          font-size="1rem"
          show-value
          size="120px"
          :track-color="Dark.isActive ? 'dark-grey-light' : 'light-blue'"
          v-model="percent"
          readonly
        />
        <div class="title">
          {{ t(title) }}
        </div>
      </div>
    </div>
  </transition>
</template>

<script lang="ts" setup>
import { toRefs } from 'vue';
import { useI18n } from 'vue-i18n';
import { Dark } from 'quasar';

const { t } = useI18n({ useScope: 'global' });

const props = defineProps({
  title: { type: String, default: '' },
  percent: { type: Number, default: 0 },
});

const {
  title, percent,
} = toRefs(props);
</script>

<style lang="sass" scoped>
body.body--dark
  .statContainer
    .badge
      .percent
        color: white
        text-shadow: -1px 0 $dark-grey-stronger, 0 1px $dark-grey-stronger, 1px 0 $dark-grey-stronger, 0 -1px $dark-grey-stronger

      .title
        color: white
        text-shadow: -1px 0 $dark-grey-stronger, 0 1px $dark-grey-stronger, 1px 0 $dark-grey-stronger, 0 -1px $dark-grey-stronger

.statContainer
  display: flex
  justify-content: center
  align-items: center

  .badge
    display: flex
    flex-direction: column
    justify-content: center
    align-items: center
    position: relative

    .percent
      color: white
      text-shadow: -1px 0 $secondary, 0 1px $secondary, 1px 0 $secondary, 0 -1px $secondary

    .title
      text-align: center
      text-transform: uppercase
      color: white
      text-shadow: -1px 0 $secondary, 0 1px $secondary, 1px 0 $secondary, 0 -1px $secondary
      position: absolute
      top: 5rem
      left: 0
      right: 0
</style>
