# Recalbox Gamepad Proxy (R.G.P.)

## Architecture

```
┌─────────────────────┐     ┌─────────────────────┐
│  Physical Gamepad   │     │  External Device    │
│  (Xbox Controller)  │     │  (Foot Switch)      │
└──────────┬──────────┘     └──────────┬──────────┘
           │                           │
           ▼                           ▼
┌──────────────────────────────────────────────────┐
│              R.G.P. Kernel Module                │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐  │
│  │  Debounce  │  │   Combo    │  │  External  │  │
│  │   Filter   │  │   System   │  │  Mappings  │  │
│  └────────────┘  └────────────┘  └────────────┘  │
└──────────────────────┬───────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────┐
│           Virtual Proxy Device                  │
│         "Xbox Controller (R.G.P.)"              │
└─────────────────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────┐
│              Application / Emulator             │
└─────────────────────────────────────────────────┘
```

R.G.P. sits between physical input devices and applications. It intercepts gamepad events, applies transformations (combos, debouncing, external mappings), and emits them through a virtual proxy device. Applications see only the proxy, ensuring clean and enhanced input.

## Overview

R.G.P. is a Linux kernel module that intercepts gamepad input devices and creates virtual proxy devices with enhanced functionality. It was designed for the Recalbox project to provide seamless integration between arcade hardware and emulators.

**Version:** 3.19  
**Requires:** Linux kernel 6.12+  
**License:** GPL  
**Author:** @digitalLumberjack

## Features

- **Device Proxying**: Creates virtual gamepad devices that mirror physical ones
- **Combo System**: START/MODE + button combinations for special actions
- **Runtime Game Combos**: per-game `modifier + trigger → emit` rules pushed via sysfs, with trigger/modifier suppression (see below)
- **Exit Combo**: Long-press or button combinations to exit emulators (START+MODE)
- **Volume Control**: START + Up/Down for volume adjustment
- **Credit Insert**: START + SOUTH (A) to insert credits (sends SELECT)
- **External Source Mapping**: Map buttons from keyboards, foot switches, or other devices to gamepad buttons
- **Input Debouncing**: Configurable debounce for arcade microswitches
- **Device Grabbing**: Exclusive access to prevent duplicate inputs
- **Runtime Configuration**: All settings adjustable via sysfs without reload

## Installation

### Building the Module

```bash
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
sudo insmod recalbox_gamepad_proxy.ko
```

### Device Tree Overlay (Required for Activation)

The module only activates when a matching Device Tree overlay is loaded. Create a `.dts` file:

```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2835";

    fragment@0 {
        target-path = "/";
        __overlay__ {
            recalbox_gamepad_proxy {
                compatible = "recalbox,gamepad-proxy";
                status = "okay";

                /* Optional settings */
                grab;
                debounce-ms = <10>;
            };
        };
    };
};
```

Compile and load the overlay:

```bash
dtc -I dts -O dtb -o rgp.dtbo rgp-overlay.dts
sudo dtoverlay rgp.dtbo
```

## Configuration

### Sysfs Interface

All runtime configuration is available at `/sys/class/recalbox_gamepad_proxy/`:

| File | Type | Range | Description |
|------|------|-------|-------------|
| `version` | RO | - | Module version |
| `debug` | RW | 0-1 | Enable verbose logging |
| `do_grab` | RW | 0-1 | Grab devices exclusively |
| `combo_modifier` | RW | 0-2 | Combo key: 0=START, 1=MODE, 2=DISABLED |
| `start_longpress_ms` | RW | 0-30000 | Long-press duration for exit combo (0=disabled) |
| `stick_threshold` | RW | 0-32767 | Analog stick threshold for direction detection |
| `exit_combo_release_ms` | RW | 10-1000 | Duration to hold exit combo keys |
| `enable_thumbstick_exit` | RW | 0-1 | Enable L3+R3 as exit combo |
| `modifier_tap_delay_ms` | RW | 0-5000 | Delay for modifier tap-through |
| `debounce_ms` | RW | 0-500 | Input debounce delay |
| `enable_longpress_exit` | RW | 0-1 | Enable long-press exit combo |
| `enable_shift_mode_passthrough` | RW | 0-1 | Enable START+button → MODE+button |
| `enable_shift_volume` | RW | 0-1 | Enable START+Up/Down for volume |
| `enable_shift_credit` | RW | 0-1 | Enable START+A for credit (SELECT) |
| `device_filter` | RW | string | Filter devices by name (substring match) |
| `external_maps` | RW | string | External source mappings |
| `inverted_inputs` | RW | string | Input inversion rules |
| `combos` | RW | string | Runtime per-game combos (modifier + trigger → emit, see below) |

