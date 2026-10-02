#ifndef RECALBOX_GAMEPAD_PROXY_RGP_LOGIC_H
#define RECALBOX_GAMEPAD_PROXY_RGP_LOGIC_H

/*
 * Pure input-mapping logic shared between recalbox-gamepad-proxy.c and the
 * host unit-test harness (projects/recalbox-gamepad-proxy/test/).
 * No kernel state, no locks, no allocation: parsing + match decisions only.
 * Kernel-side this is included after the driver's own headers; host-side it
 * ships shims for the few kernel helpers it uses.
 */

#ifndef RGP_MAX_NAME_LEN
#define RGP_MAX_NAME_LEN 96
#endif

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/input.h>
#else /* host test harness */
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#ifndef EV_KEY
#define EV_KEY 0x01
#endif
#ifndef EV_ABS
#define EV_ABS 0x03
#endif
#ifndef EINVAL
#define EINVAL 22
#endif
#ifndef EOPNOTSUPP
#define EOPNOTSUPP 95
#endif
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

static inline int rgp_h_kstrtouint(const char *s, unsigned int base, unsigned int *res)
{
  char *end;
  unsigned long v;
  (void)base;
  if (!s || !*s)
    return -EINVAL;
  v = strtoul(s, &end, 0);
  if (*end != '\0')
    return -EINVAL;
  *res = (unsigned int)v;
  return 0;
}

static inline int rgp_h_kstrtoint(const char *s, unsigned int base, int *res)
{
  char *end;
  long v;
  (void)base;
  if (!s || !*s)
    return -EINVAL;
  v = strtol(s, &end, 0);
  if (*end != '\0')
    return -EINVAL;
  *res = (int)v;
  return 0;
}

static inline long rgp_h_strscpy(char *dst, const char *src, size_t n)
{
  size_t len = strlen(src);
  if (len >= n) {
    if (n) {
      memcpy(dst, src, n - 1);
      dst[n - 1] = '\0';
    }
    return -7; /* -E2BIG */
  }
  memcpy(dst, src, len + 1);
  return (long)len;
}

#define kstrtouint rgp_h_kstrtouint
#define kstrtoint  rgp_h_kstrtoint
#define strscpy    rgp_h_strscpy
#endif /* __KERNEL__ */

/* A single external source->target remap, parsed from a `external_maps` entry
 * or the device tree. EV_KEY sources only. */
struct rgp_external_binding {
  char source_name[RGP_MAX_NAME_LEN];
  char target_gamepad_name[RGP_MAX_NAME_LEN];
  unsigned int src_type;
  unsigned int src_code;
  int src_value;
  unsigned int dst_code;
  bool dst_tap;
  bool replace_source;
  bool replace_target;
  bool invert_logic;
};

/* Substring match: does `name` contain `needle`? Empty/NULL needle never matches. */
static inline bool rgp_name_matches(const char *name, const char *needle)
{
  if (!name || !needle || needle[0] == '\0')
    return false;
  return strstr(name, needle) != NULL;
}

/*
 * Parse the mode field of a binding.
 * "1"/"0" are the legacy shorthand for tap/hold with no extra flags.
 * Otherwise the string must contain exactly one of "tap"/"hold" and may add
 * "replace-source"/"replace-target"/"replace"/"invert".
 */
static inline int rgp_parse_binding_mode(const char *mode, struct rgp_external_binding *b)
{
  bool has_tap, has_hold;

  if (!mode || !b)
    return -EINVAL;

  if (!strcmp(mode, "1")) {
    b->dst_tap = true;
    return 0;
  }

  if (!strcmp(mode, "0")) {
    b->dst_tap = false;
    return 0;
  }

  has_tap = strstr(mode, "tap") != NULL;
  has_hold = strstr(mode, "hold") != NULL;

  if (has_tap == has_hold)
    return -EINVAL;

  b->dst_tap = has_tap;

  if (strstr(mode, "replace-source"))
    b->replace_source = true;
  if (strstr(mode, "replace-target"))
    b->replace_target = true;
  if (strstr(mode, "replace") && !b->replace_source && !b->replace_target)
    b->replace_source = true;
  if (strstr(mode, "invert"))
    b->invert_logic = true;

  return 0;
}

