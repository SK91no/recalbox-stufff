<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="background virtual-trackpad">
    <div class="trackpad" v-on:mousemove="updateCoordinates"/>
  </div>
</template>

<script lang="ts" setup>
import client from 'boot/mqtt';

let x = 0;
let y = 0;

const updateCoordinates = (event: { offsetX: number; offsetY: number; }) => {
  x = event.offsetX;
  y = event.offsetY;
  client.publish(String(process.env.MQTT_VD_EVENTS_TOUCHPAD_CHANNEL), `${x}, ${y}`);
};
</script>

<style lang="sass">
.virtual-trackpad
  height: 100vh
  display: flex
  align-items: center
  padding: 1em
  justify-content: center

  &:before
    content: "\F07F8"

  .trackpad
    background-color: rgba($black, .1)
    width: 90vw
    height: 90vh
    border-radius: 5px
    opacity: .5
</style>
