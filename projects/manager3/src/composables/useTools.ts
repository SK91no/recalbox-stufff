import { i18n } from 'boot/i18n';
import { copyToClipboard, Notify } from 'quasar';
import { SimpleStringListObject, StringListObject } from 'stores/types/misc';
import { t } from 'src/boot/i18n';

export const useTools = () => {
  const copy = (content: string) => {
    copyToClipboard(content)
      .then(() => {
        Notify.create({
          message: i18n.global.t('general.copyToClipboard'),
          color: 'positive',
          icon: 'mdi-clipboard-check',
        });
      }, (error) => {
        Notify.create({
          message: error,
          type: 'negative',
          icon: 'mdi-alert-outline',
        });
      });
  };

  const allowedStringListToSelectOptions = (stringListObject: SimpleStringListObject) => stringListObject.allowedStringList.sort().map(
    (value) => (
      {
        value,
        label: value,
      }
    ),
  );

  const translatedAllowedStringListToSelectOptions = (
    translationKey: string,
    stringListObject: SimpleStringListObject,
  ) => stringListObject.allowedStringList.sort()
    .filter((value) => value.trim() !== '')
    .map((value) => ({
      value,
      label: t(`${translationKey}.${value.replace(/\u00a0/g, '')}`),
    }));

  const stripLeadingIcon = (text: string) => text.replace(/^[^\p{L}\p{N}]+/u, '').trim();

  const displayableStringListToSelectOptions = (stringListObject: StringListObject) => stringListObject.allowedStringList.map(
    (value, index) => (
      {
        value,
        label: stringListObject.displayableStringList[index],
      }
    ),
  ).sort((a, b) => stripLeadingIcon(a.label).localeCompare(stripLeadingIcon(b.label)));

  return {
    copy,
    displayableStringListToSelectOptions,
    allowedStringListToSelectOptions,
    translatedAllowedStringListToSelectOptions,
  };
};
