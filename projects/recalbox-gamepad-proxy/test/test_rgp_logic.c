/*
 * Host characterization tests for the pure RGP mapping logic (rgp_logic.h).
 * Goal: pin the CURRENT behavior before/around the combos refactor so no
 * feature is silently lost. Build & run: `make` in this directory.
 */
#define _GNU_SOURCE
#include "../rgp_logic.h"
#include <stdio.h>
#include <string.h>

static int fails;

#define CHECK(cond, what) do {                                        \
  if (!(cond)) {                                                      \
    fprintf(stderr, "FAIL %s:%d  %s\n", __func__, __LINE__, what);    \
    fails++;                                                          \
  }                                                                  \
} while (0)

/* strsep() mutates its input, so parse tests need a writable copy. */
static int parse(const char *s, struct rgp_external_binding *b)
{
  char buf[256];
  strncpy(buf, s, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  return rgp_parse_external_binding_string(buf, b);
}

static void test_name_matches(void)
{
  CHECK(!rgp_name_matches(NULL, "x"), "null name never matches");
  CHECK(!rgp_name_matches("x", NULL), "null needle never matches");
  CHECK(!rgp_name_matches("RecalboxJVS Gamepad #1", ""), "empty needle never matches");
  CHECK(rgp_name_matches("RecalboxJVS Gamepad #1", "JVS"), "substring hit");
  CHECK(rgp_name_matches("RecalboxJVS Gamepad #1", "RecalboxJVS Gamepad #1"), "exact match");
  CHECK(!rgp_name_matches("RecalboxJVS Gamepad #1", "Xbox"), "miss");
}

static void test_parse_mode(void)
{
  struct rgp_external_binding b;

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("1", &b) == 0 && b.dst_tap, "\"1\" -> tap");

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("0", &b) == 0 && !b.dst_tap, "\"0\" -> hold");

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("tap", &b) == 0 && b.dst_tap, "\"tap\" -> tap");

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("hold", &b) == 0 && !b.dst_tap, "\"hold\" -> hold");

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("", &b) == -EINVAL, "empty mode invalid");

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("holdtap", &b) == -EINVAL, "both tap+hold invalid");

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("hold-replace-source", &b) == 0 && b.replace_source && !b.replace_target,
        "replace-source flag");

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("hold-replace-target", &b) == 0 && b.replace_target && !b.replace_source,
        "replace-target flag");

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("hold-replace", &b) == 0 && b.replace_source && !b.replace_target,
        "bare replace defaults to source");

  memset(&b, 0, sizeof(b));
  CHECK(rgp_parse_binding_mode("tap-invert", &b) == 0 && b.dst_tap && b.invert_logic,
        "tap-invert sets both");
}

static void test_parse_binding_string(void)
{
  struct rgp_external_binding b;

  /* minimal 5 fields: name:type:src:dst:mode */
  CHECK(parse("PadA:1:307:308:hold", &b) == 0, "minimal valid parses");
  CHECK(!strcmp(b.source_name, "PadA"), "source_name");
  CHECK(b.src_type == EV_KEY && b.src_code == 307 && b.dst_code == 308, "codes");
  CHECK(!b.dst_tap, "hold mode");
  CHECK(b.src_value == -1, "src_value default -1");
  CHECK(b.target_gamepad_name[0] == '\0', "no target by default");

  CHECK(parse("PadA:1:307:308", &b) == -EINVAL, "under 5 fields invalid");
  CHECK(parse(":1:307:308:hold", &b) == -EINVAL, "empty source invalid");
  CHECK(parse("PadA:3:307:308:hold", &b) == -EOPNOTSUPP, "EV_ABS source unsupported");
  CHECK(parse("PadA:x:307:308:hold", &b) == -EINVAL, "bad type number invalid");

  /* 6th field = target gamepad name */
  CHECK(parse("PadA:1:307:308:tap:RecalboxJVS Gamepad #1", &b) == 0, "6 fields parse");
  CHECK(!strcmp(b.target_gamepad_name, "RecalboxJVS Gamepad #1"), "target captured");

  /* 7th field = src_value */
  CHECK(parse("PadA:1:307:308:tap:Target:0", &b) == 0 && b.src_value == 0, "src_value 7th field");
}

