# Piezo Puzzle

[![gh actions](https://github.com/dieret/piezo-puzzle/actions/workflows/build.yaml/badge.svg)](https://github.com/dieret/piezo-puzzle/actions)
[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/dieret/piezo-puzzle/main.svg)](https://results.pre-commit.ci/latest/github/dieret/piezo-puzzle/main)
[![License](https://img.shields.io/github/license/dieret/piezo-puzzle.svg)](https://github.com/klieret/piezo-puzzle/blob/main/LICENSE.txt)

An interactive birthday puzzle with a piezo buzzer on an atmega8 MCU.

## Build

You might need to

```bash
sudo apt-get install avr-libc
sudo apt-get install gcc-avr
```

```bash
cmake -S .
cmake --build .
cmake --build . --target upload
```


## Development setup

```
sudo apt-get install clang-format
pre-commit install
```
