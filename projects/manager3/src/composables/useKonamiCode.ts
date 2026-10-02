import { EventBus } from 'quasar';
import { inject, onMounted, onUnmounted } from 'vue';

export function useKonamiCode() {
  const bus = inject<EventBus>('bus');

  if (bus) {
    const konamiCode = [
      'ArrowUp',
      'ArrowUp',
      'ArrowDown',
      'ArrowDown',
      'ArrowLeft',
      'ArrowRight',
      'ArrowLeft',
      'ArrowRight',
      'b',
      'a',
    ];

    let konamiIndex = 0;

    const keyHandler = (event: KeyboardEvent) => {
      const { key } = event;

      if (key.toLowerCase() === konamiCode[konamiIndex].toLowerCase()) {
        konamiIndex += 1;
        if (konamiIndex === konamiCode.length) {
          bus.emit('konami-code');
          konamiIndex = 0;
        }
      } else {
        konamiIndex = 0;
      }
    };

    onMounted(() => {
      window.addEventListener('keydown', keyHandler);
    });

    onUnmounted(() => {
      window.removeEventListener('keydown', keyHandler);
    });
  }
}