static void test_matches_event(void)
{
  struct rgp_external_binding b;

  /* hold: fires on any 0/1 edge, ignores autorepeat */
  parse("PadA:1:307:308:hold", &b);
  CHECK(rgp_binding_matches_event(&b, EV_KEY, 307, 1), "hold matches press");
  CHECK(rgp_binding_matches_event(&b, EV_KEY, 307, 0), "hold matches release");
  CHECK(!rgp_binding_matches_event(&b, EV_KEY, 307, 2), "autorepeat ignored");
  CHECK(!rgp_binding_matches_event(&b, EV_KEY, 999, 1), "wrong code no match");
  CHECK(!rgp_binding_matches_event(&b, EV_ABS, 307, 1), "wrong type no match");

  /* tap: default fires on press only */
  parse("PadA:1:307:308:tap", &b);
  CHECK(rgp_binding_matches_event(&b, EV_KEY, 307, 1), "tap matches press");
  CHECK(!rgp_binding_matches_event(&b, EV_KEY, 307, 0), "tap ignores release");

  /* tap + src_value 0: fires on release */
  parse("PadA:1:307:308:tap:Target:0", &b);
  CHECK(rgp_binding_matches_event(&b, EV_KEY, 307, 0), "tap src_value 0 fires on release");
  CHECK(!rgp_binding_matches_event(&b, EV_KEY, 307, 1), "tap src_value 0 ignores press");

  /* tap + invert: fires on release (inverted press) */
  parse("PadA:1:307:308:tap-invert", &b);
  CHECK(rgp_binding_matches_event(&b, EV_KEY, 307, 0), "tap-invert fires on release");
  CHECK(!rgp_binding_matches_event(&b, EV_KEY, 307, 1), "tap-invert ignores press");

  /*
   * hold matches BOTH edges regardless of invert: hold mode must track press
   * and release to follow the level. invert_logic is applied at EMIT time
   * (rgp_process_one_binding flips mapped_value), not in this matcher.
   */
  parse("PadA:1:307:308:hold-invert", &b);
  CHECK(b.invert_logic, "hold-invert sets the flag");
  CHECK(rgp_binding_matches_event(&b, EV_KEY, 307, 1), "hold matches press (level inverted at emit)");
  CHECK(rgp_binding_matches_event(&b, EV_KEY, 307, 0), "hold matches release (level inverted at emit)");
}

