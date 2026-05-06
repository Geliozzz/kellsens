# Kellsens

Ultra-low-power thermohygrometer firmware, built on Zephyr RTOS. Reads temperature and humidity, then renders the data on a 200×200 SSD1681 e-paper display — consuming near-zero power between updates thanks to e-paper's bistable nature and deep sleep between sensor readings.

## Features

- **E-paper display** — SSD1681 200×200, driven via MIPI DBI over SPI; retains image with zero power draw
- **Temperature & humidity** — periodic sensor reads with configurable interval
- **Ultra-low power** — deep sleep between cycles; display update only on value change
- **Zephyr RTOS v4.3.0** — portable, hardware-abstracted firmware

## Targets

| Board | Status |
|---|---|
| `nucleo_l433rc_p` (STM32L433) | Active |
| `nrf9151dk/nrf9151` | Experimental |
| `doit_esp32_devkit_v1` | Experimental |

## Getting Started

### Prerequisites

- [Zephyr SDK and `west`](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- ARM GCC toolchain (provided by Zephyr SDK)

### Setup

```sh
west init -l kellsens --mf west.yml
west update
west zephyr-export
```

### Build & Flash

```sh
# STM32 NUCLEO (primary target)
west build -b nucleo_l433rc_p -s app -d build
west flash -d build

# nRF9151 DK
west build -b nrf9151dk/nrf9151 -s app -d build
```

### Debug output

Serial console via UART. Use any terminal at the board's default baud rate to see `printk()` output.

## Project Structure

```
app/src/
  main.c      — Boot, deferred EPD init, main loop
  display.c   — SSD1681 command layer over MIPI DBI
app/boards/   — Per-board device tree overlays
prj.conf      — Zephyr Kconfig
west.yml      — Manifest (Zephyr v4.3.0)
```

## License

MIT — see [LICENSE](LICENSE).