### Module Parameters

Parameters can also be set at module load time:

```bash
sudo insmod recalbox_gamepad_proxy.ko debug=1 do_grab=1 debounce_ms=10
```

### Device Tree Properties

| Property | Type | Description |
|----------|------|-------------|
| `debug` / `no-debug` | bool | Enable/disable debug logging |
| `grab` / `no-grab` | bool | Enable/disable device grabbing |
| `combo-modifier` | u32 | Combo modifier key (0=START, 1=MODE, 2=DISABLED) |
| `longpress-ms` | u32 | Long-press duration in milliseconds |
| `stick-threshold` | u32 | Analog stick threshold |
| `exit-combo-release-ms` | u32 | Exit combo hold duration |
| `thumbstick-exit` / `no-thumbstick-exit` | bool | Enable L3+R3 exit |
| `modifier-tap-delay-ms` | u32 | Modifier tap delay |
| `debounce-ms` | u32 | Debounce delay |
| `longpress-exit` / `no-longpress-exit` | bool | Enable long-press exit |
| `shift-mode-passthrough` / `no-shift-mode-passthrough` | bool | Enable mode passthrough |
| `shift-volume` / `no-shift-volume` | bool | Enable volume control |
| `shift-credit` / `no-shift-credit` | bool | Enable credit insertion |
| `device-filter` | string | Device name filter |

## Combo System

### Default Combos (with combo_modifier=0, START as modifier)

| Combo | Action | Notes |
|-------|--------|-------|
| START (tap) | Send START | Normal START button behavior |
| START (hold 4s) | EXIT (START+MODE) | Configurable via `start_longpress_ms` |
| START + A | INSERT CREDIT (SELECT) | Requires `enable_shift_credit=1` |
| START + UP | VOLUME UP | Requires `enable_shift_volume=1` |
| START + DOWN | VOLUME DOWN | Requires `enable_shift_volume=1` |
| START + any button | MODE + button | Requires `enable_shift_mode_passthrough=1` |
| L3 + R3 | EXIT (START+MODE) | Requires `enable_thumbstick_exit=1` |

### Combo Modifier Modes

- **0 (START)**: START button is the combo modifier
- **1 (MODE)**: MODE button is the combo modifier
- **2 (DISABLED)**: All combos disabled, pure passthrough

## External Source Mapping

Map buttons from external devices (keyboards, foot switches, other gamepads) to the proxied gamepad.

### Sysfs Configuration

```bash
echo "<mapping>" > /sys/class/recalbox_gamepad_proxy/external_maps
```

### Mapping Format

```
<source-name>:<src-type>:<src-code>:<dst-code>:<mode>[:<target-gamepad>[:<src-value>]]
```

| Field | Description |
|-------|-------------|
| `source-name` | Substring to match source device name |
| `src-type` | Event type (1 = EV_KEY) |
| `src-code` | Source key/button code |
| `dst-code` | Destination button code on proxy |
| `mode` | Behavior mode (see below) |
| `target-gamepad` | Optional: target proxy by name (substring) |
| `src-value` | Optional: specific value for tap mode |

### Modes

| Mode | Description |
|------|-------------|
| `hold` | Map press/release 1:1 |
| `tap` | Send tap on press only |
| `hold-replace-source` | Map + block original source event |
| `hold-replace-target` | Map + block target button on gamepad |
| `tap-replace-source` | Tap + block source |
| `tap-replace-target` | Tap + block target |
| `hold-invert` | Invert press/release logic |
| `tap-invert` | Invert tap trigger |

Modes can be combined: `hold-replace-target-invert`

### Examples

**Foot switch to BTN_EAST on first available proxy:**
```bash
echo "FootSwitch:1:48:305:hold" > /sys/class/recalbox_gamepad_proxy/external_maps
```

**Foot switch replacing BTN_EAST on a specific gamepad:**
```bash
echo "FootSwitch:1:48:305:hold-replace-target:Xbox Controller" > /sys/class/recalbox_gamepad_proxy/external_maps
```

**Keyboard key to gamepad button:**
```bash
echo "Keyboard:1:57:304:hold" > /sys/class/recalbox_gamepad_proxy/external_maps
```
This maps SPACE (57) to BTN_SOUTH (304).

**Multiple mappings (semicolon-separated):**
```bash
echo "FootSwitch:1:48:305:hold;Keyboard:1:30:304:tap" > /sys/class/recalbox_gamepad_proxy/external_maps
```

### Key Codes Reference

