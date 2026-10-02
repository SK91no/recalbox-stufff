<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <div class="row">
    <div class="col col-xs-12 col-sm-12 col-md-6">
      <FormFragmentContainer title="settings.emustation.display.title">
        <template v-slot:content>
          <WrappedSelect
            label="settings.emustation.display.videomode.label"
            :options="esVideomodeOptions"
            :getter="system['es.videomode']"
            :setter="systemStore.post"
            apiKey="es.videomode"
            v-if="system['es.videomode']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.display.videomode.help') }}
            </template>
          </WrappedSelect>
          <WrappedToggle
            label="settings.emustation.display.force43.label"
            :getter="system['es.force43']"
            :setter="systemStore.post"
            apiKey="es.force43"
            v-if="system['es.force43']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.display.force43.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.emustation.menus.title">
        <template v-slot:content>
          <WrappedSelect
            label="settings.emustation.menus.menuStyle.label"
            :options="translatedMenuOptions"
            :getter="emulationstation.menu"
            :setter="emulationstationStore.post"
            apiKey="menu"
            v-if="emulationstation.menu"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.menuStyle.help.availableOptions') }}
              <ul>
                <li v-html="t('settings.emustation.menus.menuStyle.help.bartop')"></li>
                <li v-html="t('settings.emustation.menus.menuStyle.help.default')"></li>
                <li v-html="t('settings.emustation.menus.menuStyle.help.none')"></li>
              </ul>
            </template>
          </WrappedSelect>

          <q-separator/>

          <WrappedSelect
            label="settings.emustation.menus.systemSelectedAsDefault.label"
            :options="selectedsystemOptions"
            :getter="emulationstation.selectedsystem"
            :setter="emulationstationStore.post"
            apiKey="selectedsystem"
            v-if="emulationstation.selectedsystem"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.systemSelectedAsDefault.help') }}
            </template>
          </WrappedSelect>

          <q-separator/>

          <WrappedToggle
            label="settings.emustation.menus.startFromFirstSystem.label"
            :getter="emulationstation.bootongamelist"
            :setter="emulationstationStore.post"
            apiKey="bootongamelist"
            v-if="emulationstation.bootongamelist"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.startFromFirstSystem.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.menus.disableSystemSelection.label"
            :getter="emulationstation.hidesystemview"
            :setter="emulationstationStore.post"
            apiKey="hidesystemview"
            v-if="emulationstation.hidesystemview"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.disableSystemSelection.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.menus.quickSystemSelect.label"
            :getter="emulationstation.quicksystemselect"
            :setter="emulationstationStore.post"
            apiKey="quicksystemselect"
            v-if="emulationstation.quicksystemselect"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.quickSystemSelect.help') }}
            </template>
          </WrappedToggle>
          <WrappedSelect
            label="settings.emustation.menus.systemsorting.label"
            :options="translatedSystemSortingOptions"
            :getter="emulationstation.systemsorting"
            :setter="emulationstationStore.post"
            apiKey="systemsorting"
            v-if="emulationstation.systemsorting"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.systemsorting.help') }}
            </template>
          </WrappedSelect>
          <WrappedToggle
            label="settings.emustation.menus.showOnlyScrapedGames.label"
            :getter="emulationstation.gamelistonly"
            :setter="emulationstationStore.post"
            apiKey="gamelistonly"
            v-if="emulationstation.gamelistonly"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.showOnlyScrapedGames.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.menus.clock.label"
            :getter="emulationstation.clock"
            :setter="emulationstationStore.post"
            apiKey="clock"
            v-if="emulationstation.clock"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.clock.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.menus.enableeditfavorites.label"
            :getter="frontend.enableeditfavorites"
            :setter="frontendStore.post"
            apiKey="enableeditfavorites"
            v-if="frontend.enableeditfavorites"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.listfastmovenabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.menus.listfastmovenabled.label"
            :getter="frontend.listfastmovenabled"
            :setter="frontendStore.post"
            apiKey="listfastmovenabled"
            v-if="frontend.listfastmovenabled"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.listfastmovenabled.help') }}
            </template>
          </WrappedToggle>
          <WrappedSelect
            label="settings.emustation.menus.gameTransition.label"
            :options="translatedGameTransitionOptions"
            :getter="emulationstation['game.transition']"
            :setter="emulationstationStore.post"
            apiKey="game.transition"
            v-if="emulationstation['game.transition']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.menus.gameTransition.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>

      <FormFragmentContainer title="settings.emustation.filters.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.emustation.filters.showonlylatestversion.label"
            :getter="emulationstation.showonlylatestversion"
            :setter="emulationstationStore.post"
            apiKey="showonlylatestversion"
            v-if="emulationstation.showonlylatestversion"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.showonlylatestversion.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.filters.favoritesonly.label"
            :getter="emulationstation.favoritesonly"
            :setter="emulationstationStore.post"
            apiKey="favoritesonly"
            v-if="emulationstation.favoritesonly"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.favoritesonly.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.filters.showhidden.label"
            :getter="emulationstation.showhidden"
            :setter="emulationstationStore.post"
            apiKey="showhidden"
            v-if="emulationstation.showhidden"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.showhidden.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.filters.filteradultgames.label"
            :getter="emulationstation.filteradultgames"
            :setter="emulationstationStore.post"
            apiKey="filteradultgames"
            v-if="emulationstation.filteradultgames"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.filteradultgames.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.filters.hidepreinstalledgames.label"
            :getter="global.hidepreinstalledgames"
            :setter="globalStore.post"
            apiKey="hidepreinstalledgames"
            v-if="global.hidepreinstalledgames"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.hidepreinstalledgames.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.filters.showonly3plusplayers.label"
            :getter="emulationstation.showonly3plusplayers"
            :setter="emulationstationStore.post"
            apiKey="showonly3plusplayers"
            v-if="emulationstation.showonly3plusplayers"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.showonly3plusplayers.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.filters.yokoonly.label"
            :getter="emulationstation.yokoonly"
            :setter="emulationstationStore.post"
            apiKey="yokoonly"
            v-if="emulationstation.yokoonly"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.yokoonly.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.filters.tateonly.label"
            :getter="emulationstation.tateonly"
            :setter="emulationstationStore.post"
            apiKey="tateonly"
            v-if="emulationstation.tateonly"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.tateonly.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.filters.hidenogames.label"
            :getter="emulationstation.hidenogames"
            :setter="emulationstationStore.post"
            apiKey="hidenogames"
            v-if="emulationstation.hidenogames"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.hidenogames.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.filters.displaybyfilename.label"
            :getter="emulationstation.displaybyfilename"
            :setter="emulationstationStore.post"
            apiKey="displaybyfilename"
            v-if="emulationstation.displaybyfilename"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.filters.displaybyfilename.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>
    </div>
    <div class="col col-xs-12 col-sm-12 col-md-6">
      <FormFragmentContainer title="settings.emustation.virtualsystems.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.emustation.virtualsystems.allgames.label"
            :getter="emulationstation['collection.allgames']"
            :setter="emulationstationStore.post"
            apiKey="collection.allgames"
            v-if="emulationstation['collection.allgames']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.virtualsystems.allgames.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.virtualsystems.multiplayer.label"
            :getter="emulationstation['collection.multiplayer']"
            :setter="emulationstationStore.post"
            apiKey="collection.multiplayer"
            v-if="emulationstation['collection.multiplayer']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.virtualsystems.multiplayer.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.virtualsystems.lastplayed.label"
            :getter="emulationstation['collection.lastplayed']"
            :setter="emulationstationStore.post"
            apiKey="collection.lastplayed"
            v-if="emulationstation['collection.lastplayed']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.virtualsystems.lastplayed.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.virtualsystems.lightgun.label"
            :getter="emulationstation['collection.lightgun.hide']"
            :setter="emulationstationStore.post"
            apiKey="collection.lightgun.hide"
            v-if="emulationstation['collection.lightgun.hide']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.virtualsystems.lightgun.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.virtualsystems.ports.label"
            :getter="emulationstation['collection.ports.hide']"
            :setter="emulationstationStore.post"
            apiKey="collection.ports.hide"
            v-if="emulationstation['collection.ports.hide']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.virtualsystems.ports.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.virtualsystems.tate.label"
            :getter="emulationstation['collection.tate']"
            :setter="emulationstationStore.post"
            apiKey="collection.tate"
            v-if="emulationstation['collection.tate']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.virtualsystems.tate.help') }}
            </template>
          </WrappedToggle>
          <WrappedToggle
            label="settings.emustation.tateonly.label"
            :getter="emulationstation.tateonly"
            :setter="emulationstationStore.post"
            apiKey="tateonly"
            v-if="emulationstation.tateonly"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.tateonly.help') }}
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.emustation.theme.title">
        <template v-slot:content>
          <WrappedSelect
            label="settings.emustation.theme.folder.label"
            :options="themeFolderOptions"
            :getter="emulationstation['theme.folder']"
            :setter="emulationstationStore.post"
            apiKey="theme.folder"
            v-if="emulationstation['theme.folder']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.theme.folder.help') }}
            </template>
          </WrappedSelect>
          <WrappedSelect
            label="settings.emustation.theme.transition.label"
            :options="translatedThemeTransitionOptions"
            :getter="emulationstation['theme.transition']"
            :setter="emulationstationStore.post"
            apiKey="theme.transition"
            v-if="emulationstation['theme.transition']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.theme.transition.help') }}
            </template>
          </WrappedSelect>
          <WrappedSelect
            label="settings.emustation.theme.region.label"
            :options="translatedThemeRegionOptions"
            :getter="emulationstation['theme.region']"
            :setter="emulationstationStore.post"
            apiKey="theme.region"
            v-if="emulationstation['theme.region']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.theme.region.help') }}
            </template>
          </WrappedSelect>
          <WrappedToggle
            label="settings.emustation.theme.carousel.label"
            :getter="emulationstation['theme.carousel']"
            :setter="emulationstationStore.post"
            apiKey="theme.carousel"
            v-if="emulationstation['theme.carousel']"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.theme.carousel.help.availableOptions') }}
              <ul>
                <li v-html="t('settings.emustation.theme.carousel.help.0')"></li>
                <li v-html="t('settings.emustation.theme.carousel.help.1')"></li>
              </ul>
            </template>
          </WrappedToggle>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.emustation.popups.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.emustation.popups.showhelp.label"
            :getter="emulationstation.showhelp"
            :setter="emulationstationStore.post"
            apiKey="showhelp"
            v-if="emulationstation.showhelp"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.popups.showhelp.help') }}
            </template>
          </WrappedToggle>
          <WrappedSlider
            label="settings.emustation.popups.popup.help.label"
            :getter="emulationstation['popoup.help']"
            :setter="emulationstationStore.post"
            apiKey="popoup.help"
            v-if="emulationstation['popoup.help']"
            :min="popupHelpOptions.lowerValue"
            :max="popupHelpOptions.higherValue"
            :disable="!emulationstation['showhelp'].value"
            icon="mdi-comment-processing-outline"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.popups.popup.help.help') }}
            </template>
          </WrappedSlider>
          <WrappedSlider
            label="settings.emustation.popups.popup.music.label"
            :getter="emulationstation['popoup.music']"
            :setter="emulationstationStore.post"
            apiKey="popoup.music"
            v-if="emulationstation['popoup.music']"
            :min="popupMusicOptions.lowerValue"
            :max="popupMusicOptions.higherValue"
            :disable="!emulationstation['showhelp'].value"
            icon="mdi-comment-processing-outline"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.popups.popup.music.help') }}
            </template>
          </WrappedSlider>
          <WrappedSlider
            label="settings.emustation.popups.popup.netplay.label"
            :getter="emulationstation['popoup.netplay']"
            :setter="emulationstationStore.post"
            apiKey="popoup.netplay"
            v-if="emulationstation['popoup.netplay']"
            :min="popupNetplayOptions.lowerValue"
            :max="popupNetplayOptions.higherValue"
            :disable="!emulationstation['showhelp'].value"
            icon="mdi-comment-processing-outline"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.popups.popup.netplay.help') }}
            </template>
          </WrappedSlider>
        </template>
      </FormFragmentContainer>
      <FormFragmentContainer title="settings.emustation.onegameonerom.title">
        <template v-slot:content>
          <WrappedToggle
            label="settings.emustation.onegameonerom.onegameonerom.label"
            :getter="emulationstation.onegameonerom"
            :setter="emulationstationStore.post"
            apiKey="onegameonerom"
            v-if="emulationstation.onegameonerom"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.onegameonerom.onegameonerom.help') }}
            </template>
          </WrappedToggle>
          <WrappedSelect
            label="settings.emustation.onegameonerom.rompreferredregion.label"
            :options="translatedRomPreferredRegionOptions"
            :getter="emulationstation.rompreferredregion"
            :setter="emulationstationStore.post"
            apiKey="rompreferredregion"
            v-if="emulationstation.rompreferredregion"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.onegameonerom.rompreferredregion.help') }}
            </template>
          </WrappedSelect>
          <WrappedSelect
            label="settings.emustation.onegameonerom.romsecondpreferredregion.label"
            :options="translatedRomSecondPreferredRegionOptions"
            :getter="emulationstation.romsecondpreferredregion"
            :setter="emulationstationStore.post"
            apiKey="romsecondpreferredregion"
            v-if="emulationstation.romsecondpreferredregion"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.onegameonerom.romsecondpreferredregion.help') }}
            </template>
          </WrappedSelect>
          <WrappedSelect
            label="settings.emustation.onegameonerom.rompreferredfallback.label"
            :options="romPreferredFallbackOptions"
            :getter="emulationstation.rompreferredfallback"
            :setter="emulationstationStore.post"
            apiKey="rompreferredfallback"
            v-if="emulationstation.rompreferredfallback"
            help
          >
            <template v-slot:help>
              {{ t('settings.emustation.onegameonerom.rompreferredfallback.help') }}
            </template>
          </WrappedSelect>
        </template>
      </FormFragmentContainer>
    </div>
  </div>
