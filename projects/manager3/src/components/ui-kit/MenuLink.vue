<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-item
    :to="{name: route}"
    clickable
    v-ripple
    :exact="exact"
  >
    <q-item-section
      avatar
      v-if="icon"
    >
      <q-icon :name="icon"/>
    </q-item-section>
    <q-item-section>
      {{ t(title) }}
    </q-item-section>
  </q-item>
</template>

<script lang="ts" setup>
import { toRefs } from 'vue';
import { useI18n } from 'vue-i18n';

const { t } = useI18n({ useScope: 'global' });

const props = defineProps({
  title: { type: String, default: '' },
  route: { type: String, default: 'home' },
  exact: { type: Boolean, default: false },
  icon: { type: String, default: '' },
});

const {
  title, route, exact, icon,
} = toRefs(props);
</script>

<style lang="sass">
body.body--dark
  .menu
    .q-link
      color: $dark-grey-light

      &:hover
        color: $accent

    .q-router-link--active
      color: $accent

      i
        color: $accent

    .q-router-link--active:after
      color: $dark-grey-light

    .q-separator--horizontal
      background: $dark-grey-medium

.menu
  //aside
  .q-link
    color: $secondary
    overflow-x: hidden

    &:hover
      color: $accent

  .q-router-link--active
    color: $accent

    i
      color: $accent

  .q-router-link--active:after
    content: '\f035e'
    display: block
    font-family: 'Material Design Icons'
    position: absolute
    right: -11px
    top: 7px
    color: $rc-light-grey
    font-size: x-large

  .menu-logo
    position: absolute
    bottom: 0
    left: 0
    right: 0
    text-align: center

    img
      width: 30px
      margin-bottom: 1em
      transition: transform .7s ease-in-out

      &:hover
        transform: rotate(360deg)

  .q-separator--horizontal
    height: 1px
    background: $secondary
    margin-bottom: 0
</style>
