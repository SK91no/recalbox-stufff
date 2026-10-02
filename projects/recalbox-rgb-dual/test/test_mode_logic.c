#include "../mode_logic.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;

static const char *hat_name(enum HatReference h) {
   switch (h) {
      case RecalboxRGBDual:   return "Dual";
      case RecalboxRGBDual2:  return "Dual2";
      case RecalboxRGBJAMMA:  return "JAMMA";
      case RecalboxRGBJAMMA2: return "JAMMA2";
      case RecalboxJVS:       return "JVS";
      default:                return "OTHER";
   }
}

static const char *st_name(enum ScreenType s) {
   switch (s) {
      case Auto:         return "Auto";
      case KHz15:        return "KHz15";
      case KHzMulti1525: return "KHzMulti1525";
      case KHz31:        return "KHz31";
      case KHzMulti1531: return "KHzMulti1531";
      case KHzTriFreq:   return "KHzTriFreq";
   }
   return "?";
}

#define EXPECT(actual, expected, what) do {                                              \
   int _a = (int)(actual), _e = (int)(expected);                                          \
   if (_a != _e) {                                                                        \
      fprintf(stderr, "FAIL [%s / %s / gpio=%d] %s: got %d, expected %d\n",               \
              hat_name(hat), st_name(st), gpio, what, _a, _e);                            \
      fails++;                                                                            \
   }                                                                                      \
} while (0)

static void check(enum HatReference hat, enum ScreenType st, int gpio,
                  bool f31_only, bool s31, bool s25, bool s120, int default_res) {
   EXPECT(ml_forced31kHzOnly(st, hat, gpio), f31_only, "forced31kHzOnly");
   EXPECT(ml_supports31kHz   (st, hat, gpio), s31,     "supports31kHz");
   EXPECT(ml_supports25kHz   (st, hat, gpio), s25,     "supports25kHz");
   EXPECT(ml_supports15kHz   (st, hat, gpio), !f31_only, "supports15kHz");
   EXPECT(ml_supports31kHz120Hz(hat),         s120,    "supports31kHz120Hz");
   EXPECT(ml_default_resolution(st, hat, gpio), default_res, "default_resolution");
}

int main(void) {
   enum HatReference hat;
   enum ScreenType st;
   int gpio;

   /* ==== JVS ====
    * Default = 31kHz (Auto). 15/25 kHz only via explicit screentype.
    * GPIO has no effect on JVS. */
   hat = RecalboxJVS;
   gpio = 0;
   /* st, gpio, f31only, s31, s25, s120, default_res */
   st = Auto;         check(hat, st, gpio, true,  true,  false, false, 480);
   st = KHz31;        check(hat, st, gpio, true,  true,  false, false, 480);
   st = KHz15;        check(hat, st, gpio, false, false, false, false, 240);
   st = KHzMulti1525; check(hat, st, gpio, false, false, true,  false, 240);
   st = KHzMulti1531; check(hat, st, gpio, false, true,  false, false, 480);
   st = KHzTriFreq;   check(hat, st, gpio, false, true,  true,  false, 480);
   gpio = 1;  /* same result */
   st = Auto;         check(hat, st, gpio, true,  true,  false, false, 480);
   st = KHz31;        check(hat, st, gpio, true,  true,  false, false, 480);

   /* ==== JAMMA / JAMMA2 ====
    * Default = 15kHz (Auto). 31/25 kHz only via explicit screentype.
    * GPIO has no effect. */
   gpio = 0;
   for (int j = 0; j < 2; j++) {
      hat = (j == 0) ? RecalboxRGBJAMMA : RecalboxRGBJAMMA2;
      st = Auto;         check(hat, st, gpio, false, false, false, false, 240);
      st = KHz15;        check(hat, st, gpio, false, false, false, false, 240);
      st = KHz31;        check(hat, st, gpio, true,  true,  false, false, 480);
      st = KHzMulti1525; check(hat, st, gpio, false, false, true,  false, 240);
      st = KHzMulti1531; check(hat, st, gpio, false, true,  false, false, 480);
      st = KHzTriFreq;   check(hat, st, gpio, false, true,  true,  false, 480);
   }

   /* ==== Dual / Dual2 ====
    * Dipswitch (gpio_state) gates everything past 15kHz.
    *   gpio=1 (open):    15kHz only, regardless of screentype.
    *   gpio=0 (engaged): screentype drives the choice; Auto/KHz31 = forced31kHzOnly. */
   for (int d = 0; d < 2; d++) {
      hat = (d == 0) ? RecalboxRGBDual : RecalboxRGBDual2;
      /* gpio=1: dipswitch open → only 15kHz */
      gpio = 1;
      st = Auto;         check(hat, st, gpio, false, false, false, true, 240);
      st = KHz15;        check(hat, st, gpio, false, false, false, true, 240);
      st = KHz31;        check(hat, st, gpio, false, false, false, true, 240);
      st = KHzMulti1525; check(hat, st, gpio, false, false, false, true, 240);
      st = KHzMulti1531; check(hat, st, gpio, false, false, false, true, 240);
      st = KHzTriFreq;   check(hat, st, gpio, false, false, false, true, 240);
      /* gpio=0: dipswitch engaged → screentype-driven */
      gpio = 0;
      st = Auto;         check(hat, st, gpio, true,  true,  false, true, 480);
      st = KHz31;        check(hat, st, gpio, true,  true,  false, true, 480);
      st = KHz15;        check(hat, st, gpio, false, false, false, true, 240);
      st = KHzMulti1525; check(hat, st, gpio, false, false, true,  true, 240);
      st = KHzMulti1531; check(hat, st, gpio, false, true,  false, true, 480);
      st = KHzTriFreq;   check(hat, st, gpio, false, true,  true,  true, 480);
   }

   /* ==== supports50Hz: Dual/Dual2 only ==== */
   {
      const struct { enum HatReference h; bool expected; } cases[] = {
         { RecalboxJVS,         false },
         { RecalboxRGBJAMMA,    false },
         { RecalboxRGBJAMMA2,   false },
         { RecalboxRGBDual,     true  },
         { RecalboxRGBDual2,    true  },
      };
      for (unsigned i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
         if (ml_supports50Hz(cases[i].h) != cases[i].expected) {
            fprintf(stderr, "FAIL supports50Hz(%s): got %d, expected %d\n",
                    hat_name(cases[i].h),
                    (int)ml_supports50Hz(cases[i].h),
                    (int)cases[i].expected);
            fails++;
         }
      }
   }

   if (fails == 0) {
      printf("All mode_logic tests passed.\n");
      return 0;
   }
   fprintf(stderr, "%d mode_logic test(s) failed.\n", fails);
   return 1;
}
