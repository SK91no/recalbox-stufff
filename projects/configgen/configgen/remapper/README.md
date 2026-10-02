# Controller input remapper

This module maps a **game + controller (+ optional cabinet)** to an input layout, so a
game designed for one control panel plays correctly on a different one — e.g. an arcade
or console game running on a **JVS cabinet** (steering wheel, 2 pedals, a gear lever and
START, with **no face/action buttons**), or on a plain arcade stick.

It is **emulator-agnostic**: consumed by the libretro generator and by the standalone
Supermodel (Model 3) generator, and it is **not tied to arcade** systems — it also drives
console games (dynamic per-state control switching, see below).

The data lives in **`remap-profiles.json`**; everything else here is the engine that
loads it, resolves a profile, and pushes the result to the **R.G.P.** (recalbox-gamepad-proxy)
kernel module, which rewrites input events *upstream of the emulator* (core-agnostic).


---

## Files

| File | Role |
|---|---|
| `remap-profiles.json` | **The data.** Per-system, per-game input profiles. |
| `RemapProfileStore.py` | Loads the JSON and `resolve()`s a profile for `(system, rom, controller_type, cab_type)`. |
| `Remapper.py` | Turns a resolved profile into a JVS button map + pushes combos to the R.G.P. Owns the RAM-watch daemon. |
| `RgpComboResolver.py` | Renders combos to the R.G.P. `combos` sysfs grammar. |
| `RgpSysfs.py` | Thin writer for `/sys/class/recalbox_gamepad_proxy/{combos,external_maps}`. |
| `SnesHeader.py` | Reads a SNES rom's internal header title, for header-based game identification. |
| `GameStateWatch.py` | Reads a game-state byte over RetroArch's UDP net-command interface and drives live menu↔race switching. |

Unit tests: `tests/generators/{remapProfileStore,remapper,rgpComboResolver,rgpSysfs,gameStateWatch,snesHeader}_test.py`.

---

## `remap-profiles.json` schema

Top level is keyed by **system** (`naomi`, `naomi2`, `atomiswave`, `fbneo`, `mame`,
`model3`, `snes`, …). `_comment` is documentation, ignored by the loader.

```jsonc
{
  "<system>": {
    "default": { "byController": [ … ] },          // optional, used when no game matches
    "<gameKey>": {
      "match":       ["shortname", "prefix", …],    // filename identification (see below)
      "matchHeader": ["INTERNAL TITLE"],            // optional: rom-header identification
      "byController": [                              // applies on ANY hardware, keyed on stick type
        { "types": ["arcade6","arcade8"], "remaps": { "l2":"b", "r2":"a" } }
      ],
      "jvs": {                                       // JVS-only, keyed on cab_type; WINS over byController
        "byCabType": [
          { "cabTypes": ["18_wheeler"] },            // empty entry = native, no rule
          { "cabTypes": ["race","crazy_taxi","jambo_safari"],
            "remaps": { "up":"a", "down":"b" },      // pad input -> emulator button
            "combos": [                              // modifier-gated or plain remaps
              { "modifier":"start", "trigger":"down", "emit":"north" }
            ]
          }
        ]
      }
    }
  }
}
```

### Resolution order (`RemapProfileStore.resolve`)
1. Find the game (`_match_game`); if none, use the system `default`.
2. On JVS, a matching `jvs.byCabType` entry **replaces** everything (empty entry = native).
3. Otherwise the `byController` layer, keyed on the stick type (`arcade4/6/8`).
4. Nothing matched → `{}` (apply nothing).

### `remaps`
`{ "<physical>": "<target>" }` — rewrites a control to another emulator button, 1:1.
The **key** is a pad input, in cardinal names: `up`, `down`, `left`, `right`, `start`,
`select`, `east`, `south`, `north`, `west`, `eastprime`, `northprime`, `l1`, `r1`, `l2`,
`r2`, `l3`, `r3` (`east`/`south`/`north`/`west` are the SNES A/B/X/Y, `eastprime` and
`northprime` the Megadrive C/Z). The **value** is an emulator button and keeps the
emulator's own vocabulary. On the crazy_taxi cab: wheel + pedals (`accel→r2`, `brake→l2`) +
gear lever (`up`/`down`) + `start`, and **no action buttons** — so extra functions are
reached via `start + lever` combos.

### `combos`
`{ "modifier?": "<name>", "trigger": "<name>", "emit": "<name>" }`. All three are pad
inputs, in the same cardinal names as a `remaps` key. A combo with a `modifier` is gated
by it (e.g. `start + down`); a combo **without** a modifier is a plain remap (mod-code 0)
that fires whenever the trigger moves. Mode is always `hold`.

---

## Game identification: filename vs rom header

`_match_game` tries, per entry:

