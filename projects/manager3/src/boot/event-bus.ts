import { EventBus } from 'quasar';
import { boot } from 'quasar/wrappers';

export default boot(({ app }) => {
  app.provide('bus', new EventBus());
});
