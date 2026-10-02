/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { boot } from 'quasar/wrappers';
import { createI18n } from 'vue-i18n';
import { LocalStorage } from 'quasar';
import messages from 'src/i18n';

export type MessageLanguages = keyof typeof messages;
export type MessageSchema = typeof messages['fr'];

declare module 'vue-i18n' {
  // eslint-disable-next-line @typescript-eslint/no-empty-interface
  export interface DefineLocaleMessage extends MessageSchema {}
  // eslint-disable-next-line @typescript-eslint/no-empty-interface
  export interface DefineDateTimeFormat {}
  // eslint-disable-next-line @typescript-eslint/no-empty-interface
  export interface DefineNumberFormat {}
}

// Définir la locale par défaut si manquante
const fallbackLocale = 'fr';
const savedLocale = LocalStorage.getItem('locale') as string | null;
const locale = savedLocale ?? fallbackLocale;

if (!savedLocale) {
  try {
    LocalStorage.set('locale', fallbackLocale);
  } catch (error) {
    // eslint-disable-next-line no-console
    console.error(error);
  }
}

// Init vue-i18n
const i18n = createI18n({
  locale,
  legacy: false,
  warnHtmlMessage: false,
  messages,
});

export default boot(({ app }) => {
  app.use(i18n);
});

export { i18n };
export const t = i18n.global.t.bind(i18n.global);
