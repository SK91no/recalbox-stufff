<!--
@author Nicolas TESSIER aka Asthonishia
-->
<template>
  <q-item clickable v-if="lang" v-ripple>
    <q-item-section avatar>
      <q-icon name="mdi-translate"/>
    </q-item-section>
    <q-item-section>
      {{ t(getTitle()) }}
    </q-item-section>
    <q-menu auto-close>
      <q-list>
        <q-item
          :class="isSelected(langOption.value)"
          :key="langOption.value"
          @click="select(langOption.value)"
          clickable
          v-for="langOption in langOptions"
        >
          <q-item-section avatar>
            <q-img
              :alt="t(langOption.label)"
              :img-style="{ backgroundSize: 'inherit' }"
              :src="`images/flags/${langOption.value}.png`"
            />
          </q-item-section>
          <q-item-section>
            <q-item-label>{{ t(langOption.label) }}</q-item-label>
          </q-item-section>
        </q-item>
      </q-list>
    </q-menu>
  </q-item>
</template>

<script lang="ts" setup>
import { Quasar, LocalStorage, useQuasar } from 'quasar';
import { useI18n } from 'vue-i18n';
import langEn from 'quasar/lang/en-US';
import langFr from 'quasar/lang/fr';

const { locale, t } = useI18n({ useScope: 'global' });
const lang = useQuasar().lang.getLocale();
const langOptions = [
  { value: 'en-US', label: 'i18nMenu.english' },
  { value: 'fr', label: 'i18nMenu.french' },
];
const supportedLangs = ['en-US', 'fr'] as const;
type SupportedLang = typeof supportedLangs[number];

const sanitizeLang = (value: string): SupportedLang => (supportedLangs.includes(value as SupportedLang)
  ? (value as SupportedLang)
  : 'en-US'); // fallback si langue inconnue

const isSelected = (value: string) => (value === locale.value ? 'text-accent' : '');

const getTitle = () => (langOptions.find(
  (option) => option.value === locale.value,
))?.label || '';

const select = async (langValue: string) => {
  try {
    const langIso = sanitizeLang(langValue);

    locale.value = langValue;
    LocalStorage.set('locale', langValue);

    // Map des packs de langue Quasar
    const quasarLangMap: Record<SupportedLang, typeof langEn> = {
      'en-US': langEn,
      fr: langFr,
    };

    Quasar.lang.set(quasarLangMap[langIso]);
  } catch (error) {
    console.error(error);
  }
};
</script>
