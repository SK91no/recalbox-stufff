## volumed

Volume control from the wheel.

This program listens for ABS_VOLUME event on an input device.
It will change default sink volume from the value of the event.

## Requirements

You will need PulseAudio to compile this software.

## Building

```
autoreconf -i
./configure && make && make install
```

## Usage

Run:

```
volumed /dev/input/inputX # adapt X to the desired wheel input device
```

Use the volume wheel to change audio output volume.