Common button codes:
- `BTN_SOUTH` (A): 304
- `BTN_EAST` (B): 305
- `BTN_NORTH` (X): 307
- `BTN_WEST` (Y): 308
- `BTN_TL` (L1): 310
- `BTN_TR` (R1): 311
- `BTN_TL2` (L2): 312
- `BTN_TR2` (R2): 313
- `BTN_SELECT`: 314
- `BTN_START`: 315
- `BTN_MODE`: 316
- `BTN_THUMBL` (L3): 317
- `BTN_THUMBR` (R3): 318

Common key codes:
- `KEY_A`: 30
- `KEY_B`: 48
- `KEY_SPACE`: 57
- `KEY_ENTER`: 28

Use `evtest` to find device names and key codes.

### Device Tree External Mappings

```dts
recalbox_gamepad_proxy {
    compatible = "recalbox,gamepad-proxy";
    status = "okay";

    external-mappings {
        footswitch-btn-east {
            source-name = "FootSwitch";
            source-type = <1>;  /* EV_KEY */
            source-code = <48>; /* KEY_B */
            target-code = <305>; /* BTN_EAST */
            /* Optional properties: */
            /* tap; */
            /* replace-source; */
            /* replace-target; */
            /* invert-logic; */
            /* target-gamepad-name = "Xbox Controller"; */
        };
    };
};
```

## Runtime Game Combos

The built-in combos (credit, volume, exit) are hard-wired. The `combos` sysfs
attribute lets **configgen push per-game combos at launch** — arbitrary
`modifier + trigger → emit` rules — and clear them on exit.

### Why this exists (division of labour)

Arcade/JVS input adaptation is split in two:

- **configgen owns all static remaps** (physical button → RetroArch button),
  written straight into the RetroArch config. RGP is *not* involved.
- **RGP owns the combos** — anything needing a modifier + live state +
  input suppression, which RetroArch's static config cannot express.

A JVS race cab (Crazy Taxi, generic `race`, …) has few digital inputs: a
gear lever (up/down), start, coin. A game like Initial D needs gear up, gear
down **and** a VIEW button. configgen maps the lever to the gears; RGP adds
`START + lever → VIEW`, and — this is the point — **suppresses the lever's gear
while START is held**, so START+lever is VIEW *only*, not VIEW+gear.

### How it works (reuses the shift-rule engine)

Combos are **not** a parallel additive layer. Each combo is parsed into a
runtime `rgp_rule` and evaluated in the **same firing loop as the static
credit/volume rules** (`rgp_process_{key,abs}_event_internal`). It therefore
inherits, for free:

- **modifier check** (`rgp_mods_down`) — the trigger only fires while the
  modifier key is held;
- **suppression** — the trigger is consumed (`remapped_down`, early return) and
  the modifier is swallowed (`mod_pending`/`mod_used`), exactly like
  `START + SOUTH → SELECT` emits *only* SELECT;
- **tap / hold** output semantics.

A combo with no modifier (`mod-code = 0`) is a plain remap **that still passes
through the engine** and gets suppression — but in practice configgen does
modifier-less remaps itself, so runtime combos always carry a modifier.

Because RGP produces the proxy vdev that RetroArch reads, suppressing the
trigger at the vdev level means configgen's own remap of that button simply
never sees it — no cross-layer conflict.

### Sysfs Configuration

```bash
echo "<combo>[;<combo>...]" > /sys/class/recalbox_gamepad_proxy/combos
# clear (on game exit):
echo "" > /sys/class/recalbox_gamepad_proxy/combos
```

### Combo Format

```
<target-name>:<mod-code>:<trig-type>:<trig-code>:<trig-val>:<emit-code>:<mode>
```

| Field | Description |
|-------|-------------|
| `target-name` | Substring of the proxied pad name (informational; currently applies to all) |
| `mod-code` | EV_KEY code of the modifier held to arm the combo (e.g. 315 = BTN_START) |
| `trig-type` | Trigger event type: `1` = EV_KEY, `3` = EV_ABS |
| `trig-code` | Trigger code (key code, or axis such as `1` = ABS_Y) |
| `trig-val` | EV_ABS: signed direction (`+1`/`-1`); EV_KEY: unused |
| `emit-code` | EV_KEY code emitted when the combo fires |
| `mode` | `hold` (follows the gate) or `tap` (pulse) |

EV_ABS triggers on dpad/hat axes are read as exact `±1`; sticks fall back to
`stick_threshold`. Both `tap` and `hold` outputs are supported for EV_ABS
triggers: a `hold` keeps the emitted key down until the axis leaves its
engaging direction (or the modifier is released), mimicking a human press.

