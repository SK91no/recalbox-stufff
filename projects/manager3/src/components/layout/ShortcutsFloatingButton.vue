<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-fab
    ref="menu"
    color="white"
    direction="up"
    flat
    icon="mdi-web"
    id="help-button"
    square
    vertical-actions-align="right"
    style="margin-right: 1em;"
  >
    <q-fab-action
      @click="() => urls.patreon && openURL(urls.patreon)"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-patreon"
      label="Patreon"
      label-position="left"
      square
    />
    <q-fab-action
      @click="() => urls.gitlab && openURL(urls.gitlab)"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-gitlab"
      label="Gitlab"
      label-position="left"
      square
    />
    <q-fab-action
      @click="() => urls.forum && openURL(urls.forum)"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-forum"
      label="Forum"
      label-position="left"
      square
    />
    <q-fab-action
      @click="() => urls.wiki && openURL(urls.wiki)"
      class="wiki"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="icon-gitbook"
      label="Documentation"
      label-position="left"
      square
    />
    <q-fab-action
      @click="() => urls.discord && openURL(urls.discord)"
      :color="Dark.isActive ? 'dark-grey-stronger' : 'secondary'"
      icon="mdi-discord"
      label="Discord"
      label-position="left"
      square
    />
  </q-fab>
</template>

<script lang="ts" setup>
import { onClickOutside } from '@vueuse/core';
import { openURL, Dark, QFab } from 'quasar';
import { ref } from 'vue';
import { URL } from 'stores/types/misc';

const urls: URL = {
  discord: process.env.DISCORD_URL,
  wiki: process.env.WIKI_URL,
  forum: process.env.FORUM_URL,
  gitlab: process.env.GITLAB_URL,
  patreon: process.env.PATREON_URL,
};

const menu = ref<InstanceType<typeof QFab> | null>(null);

onClickOutside(menu, () => {
  if (menu.value) {
    menu.value.hide();
  }
});
</script>

<style lang="sass">
.q-page-container
  #help-button
    .q-btn--fab-mini:hover
      .q-icon:before
        color: $accent

    .wiki
      i
        font-size: 18px
        margin-left: 3px
        margin-right: 3px
</style>
