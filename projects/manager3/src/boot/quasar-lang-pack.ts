/**
 * @author Nicolas TESSIER aka Asthonishia
 */
import { LocalStorage, Quasar } from 'quasar';
import type { QuasarLanguage } from 'quasar';

// Map des langues supportées
const langMap: Record<string, () => Promise<{ default: QuasarLanguage }>> = {
  fr: () => import('quasar/lang/fr'),
  'fr-fr': () => import('quasar/lang/fr'),
  en: () => import('quasar/lang/en-US'),
  'en-us': () => import('quasar/lang/en-US'),
};

export default async (): Promise<void> => {
  const locale = (LocalStorage.getItem('locale') as string | null) ?? 'en-us';
  const normalizedLocale = locale.toLowerCase();
  const loader = langMap[normalizedLocale] || langMap['en-us'];

  try {
    const { default: lang } = await loader();
    Quasar.lang.set(lang);
  } catch (err) {
    // eslint-disable-next-line no-console
    console.warn('⚠️ Failed to load Quasar language pack:', locale, err);
  }
};