### Example (Initial D VIEW on a race cab)

```bash
# START + lever-up (ABS_Y -1) -> tap BTN_NORTH, which configgen maps to VIEW
echo "RecalboxJVS Gamepad #1:315:3:1:-1:308:tap" > /sys/class/recalbox_gamepad_proxy/combos
```

Limits: up to 8 combos (`RGP_MAX_COMBOS`), 2048-char buffer. Cleared to the
static-rules-only baseline by writing an empty string.

## Input Inversion

Invert press/release logic for specific inputs (useful for active-low hardware).

### Sysfs Configuration

```bash
echo "<source-name>:<src-type>:<src-code>" > /sys/class/recalbox_gamepad_proxy/inverted_inputs
```

### Example

```bash
echo "Arcade Stick:1:304;Arcade Stick:1:305" > /sys/class/recalbox_gamepad_proxy/inverted_inputs
```

### Device Tree Configuration

```dts
recalbox_gamepad_proxy {
    compatible = "recalbox,gamepad-proxy";

    inverted-inputs {
        btn-south {
            source-name = "Arcade Stick";
            source-type = <1>;
            source-code = <304>;
        };
    };
};
```

## Debouncing

Arcade microswitches often bounce, causing multiple rapid press/release events. The debounce system filters these.

### Configuration

```bash
# Set 10ms debounce
echo 10 > /sys/class/recalbox_gamepad_proxy/debounce_ms

# Disable debounce
echo 0 > /sys/class/recalbox_gamepad_proxy/debounce_ms
```

### Behavior

1. **First event**: Emitted immediately, starts debounce window
2. **During window**: Changes are tracked but not emitted
3. **Window expires**: If state changed, emit the final value

Recommended values:
- Standard controllers: 0 (disabled)
- Arcade joysticks/buttons: 5-15ms
- Very bouncy switches: 20-30ms

## Device Filtering

Limit which devices R.G.P. proxies:

```bash
# Only proxy devices with "Xbox" in their name
echo "Xbox" > /sys/class/recalbox_gamepad_proxy/device_filter

# Clear filter (proxy all gamepads)
echo "" > /sys/class/recalbox_gamepad_proxy/device_filter
```

## Troubleshooting

### Enable Debug Logging

```bash
echo 1 > /sys/class/recalbox_gamepad_proxy/debug
dmesg -w
```

### Check Proxied Devices

```bash
# List all input devices
evtest

# The proxy adds "(R.G.P.)" to device names
# Example: "Xbox Controller (R.G.P.)"
```

### Verify Device is Grabbed

```bash
# Check for rgp_proxied attribute
cat /sys/class/input/event*/device/rgp_proxied
```

### Common Issues

**Events not reaching applications:**
- Check `do_grab=1` is set
- Ensure application listens to the proxy device, not the original

**Combos not working:**
- Verify `combo_modifier` is not 2 (disabled)
- Check `enable_shift_*` settings

**External mappings not working:**
- Verify source device name with `evtest`
- Check key codes are correct
- Ensure the proxy device is created before adding mappings

**Stuck buttons:**
- Increase `debounce_ms` value
- Check for hardware issues

### Module Not Activating

The module requires a Device Tree overlay with `compatible = "recalbox,gamepad-proxy"`. Without it, the module loads but remains dormant.

Check dmesg for:
```
recalbox_gamepad_proxy: platform driver registered, waiting for DT match
```

If you see this but no "handlers registered", the overlay is not loaded.

## Unit Tests

The pure input logic (mode/binding/combo parsers and the binding matcher) lives
in `rgp_logic.h`, guarded by `#ifdef __KERNEL__` so it compiles both into the
module and into a host test harness. `test/test_rgp_logic.c` characterizes the
current behavior (`-Wall -Wextra -Werror`).

```bash
make -C test        # builds and runs the host tests
```

These tests run automatically at package build time (the `.mk` invokes
`make -C test CC="$(HOSTCC)" run` before compiling the module), so a logic
regression fails the build.

## Version History

- **v3.19**: Runtime per-game `combos` sysfs (modifier + trigger → emit) reusing
  the shift-rule engine; pure logic extracted to `rgp_logic.h` with host tests
  wired into the package build
- **v3.13**: External source mapping fixes, debug improvements
- **v3.12**: External source mappings, input inversion support
- **v3.7**: Fixed debounce return-to-neutral bug
- **v3.6**: Device Tree conditional activation
- **v2.5**: Sysfs runtime configuration, initial debounce support

## License

This module is licensed under the GPL (GNU General Public License).