/*
 * Parse one ':'-separated binding entry into `b`.
 * Fields: source_name:src_type:src_code:dst_code:mode[:target_name[:src_value]]
 * At least 5 fields, src_type must be EV_KEY.
 */
static inline int rgp_parse_external_binding_string(char *entry, struct rgp_external_binding *b)
{
  char *cursor = entry;
  char *tok;
  char *fields[7];
  int nfields = 0;
  unsigned int uval;

  memset(b, 0, sizeof(*b));
  b->src_value = -1;
  b->replace_source = false;
  b->replace_target = false;
  b->invert_logic = false;

  while ((tok = strsep(&cursor, ":")) != NULL && nfields < (int)ARRAY_SIZE(fields))
    fields[nfields++] = tok;

  if (nfields < 5)
    return -EINVAL;

  if (fields[0][0] == '\0')
    return -EINVAL;

  if (strscpy(b->source_name, fields[0], sizeof(b->source_name)) < 0)
    return -EINVAL;

  if (kstrtouint(fields[1], 0, &uval) < 0)
    return -EINVAL;
  b->src_type = uval;

  if (b->src_type != EV_KEY)
    return -EOPNOTSUPP;

  if (kstrtouint(fields[2], 0, &uval) < 0)
    return -EINVAL;
  b->src_code = uval;

  if (kstrtouint(fields[3], 0, &uval) < 0)
    return -EINVAL;
  b->dst_code = uval;

  if (rgp_parse_binding_mode(fields[4], b) < 0)
    return -EINVAL;

  if (nfields >= 6 && fields[5] && fields[5][0] != '\0')
    strscpy(b->target_gamepad_name, fields[5], sizeof(b->target_gamepad_name));

  if (nfields >= 7 && fields[6] && fields[6][0] != '\0') {
    int ival;
    if (kstrtoint(fields[6], 0, &ival) < 0)
      return -EINVAL;
    b->src_value = ival;
  }

  return 0;
}

/*
 * Does an incoming (type, code, value) event fire this binding?
 * EV_KEY only, value must be 0/1 (autorepeat 2 is ignored).
 * hold: fires on any 0/1 edge (the emitted level is computed elsewhere).
 * tap:  fires only when the (optionally inverted) value equals the tap value.
 * NOTE: invert_logic is consumed here and ONLY affects tap mode; in hold mode
 * it is parsed but has no effect.
 */
static inline bool rgp_binding_matches_event(const struct rgp_external_binding *b,
                                             unsigned int type, unsigned int code, int value)
{
  int tap_value;
  int effective_value = value;

  if (!b)
    return false;
  if (b->src_type != type || b->src_code != code)
    return false;

  if (b->src_type != EV_KEY)
    return false;
  if (value != 0 && value != 1)
    return false;

  if (b->invert_logic)
    effective_value = !value;

  if (b->dst_tap) {
    tap_value = (b->src_value >= 0) ? b->src_value : 1;
    return effective_value == tap_value;
  }

  return true;
}

/* ============================================================================
 * GAME COMBOS (per-game input macros)
 *
 * A game combo emits a virtual key while a MODIFIER key is held AND a TRIGGER
 * fires. The trigger can be a key (press) or an axis moving in one direction,
 * which is what makes this distinct from `external_maps` (EV_KEY only) and from
 * the driver's unrelated START+MODE "exit combo". sysfs grammar (';'-joined):
 *   <target>:<mod-code>:<trig-type>:<trig-code>:<trig-val>:<emit-code>:<mode>
 * mode is "hold" (emit follows the gate) or "tap" (pulse on the rising edge).
 * ==========================================================================*/

