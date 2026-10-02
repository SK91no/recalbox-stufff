/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { RouteRecordRaw } from 'vue-router';

const MainLayout = () => import('layouts/MainLayout.vue');

const HomePage = () => import('pages/HomePage.vue');
const MonitoringPage = () => import('pages/MonitoringPage.vue');

const SettingsPage = () => import('pages/SettingsPage.vue');
const SettingsSystemTabContent = () => import('components/settings/SettingsSystemTabContent.vue');
const SettingsAudioTabContent = () => import('components/settings/SettingsAudioTabContent.vue');
const SettingsNetworkTabContent = () => import('components/settings/SettingsNetworkTabContent.vue');
const SettingsEmuStationTabContent = () => import('components/settings/SettingsEmuStationTabContent.vue');
const SettingsScraperTabContent = () => import('components/settings/SettingsScraperTabContent.vue');
const SettingsKodiTabContent = () => import('components/settings/SettingsKodiTabContent.vue');
const SettingsHyperionTabContent = () => import('components/settings/SettingsHyperionTabContent.vue');

const EmulationPage = () => import('pages/EmulationPage.vue');
const EmulationGlobalTabContent = () => import('components/emulation/EmulationGlobalTabContent.vue');
const EmulationNetplayTabContent = () => import('components/emulation/EmulationNetplayTabContent.vue');
const EmulationRetroachievementsTabContent = () => import('components/emulation/EmulationRetroachievementsTabContent.vue');
const EmulationBiosTabContent = () => import('components/emulation/EmulationBiosTabContent.vue');
const EmulationControllersTabContent = () => import('components/emulation/EmulationControllersTabContent.vue');
const EmulationSystemsContainer = () => import('components/emulation/EmulationSystemsRomsContainer.vue');
const EmulationSystemsTabContent = () => import('components/emulation/EmulationSystemsRomsTabSystemsContent.vue');
const EmulationSystemsTabSystemContent = () => import('components/emulation/EmulationSystemsRomsTabSystemContent.vue');

const VirtualDevicesLayout = () => import('layouts/VirtualDevicesLayout.vue');
const VirtualKeyboard = () => import('components/virtualdevices/VirtualKeyboard.vue');
const VirtualTrackpad = () => import('components/virtualdevices/VirtualTrackpad.vue');
const VirtualGamepad = () => import('components/virtualdevices/VirtualGamepad.vue');

const ScreenshotsPage = () => import('pages/ScreenshotsPage.vue');

const PatreonPage = () => import('pages/PatreonPage.vue');

const routes: RouteRecordRaw[] = [
  {
    path: '/',
    component: MainLayout,
    children: [
      {
        path: '',
        name: 'home',
        component: HomePage,
      },
      {
        path: 'monitoring',
        name: 'monitoring',
        component: MonitoringPage,
      },
      {
        path: 'settings',
        name: 'settings',
        redirect: { name: 'system' },
        component: SettingsPage,
        children: [
          {
            path: 'system',
            name: 'system',
            component: SettingsSystemTabContent,
          },
          {
            path: 'audio',
            name: 'audio',
            component: SettingsAudioTabContent,
          },
          {
            path: 'network',
            name: 'network',
            component: SettingsNetworkTabContent,
          },
          {
            path: 'emustation',
            name: 'emustation',
            component: SettingsEmuStationTabContent,
          },
          {
            path: 'scraper',
            name: 'scraper',
            component: SettingsScraperTabContent,
          },
          {
            path: 'kodi',
            name: 'kodi',
            component: SettingsKodiTabContent,
          },
          {
            path: 'hyperion',
            name: 'hyperion',
            component: SettingsHyperionTabContent,
          },
        ],
      },
      {
        path: 'emulation',
        name: 'emulation',
        redirect: { name: 'global' },
        component: EmulationPage,
        children: [
          {
            path: 'global',
            name: 'global',
            component: EmulationGlobalTabContent,
          },
          {
            path: 'netplay',
            name: 'netplay',
            component: EmulationNetplayTabContent,
          },
          {
            path: 'retroachievements',
            name: 'retroachievements',
            component: EmulationRetroachievementsTabContent,
          },
          {
            path: 'bios',
            name: 'bios',
            component: EmulationBiosTabContent,
          },
          {
            path: 'systems',
            name: 'systems-parent',
            component: EmulationSystemsContainer,
            redirect: { name: 'systems-systems' },
            children: [
              {
                path: '',
                name: 'systems-systems',
                component: EmulationSystemsTabContent,
              },
              {
                path: ':system',
                name: 'systems-system',
                component: EmulationSystemsTabSystemContent,
                props: true,
              },
            ],
          },
          {
            path: 'controllers',
            name: 'controllers',
            component: EmulationControllersTabContent,
          },
        ],
      },
      {
        path: 'screenshots',
        name: 'screenshots',
        component: ScreenshotsPage,
      },
      {
        path: 'patreon',
        name: 'patreon',
        component: PatreonPage,
      },
    ],
  },
  {
    path: '/virtual-devices',
    name: 'virtual-devices',
    redirect: { name: 'virtual-devices-keyboard' },
    component: VirtualDevicesLayout,
    children: [
      {
        path: 'keyboard',
        name: 'virtual-devices-keyboard',
        component: VirtualKeyboard,
      },
      {
        path: 'trackpad',
        name: 'virtual-devices-trackpad',
        component: VirtualTrackpad,
      },
      {
        path: 'gamepad',
        name: 'virtual-devices-gamepad',
        component: VirtualGamepad,
      },
    ],
  },
  // Always leave this as last one,
  // but you can also remove it
  {
    path: '/:catchAll(.*)*',
    component: () => import('pages/ErrorNotFound.vue'),
  },
];

export default routes;
