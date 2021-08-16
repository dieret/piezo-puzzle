# Piezo Puzzle

[![gh actions](https://github.com/dieret/piezo-puzzle/actions/workflows/build.yaml/badge.svg)](https://github.com/dieret/piezo-puzzle/actions)
[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/dieret/piezo-puzzle/main.svg)](https://results.pre-commit.ci/latest/github/dieret/piezo-puzzle/main)
[![License](https://img.shields.io/github/license/dieret/piezo-puzzle.svg)](https://github.com/dieret/piezo-puzzle/blob/main/LICENSE.txt)

An interactive birthday puzzle with a piezo buzzer on an atmega8 MCU.

## Description

## Hardware

* MCU: atmega8
* Input: Rotary switch with 16 positions [KMR 16](https://www.reichelt.de/dreh-codierschalter-16-polig-mit-vertikal-achse-kmr-16-p9434.html?&nbc=1)
* Output: Piezo element [SUMMER EPM 121](https://www.reichelt.de/piezo-schallwandler-85-db-4-khz-summer-epm-121-p35927.html?&nbc=1)
* Transistor [SC 1815](https://www.reichelt.de/bipolartransistor-npn-50v-0-15a-0-4w-to-92-sc-1815-p16334.html?&trstct=pos_0&nbc=1). **WARNING: unusual pin configuration**; check first page of data sheet
* Power supply: Directly via USB cable
    * 40cm cable with USB A female [DELOCK 83825](https://www.reichelt.de/usb-pinheader-buchse-auf-usb-a-buchse-40-cm-delock-83825-p163112.html?&nbc=1)
    * Adapter with USB A male <> micro USB A/B female [DELOCK 65029](https://www.reichelt.de/usb-micro-b-buchse-auf-usb-2-0-a-stecker-delock-65029-p160326.html?&nbc=1)
    * Adapter with USB A male <> USB A male [DELOCK 65011](https://www.reichelt.de/usb-a-stecker-auf-usb-a-stecker-delock-65011-p180114.html?&nbc=1)

### Form factor

Dimensions: Without axis and usb connection 44.5 x 48.35 x 11.5, Offset axis: 5.8

* breadboard thickness + soldering ~ 4mm
* IC + socket + breadboard + soldering ~ 11.5 mm
    * Piezo buzzer less high than that
    * **USB adapter should be put at 90°** to not increase height
* rotary switch: 6mm + 7.3mm (axis) + 4mm (breadboard + soldering)
* Breadboard: 44.5 x 48.35 mm (though usb connection protrudes)

## Compiling

You might need to

```bash
sudo apt-get install avr-libc
sudo apt-get install gcc-avr
```

Then run:

```bash
cmake -S .
cmake --build .
# To upload to the MCU
cmake --build . --target upload
```


## Development setup

```
sudo apt-get install clang-format
pre-commit install
```