struct rgp_game_combo {
  char target_gamepad_name[RGP_MAX_NAME_LEN];
  unsigned int mod_code;   /* EV_KEY modifier that arms the combo */
  unsigned int trig_type;  /* EV_KEY or EV_ABS */
  unsigned int trig_code;
  int trig_val;            /* EV_ABS: signed direction/threshold; EV_KEY: unused */
  unsigned int emit_code;  /* EV_KEY emitted while gated */
  bool emit_tap;           /* tap (pulse) vs hold */
};

/* Parse one ';'-free combo entry. All 7 fields required. */
static inline int rgp_parse_game_combo_string(char *entry, struct rgp_game_combo *c)
{
  char *cursor = entry;
  char *tok;
  char *fields[7];
  int nfields = 0;
  unsigned int uval;
  int ival;

  memset(c, 0, sizeof(*c));

  while ((tok = strsep(&cursor, ":")) != NULL && nfields < (int)ARRAY_SIZE(fields))
    fields[nfields++] = tok;

  if (nfields < 7)
    return -EINVAL;

  if (fields[0][0] == '\0')
    return -EINVAL;
  if (strscpy(c->target_gamepad_name, fields[0], sizeof(c->target_gamepad_name)) < 0)
    return -EINVAL;

  if (kstrtouint(fields[1], 0, &uval) < 0)
    return -EINVAL;
  c->mod_code = uval;

  if (kstrtouint(fields[2], 0, &uval) < 0)
    return -EINVAL;
  c->trig_type = uval;
  if (c->trig_type != EV_KEY && c->trig_type != EV_ABS)
    return -EOPNOTSUPP;

  if (kstrtouint(fields[3], 0, &uval) < 0)
    return -EINVAL;
  c->trig_code = uval;

  if (kstrtoint(fields[4], 0, &ival) < 0)
    return -EINVAL;
  c->trig_val = ival;

  if (kstrtouint(fields[5], 0, &uval) < 0)
    return -EINVAL;
  c->emit_code = uval;

  if (strstr(fields[6], "tap"))
    c->emit_tap = true;
  else if (strstr(fields[6], "hold"))
    c->emit_tap = false;
  else
    return -EINVAL;

  return 0;
}

/*
 * Direction an axis value maps to, as forwarded to the proxy.
 * Hat axes report exact -1/0/+1; analog axes cross a threshold (the rule's own
 * threshold when set, else the global stick threshold). Used to keep the proxy
 * dir-state in sync with what was actually forwarded, so a later return-to-zero
 * is not deduped away and left stuck down.
 */
static inline int rgp_abs_forward_dir(int value, bool use_hat_exact, int thr, int stick_threshold)
{
  if (use_hat_exact)
    return (value < 0) ? -1 : (value > 0) ? 1 : 0;

  {
    int t = (thr > 0) ? thr : stick_threshold;
    return (value <= -t) ? -1 : (value >= t) ? 1 : 0;
  }
}

/*
 * Whether an ABS-direction rule should fire for an axis event on `code` now
 * pointing in `dir`. A rule with NO modifier (nmods == 0) is a plain remap: it
 * fires whenever the axis enters its direction, modifier held or not. A rule
 * with a modifier fires only while that modifier is held (mod_down). A centered
 * axis (dir == 0) never fires. Caller has already resolved `dir`.
 *
 * This is the gate that lets modifier-less combos (e.g. a JVS gear lever mapped
 * straight to a button) work without a modifier, where the proxy previously
 * evaluated ABS rules only while the modifier was down.
 */
static inline bool rgp_abs_rule_fires(int nmods, unsigned int trig_code, int trig_dir,
                                      unsigned int code, int dir, bool mod_down)
{
  if (dir == 0 || trig_code != code || trig_dir != dir)
    return false;
  return nmods == 0 || mod_down;
}

#endif /* RECALBOX_GAMEPAD_PROXY_RGP_LOGIC_H */