</template>

<script lang="ts" setup>
import WrappedSelect from 'components/ui-kit/WrappedSelect.vue';
import WrappedToggle from 'components/ui-kit/WrappedToggle.vue';
import { useTools } from 'composables/useTools';
import { useEmulationstationStore } from 'stores/configuration/emulationstation';
import { storeToRefs } from 'pinia';
import { useSystemStore } from 'stores/configuration/system';
import WrappedSlider from 'components/ui-kit/WrappedSlider.vue';
import FormFragmentContainer from 'components/ui-kit/FormFragmentContainer.vue';
import { useGlobalStore } from 'stores/configuration/global';
import { useFrontendStore } from 'stores/configuration/frontend';
import { useI18n } from 'vue-i18n';
import { computed } from 'vue';

const { t } = useI18n({ useScope: 'global' });
const { translatedAllowedStringListToSelectOptions } = useTools();

const emulationstationStore = useEmulationstationStore();
emulationstationStore.fetch();

const systemStore = useSystemStore();
systemStore.fetch();

const {
  menuOptions,
  selectedsystemOptions,
  themeFolderOptions,
  themeTransitionOptions,
  themeRegionOptions,
  popupHelpOptions,
  popupMusicOptions,
  popupNetplayOptions,
  systemSortingOptions,
  romPreferredRegionOptions,
  romSecondPreferredRegionOptions,
  romPreferredFallbackOptions,
  gameTransitionOptions,
  emulationstation,
} = storeToRefs(emulationstationStore);
const { esVideomodeOptions, system } = storeToRefs(systemStore);

const globalStore = useGlobalStore();
globalStore.fetch();
const {
  global,
} = storeToRefs(globalStore);

const frontendStore = useFrontendStore();
frontendStore.fetch();
const { frontend } = storeToRefs(frontendStore);

const translatedMenuOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.emustation.menus.menuStyle.options',
  menuOptions.value,
));

const translatedSystemSortingOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.emustation.menus.systemsorting.options',
  systemSortingOptions.value,
));

const translatedThemeTransitionOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.emustation.theme.transition.options',
  themeTransitionOptions.value,
));

const translatedThemeRegionOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.emustation.theme.region.options',
  themeRegionOptions.value,
));

const translatedRomPreferredRegionOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'regions.list',
  romPreferredRegionOptions.value
));

const translatedRomSecondPreferredRegionOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'regions.list',
  romSecondPreferredRegionOptions.value
));

const translatedGameTransitionOptions = computed(() => translatedAllowedStringListToSelectOptions(
  'settings.emustation.menus.gameTransition.options',
  gameTransitionOptions.value,
));
</script>

<style lang="sass" scoped>
.q-tab-panel > .row
  margin-bottom: 5em
</style>