static int parse_combo(const char *s, struct rgp_game_combo *c)
{
  char buf[256];
  strncpy(buf, s, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  return rgp_parse_game_combo_string(buf, c);
}

static void test_parse_game_combo(void)
{
  struct rgp_game_combo c;

  /* the exact string configgen's RgpComboResolver emits for start+down->Y hold */
  CHECK(parse_combo("RecalboxJVS Gamepad #1:315:3:1:1:308:hold", &c) == 0, "abs combo parses");
  CHECK(!strcmp(c.target_gamepad_name, "RecalboxJVS Gamepad #1"), "target");
  CHECK(c.mod_code == 315, "mod_code");
  CHECK(c.trig_type == EV_ABS && c.trig_code == 1 && c.trig_val == 1, "trigger abs +1");
  CHECK(c.emit_code == 308 && !c.emit_tap, "emit hold");

  CHECK(parse_combo("T:315:3:1:-1:304:hold", &c) == 0 && c.trig_val == -1, "negative direction");
  CHECK(parse_combo("T:315:1:305:1:308:tap", &c) == 0 && c.trig_type == EV_KEY && c.emit_tap,
        "key trigger, tap");

  CHECK(parse_combo("T:315:3:1:1:308", &c) == -EINVAL, "under 7 fields invalid");
  CHECK(parse_combo(":315:3:1:1:308:hold", &c) == -EINVAL, "empty target invalid");
  CHECK(parse_combo("T:315:2:1:1:308:hold", &c) == -EOPNOTSUPP, "EV_REL trigger unsupported");
  CHECK(parse_combo("T:315:3:1:1:308:wat", &c) == -EINVAL, "bad mode invalid");
}

static void test_abs_forward_dir(void)
{
  /* hat: exact -1/0/+1, threshold ignored */
  CHECK(rgp_abs_forward_dir(-1, true, 0, 12000) == -1, "hat -1 -> -1");
  CHECK(rgp_abs_forward_dir(1, true, 0, 12000) == 1, "hat +1 -> +1");
  CHECK(rgp_abs_forward_dir(0, true, 0, 12000) == 0, "hat 0 -> 0");

  /* analog: rule threshold when >0 */
  CHECK(rgp_abs_forward_dir(-20000, false, 12000, 8000) == -1, "analog below -thr -> -1");
  CHECK(rgp_abs_forward_dir(20000, false, 12000, 8000) == 1, "analog above +thr -> +1");
  CHECK(rgp_abs_forward_dir(5000, false, 12000, 8000) == 0, "analog within thr -> 0");

  /* analog: falls back to stick_threshold when rule thr <= 0 */
  CHECK(rgp_abs_forward_dir(-9000, false, 0, 8000) == -1, "analog uses stick_threshold");
  CHECK(rgp_abs_forward_dir(7000, false, 0, 8000) == 0, "analog under stick_threshold -> 0");

  /* the bug this fixes: a held value must NOT map to 0 (which would desync
   * dir-state and leave the key stuck when the modifier is later pressed) */
  CHECK(rgp_abs_forward_dir(-1, true, 0, 12000) != 0, "held hat is not tracked as neutral");
}

static void test_abs_rule_fires(void)
{
  /* ABS_HAT0Y = 17, ABS_Z = 2 (raw evdev codes, as elsewhere in these tests) */
  /* nmods == 0 (plain remap): fires with the modifier down OR not */
  CHECK(rgp_abs_rule_fires(0, 17, +1, 17, +1, false), "nmods0 fires without modifier");
  CHECK(rgp_abs_rule_fires(0, 17, +1, 17, +1, true),  "nmods0 fires with modifier");
  CHECK(rgp_abs_rule_fires(0, 17, -1, 17, -1, false), "nmods0 other direction, no modifier");

  /* rule with a modifier: only while it is held */
  CHECK(!rgp_abs_rule_fires(1, 17, +1, 17, +1, false), "mod rule does not fire without modifier");
  CHECK(rgp_abs_rule_fires(1, 17, +1, 17, +1, true),   "mod rule fires with modifier");

  /* code / direction / centered gating */
  CHECK(!rgp_abs_rule_fires(0, 17, +1, 17, -1, false), "wrong direction does not fire");
  CHECK(!rgp_abs_rule_fires(0, 17, +1, 2,  +1, false), "wrong axis code does not fire");
  CHECK(!rgp_abs_rule_fires(0, 17, +1, 17, 0,  false), "centered axis never fires");
  CHECK(!rgp_abs_rule_fires(0, 17, +1, 17, 0,  true),  "centered axis never fires (mod held)");
}

int main(void)
{
  test_name_matches();
  test_parse_mode();
  test_parse_binding_string();
  test_matches_event();
  test_parse_game_combo();
  test_abs_forward_dir();
  test_abs_rule_fires();

  if (fails == 0) {
    printf("All rgp_logic tests passed.\n");
    return 0;
  }
  fprintf(stderr, "%d rgp_logic test(s) failed.\n", fails);
  return 1;
}