1. **`matchHeader`** (if present *and* the rom path is available): read the rom's
   internal header title with `SnesHeader.title_from_file()` and compare. This is the
   **robust** identifier for console games on a cab — the internal title
   (e.g. `SUPER MARIO KART`) is burned into the cart by the publisher and is identical
   across every dump / region / revision, unlike a file hash (of which there are many)
   or the filename.
2. **`match`** (filename fallback): `rom in patterns` or `rom.startswith(p)`. For arcade
   systems the filename **is** the MAME shortname (stable), so `match` alone is enough
   and `matchHeader` is unused there.

`SnesHeader` handles `.zip`/`.7z`/raw, strips a 512-byte copier header, and tries the
LoROM (`0x7FC0`), HiROM (`0xFFC0`) and ExHiROM (`0x40FFC0`) slots, keeping the one with
a valid checksum (falling back to the most-printable title). It never raises — on any
failure it returns `None` and matching falls back to the filename.

---

## The R.G.P. combos sysfs grammar

`RgpComboResolver` renders each combo to a `;`-joined line written to
`/sys/class/recalbox_gamepad_proxy/combos`:

```
<target-pad-name>:<mod-code>:<trig-type>:<trig-code>:<trig-val>:<emit-code>:<mode>
```

* `mod-code` — EV_KEY code of the modifier, or **0** for a plain remap (no modifier).
* `trig-type` — `1` = EV_KEY, `3` = EV_ABS.
* `trig-code`/`trig-val` — for the lever: `ABS_HAT0Y` (17) with `-1` (up) / `+1` (down).
* `emit-code` — the emitted EV_KEY code, resolved from the **actual controller config**.
* `mode` — always `hold`.

Emit/trigger names resolve through the launched controller, so codes are pad-specific.
Reference for the JVS pad (`arcade6` layout): `a`=305, `b`=304, `x`=308, `y`=307,
`l1`=310, `r1`=311; lever `up`/`down` = `ABS_HAT0Y` ∓1; pedals `l2`=`ABS_Z`, `r2`=`ABS_RZ`.

> Note: a plain-remap (mod-code 0) **axis** combo only works with the R.G.P. driver's
> `nmods==0` ABS handler (v3.22+), which evaluates ABS rules even with no modifier held.

---

## Dynamic controls: RAM-watch state switching

A console racer on a JVS cab needs **different controls per game phase** (menu vs race).
A profile expresses this with `states` + `ramwatch` instead of a single `combos` list:

```jsonc
"smk": {
  "match": ["Super Mario Kart", "smk"],
  "matchHeader": ["SUPER MARIO KART"],
  "jvs": { "byCabType": [ {
    "cabTypes": ["race","crazy_taxi","jambo_safari"],
    "remaps": { "r2":"b", "l2":"y" },                       // static, both phases
    "ramwatch": { "busAddr":"7e0148", "sysramOffset":"148", "menuValue":0 },
    "states": {
      "menu": { "combos": [] },                             // native lever = menu nav
      "race": { "combos": [ {"trigger":"up","emit":"a"},    // lever up  -> A
                            {"trigger":"down","emit":"r1"} ] } // lever down -> R1 (hop)
    }
  } ] }
}
```

`Remapper._setup_state_watch` starts a **background thread** inside configgen (which
stays alive for the whole game, blocked in `runCommand`). `GameStateWatch.watch()`:

* Reads the state byte every ~40 ms via RetroArch's UDP net-command interface
  (`READ_CORE_RAM <offset> 1` — snes9x has no memory map, so `READ_CORE_MEMORY` is not
  used). `classify()`: `value == menuValue → menu`, else `race`.
* Debounces (`stable_frames`), then applies the state: pushes that phase's combos to
  the R.G.P. and flashes an OSD notice via the **native** `SHOW_MSG` net-command
  (`Recalbox JVS: switched controls to Menu/Race Mode`, overridable per-state via an
  `"osd"` field).

**Robustness (v3.22+ / self-healing daemon):**
1. The **menu (safe/native) state is pushed at daemon startup**, so stale combos from a
   previous game never linger before the first RAM read.
2. **Self-heal**: the current state's combos are re-pushed every ~2 s (without re-flashing
   the OSD), so a missed transition edge or externally-overwritten combos recover.
3. The net-command **socket is rebuilt** after ~1 s of failed reads (a socket that came
   up before RetroArch's interface did).

The daemon is stopped and the combos cleared by `Remapper.clear()` at game exit.

---

## How to add a game

1. Pick the system block; add an entry keyed by a short game key.
2. Identify it: `match` (filename/shortname) and/or `matchHeader` (console rom title).
3. On a JVS cab, add a `jvs.byCabType` entry for the relevant `cabTypes` with `remaps`
   and/or `combos`. Reserve pedal/lever remaps to `jvs.byCabType` (a plain stick keeps
   the emulator's default). For a phase-dependent console racer, use `states` + `ramwatch`.
4. Add/extend a test in `tests/generators/remapProfileStore_test.py`.
