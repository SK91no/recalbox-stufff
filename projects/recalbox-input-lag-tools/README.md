# Controller Latency Tester

Measures the end-to-end latency of game controllers on Raspberry Pi 5 by
toggling a GPIO output (wired to a button input on the controller) and
measuring how long it takes for the corresponding `evdev` event to arrive
in userspace.

## Build

```bash
sudo apt install libgpiod-dev
make
```

## Wiring

Use a 2N2222 NPN transistor to drive the controller button input.
This works with both 3.3V and 5V controller logic and protects the
RPi5 GPIO from any back-feeding.

```
                         Controller
                        ┌──────────┐
                   ┌────┤ BTN sig  │ (internal pull-up to Vcc)
                   │    │          │
                   │    ├ BTN GND ─┼─── GND
                   │    └──────────┘
                   │
                 ┌─┤ C
                 │ │
  RPi5 GPIO ──[R]──┤ B    2N2222 (NPN)
  (pin 17)   1kΩ  │
                 │ │
                 └─┤ E
                   │
                  GND
```

**How it works:** when the GPIO goes HIGH, the transistor saturates and
shorts the controller button pin to GND — exactly like a physical button
press. When the GPIO goes LOW, the transistor opens and the button is
released.

- **R (base resistor):** 1kΩ is fine for 3.3V GPIO → provides ~2.5mA
  base current, plenty to saturate for typical button pull-up currents.
- **Active-low by nature:** the transistor pulls to GND, so use the `-a`
  flag if the controller reports a press on the falling edge.
- Works with any controller voltage (3.3V, 5V, 12V) since the
  collector side is independent from the RPi GPIO.

## Usage

```bash
# Basic usage - auto-detects keycode
sudo ./controller_latency -d /dev/input/event0 -p 17

# Specify keycode, 200 samples, active-low
sudo ./controller_latency -d /dev/input/event0 -p 17 -k 304 -n 200 -a

# List available input devices
cat /proc/bus/input/devices
```

## Options

| Flag | Description | Default |
|------|-------------|---------|
| `-g` | GPIO chip name | `gpiochip4` (RP1) |
| `-p` | GPIO pin number | `17` |
| `-d` | evdev device path | *(required)* |
| `-k` | EV_KEY keycode | auto-detect |
| `-n` | Number of samples | `100` |
| `-t` | Timeout per event (ms) | `200` |
| `-a` | Active-low GPIO | off |

## What it measures

The tool measures the time between:
1. **GPIO set** (`clock_gettime` right before `gpiod_line_set_value`)
2. **evdev event timestamp** (kernel `input_event.time`)

This captures: hardware debounce + controller firmware + USB polling
interval + USB transfer + kernel HID processing + evdev delivery.

## Typical results

| Controller type | Expected latency |
|----------------|-----------------|
| USB HID 1000Hz | ~1-4 ms |
| USB HID 125Hz  | ~4-12 ms |
| JVS via ESP32  | ~2-8 ms |
| Bluetooth      | ~10-30 ms |