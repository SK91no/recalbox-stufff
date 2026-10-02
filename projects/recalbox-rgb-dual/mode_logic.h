#ifndef RECALBOXRGBDUAL_MODE_LOGIC_H
#define RECALBOXRGBDUAL_MODE_LOGIC_H

/*
 * Mode availability helpers (pure functions, no kernel deps).
 * Shared between recalboxrgbdual.c and the userspace unit-test harness.
 * Spec:
 *   JVS:        31kHz is the hardware default; 15/25 kHz only via explicit screentype.
 *   RGB JAMMA:  15kHz is the default (Auto); 31/25 only via explicit screentype.
 *   RGB Dual/2: 15kHz unless the 31kHz dipswitch is engaged (gpio_state==0) AND screentype accepts other freqs.
 */

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdbool.h>
#endif

enum ScreenType {
  Auto = 0,
  KHz15,
  KHzMulti1525,
  KHz31,
  KHzMulti1531,
  KHzTriFreq,
};

enum HatReference {
  RecalboxRGBDual = 0,
  RecalboxRGBJAMMA,
  RecalboxRGBJAMMA2,
  RecalboxRGBDual2,
  RecalboxJVS,
  OTHER,
};

static inline bool ml_isRGBJAMMA(enum HatReference hat) {
  return hat == RecalboxRGBJAMMA || hat == RecalboxRGBJAMMA2;
}

static inline bool ml_isJVS(enum HatReference hat) {
  return hat == RecalboxJVS;
}

/* 31kHz only — every other mode hidden. */
static inline bool ml_forced31kHzOnly(enum ScreenType st, enum HatReference hat, int gpio31_state) {
  if (ml_isJVS(hat))
    return st == Auto || st == KHz31;
  if (ml_isRGBJAMMA(hat))
    return st == KHz31;
  return gpio31_state == 0 && (st == Auto || st == KHz31);
}

/* 31kHz progressive modes (640x480p, etc) available. */
static inline bool ml_supports31kHz(enum ScreenType st, enum HatReference hat, int gpio31_state) {
  if (ml_isJVS(hat))
    return st != KHz15 && st != KHzMulti1525;
  if (ml_isRGBJAMMA(hat))
    return st == KHz31 || st == KHzMulti1531 || st == KHzTriFreq;
  return gpio31_state == 0
      && (st == Auto || st == KHz31 || st == KHzMulti1531 || st == KHzTriFreq);
}

/* 25kHz multisync modes (384p) available — needs explicit Multi1525 or TriFreq. */
static inline bool ml_supports25kHz(enum ScreenType st, enum HatReference hat, int gpio31_state) {
  if (st != KHzMulti1525 && st != KHzTriFreq)
    return false;
  if (ml_isRGBJAMMA(hat) || ml_isJVS(hat))
    return true;
  return gpio31_state == 0;
}

/* 15kHz modes available — complement of forced31kHzOnly. */
static inline bool ml_supports15kHz(enum ScreenType st, enum HatReference hat, int gpio31_state) {
  return !ml_forced31kHzOnly(st, hat, gpio31_state);
}

/* 240p@120Hz available — Dual/Dual2 only. */
static inline bool ml_supports31kHz120Hz(enum HatReference hat) {
  return !ml_isRGBJAMMA(hat) && !ml_isJVS(hat);
}

/* 50Hz modes (15kHz 288p / 576i) — Dual/Dual2 only. */
static inline bool ml_supports50Hz(enum HatReference hat) {
  return !ml_isRGBJAMMA(hat) && !ml_isJVS(hat);
}

/* Fallback default vertical resolution when options.es.resolution isn't set.
 * Per-screentype defaults: Auto = hardware natural (31 on JVS/Dual+dipswitch,
 * 15 elsewhere); KHz15 / KHzMulti1525 baseline = 15kHz (240);
 * KHz31 / KHzMulti1531 / KHzTriFreq prefer 31kHz when supported. */
static inline int ml_default_resolution(enum ScreenType st, enum HatReference hat, int gpio31_state) {
  if (st == Auto)
    return ml_forced31kHzOnly(st, hat, gpio31_state) ? 480 : 240;
  if (st == KHz15 || st == KHzMulti1525)
    return 240;
  if (ml_supports31kHz(st, hat, gpio31_state)) return 480;
  if (ml_supports25kHz(st, hat, gpio31_state)) return 384;
  return 240;
}

#endif /* RECALBOXRGBDUAL_MODE_LOGIC_H */
