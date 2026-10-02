<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <transition
    appear
    enter-active-class="animated flipInX"
  >
    <div class="col q-pa-md statContainer total">
      <div class="badge">
        <div
          :class="[
            'circle-container',
            route ? 'hover' : ''
          ]"
          @click="click"
        >
          <div class="circle">
            <div class="value">{{ value }}</div>
            <div class="title">
              {{ t(title) }}
            </div>
          </div>
        </div>
      </div>
    </div>
  </transition>
</template>

<script lang="ts" setup>
import { toRefs } from 'vue';
import { useI18n } from 'vue-i18n';
import { useRouter } from 'vue-router';

const { t } = useI18n({ useScope: 'global' });
const router = useRouter();

const props = defineProps({
  title: { type: String, default: '' },
  value: { type: Number, default: 0 },
  route: { type: String },
});

const {
  title, value, route,
} = toRefs(props);

const click = () => {
  if (route?.value) {
    router.push(
      { name: route.value },
    );
  }
};
</script>

<style lang="sass" scoped>
body.body--dark
  .statContainer
    .badge
      .circle-container
        background-color: $dark-grey-strong

        .value,
        .title
          color: white
          text-shadow: -1px 0 $dark-grey-stronger, 0 1px $dark-grey-stronger, 1px 0 $dark-grey-stronger, 0 -1px $dark-grey-stronger

    .hover
      &:hover
        cursor: pointer
        background-color: $dark-grey-medium

.statContainer
  display: flex
  justify-content: center
  align-items: center

  .badge
    .circle-container
      transition: background-color 0.2s ease, transform 0.2s ease
      background-color: $light-blue
      height: 120px
      width: 120px
      border-radius: 50%

      .circle
        height: 100%
        width: 100%
        background-image: url(../../assets/controller-background.png)
        display: flex
        flex-direction: column
        justify-content: center
        align-items: center
        border-radius: 50%

        .value,
        .title
          cursor: default
          color: white
          text-shadow: -1px 0 $secondary, 0 1px $secondary, 1px 0 $secondary, 0 -1px $secondary

        .title
          text-transform: uppercase

        .value
          font-size: 3em

    .hover
      &:hover
        cursor: pointer
        background-color: $accent

        .value,
        .title
          cursor: pointer
</style>
