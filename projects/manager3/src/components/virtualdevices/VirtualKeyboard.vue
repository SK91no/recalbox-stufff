<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="background virtual-keyboard">
    <div class="keyboard-container">
      <div class="simple-keyboard-main"></div>

      <div class="control-arrows">
        <div class="simple-keyboard-control"></div>
        <q-img class="logo" src="../../assets/logos_v2_recalbox-all-transparent.svg"/>
        <div class="simple-keyboard-arrows"></div>
      </div>

      <div class="right-controls">
        <q-select
          standout="bg-primary text-white"
          v-model="keyboardDisplay"
          :options="keyboardOptions"
          dense
          class="lang-selector"
          map-options
          emit-value
          option-value="value"
          option-label="label"
          @update:model-value="(value) => switchLayout(value)"
        />
        <div class="num-pad">
          <div class="simple-keyboard-numpad"></div>
          <div class="simple-keyboard-numpadEnd"></div>
        </div>
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { onMounted, ref } from 'vue';
import client from 'boot/mqtt';
import Keyboard from 'simple-keyboard';
import 'simple-keyboard/build/css/index.css';
import { keyboardLayouts } from 'src/utils/keyboardLayouts';

let keyboard: Keyboard | null = null;
// eslint-disable-next-line @typescript-eslint/no-unused-vars
let keyboardControlPad: Keyboard | null = null;
// eslint-disable-next-line @typescript-eslint/no-unused-vars
let keyboardArrows: Keyboard | null = null;
// eslint-disable-next-line @typescript-eslint/no-unused-vars
let keyboardNumPad: Keyboard | null = null;
let keyboardNumPadEnd: Keyboard | null = null;

const keyboardOptions = [
  { label: 'QWERTY (english)', value: 'qwerty' },
  { label: 'AZERTY (french)', value: 'azerty' },
];
const keyboardDisplay = ref(keyboardOptions[1]);

const handleShift = () => {
  if (keyboard) {
    const currentLayout = keyboard.options.layoutName;
    keyboard.setOptions({
      layoutName: currentLayout === 'default' ? 'shift' : 'default',
    });
  }
};

const onKeyPress = (button: string) => {
  client.publish(String(process.env.MQTT_VD_EVENTS_KEYBOARD_CHANNEL), `${button}, 1`);
  if (
    button === '{shift}'
    || button === '{shiftleft}'
    || button === '{shiftright}'
    || button === '{capslock}'
  ) { handleShift(); }
};

const onKeyReleased = (button: string) => {
  client.publish(String(process.env.MQTT_VD_EVENTS_KEYBOARD_CHANNEL), `${button}, 0`);
  if (
    button === '{shift}'
    || button === '{shiftleft}'
    || button === '{shiftright}'
  ) { handleShift(); }
};

const commonKeyboardOptions = {
  onKeyPress: (button: string) => onKeyPress(button),
  onKeyReleased: (button: string) => onKeyReleased(button),
  theme: 'simple-keyboard hg-theme-default hg-layout-default',
  physicalKeyboardHighlight: true,
  syncInstanceInputs: true,
  mergeDisplay: true,
  debug: false,
};

onMounted(() => {
  keyboard = new Keyboard('.simple-keyboard-main', {
    ...commonKeyboardOptions,
    ...keyboardLayouts[keyboardDisplay.value.value].keyboard,
  });
  keyboardControlPad = new Keyboard('.simple-keyboard-control', {
    ...commonKeyboardOptions,
    ...keyboardLayouts[keyboardDisplay.value.value].keyboardControlPad,
  });
  keyboardArrows = new Keyboard('.simple-keyboard-arrows', {
    ...commonKeyboardOptions,
    ...keyboardLayouts[keyboardDisplay.value.value].keyboardArrows,
  });
  keyboardNumPad = new Keyboard('.simple-keyboard-numpad', {
    ...commonKeyboardOptions,
    ...keyboardLayouts[keyboardDisplay.value.value].keyboardNumPad,
  });
  keyboardNumPadEnd = new Keyboard('.simple-keyboard-numpadEnd', {
    ...commonKeyboardOptions,
    ...keyboardLayouts[keyboardDisplay.value.value].keyboardNumPadEnd,
  });
});

const switchLayout = (value: string) => {
  if (keyboard && keyboardNumPadEnd) {
    keyboard.setOptions({ display: keyboardLayouts[value].keyboard.display });
  }
};
</script>

<style lang="sass">
.virtual-keyboard
  height: 100vh
  display: flex
  align-items: center
  padding: 1em

  &:before
    content: "\F030C"

  .hg-button
    height: 6em

    span
      font-size: 20px

  .keyboard-container
    display: flex
    background-color: rgba($black, .1)
    width: 100vw
    margin: 0 auto
    border-radius: 5px
    z-index: 1

    .simple-keyboard-main.simple-keyboard
      flex: 2.5
      background-color: transparent

    .simple-keyboard-arrows.simple-keyboard
      align-self: flex-end
      background-color: transparent

    .control-arrows
      flex: .5
      display: flex
      align-items: center
      justify-content: space-between
      flex-flow: column

      .logo
        height: 10em
        width: 8em

    .simple-keyboard-control.simple-keyboard
      background-color: transparent

    .right-controls
      flex: .5
      display: flex
      justify-content: space-between
      flex-flow: column

      .lang-selector
        padding: 5px 5px 0 5px

      .num-pad
        display: flex
        align-items: flex-end

        .simple-keyboard-numpad.simple-keyboard
          background-color: transparent

        .simple-keyboard-numpadEnd.simple-keyboard
          width: 50px
          background: transparent
          margin: 0
          padding: 5px 5px 5px 0
</style>
