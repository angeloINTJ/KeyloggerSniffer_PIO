# KeyloggerSniffer_PIO

**Passive USB keyboard sniffer for the Raspberry Pi Pico, with encrypted flash storage and multi-language CLI.**

The entire USB capture — D+/D- signal sampling, packet decoding, and HID report filtering — is offloaded to the RP2040's PIO coprocessor. Keystrokes are encrypted with XTEA-CTR and persisted to a 1 MB circular flash log with CRC integrity protection. A serial command interface provides password-protected data access, real-time streaming, and CSV export.

> **⚠️ Legal Disclaimer**
>
> This project is provided **strictly for educational purposes, authorized security research, and legitimate hardware auditing**. It is intended to help security professionals, researchers, and enthusiasts understand USB HID protocol internals, PIO-based signal capture, and embedded cryptography.
>
> **You are solely responsible for ensuring that your use of this software complies with all applicable laws in your jurisdiction.** Intercepting, recording, or monitoring keyboard input from devices you do not own — or without the explicit, informed consent of the device owner — is **illegal** in most jurisdictions and may violate statutes including, but not limited to:
>
> * **Brazil** — Lei 12.737/2012, Art. 154-A (invasão de dispositivo informático)
> * **United States** — Computer Fraud and Abuse Act (18 U.S.C. § 1030), Wiretap Act (18 U.S.C. § 2511)
> * **European Union** — GDPR (Art. 5, 6), national implementations of the Computer Misuse Directive
> * **United Kingdom** — Computer Misuse Act 1990
>
> The author and contributors **do not condone, encourage, or support** the use of this tool for unauthorized surveillance, data theft, or any form of illegal activity. By using, downloading, or distributing this software, you acknowledge that:
>
> 1. You will only use it on devices you own or have written authorization to test
> 2. You assume all legal liability arising from your use
> 3. The MIT License governs copyright and redistribution only — it does not grant permission to violate any law
>
> **If you are unsure whether your intended use is lawful, consult a qualified legal professional before proceeding.**

---

## Features

* **PIO-accelerated USB capture** — passive high-impedance sampling of D+/D- via the PIO coprocessor; invisible to the host and device
* **Dual-core architecture** — Core 1 runs the sniffer at 120 MHz; Core 0 handles encryption, flash storage, and serial CLI
* **XTEA-CTR encryption** — every flash page is encrypted with a random 128-bit DEK wrapped by a PBKDF2-derived KEK
* **Board-locked keys** — the KDF mixes the RP2040's unique chip ID into the salt, binding decryption to the physical hardware
* **Password protection** — optional user password with brute-force lockout (progressive delay up to 30 s)
* **Read PIN** — optional 4-digit PIN (KDF-hashed) for casual access control without full password
* **1 MB circular log** — ~254,000 events with automatic wraparound, CRC-16 per page, crash-safe metadata journal
* **4 bytes/keystroke** — compact PackedEvent format (scancode + modifiers + delta timestamp)
* **Multi-keyboard support** — tracks up to 4 USB keyboards simultaneously with LRU eviction
* **10 keyboard layouts** — US, UK, DE, FR, ES, IT, PT, ABNT2, Nordic, JP (runtime-switchable)
* **10 interface languages** — EN, PT-BR, ES, FR, DE, IT, JA, ZH, KO, RU (runtime-switchable, persisted)
* **Real-time streaming** — `LIVE` mode prints decoded keystrokes as they arrive; `LIVE CLEAN` suppresses modifier tags
* **CSV export** — timestamped, filterable (`EXPORT last_N`), with session markers and optional Unix epoch anchoring
* **Firmware integrity** — CRC-32 of firmware region verified on every boot; mismatch warns of reflash/tampering
* **Watchdog + heartbeat** — 8 s watchdog on Core 0; Core 1 heartbeat monitored with 500 ms pause timeout
* **Flash wear protection** — metadata persist interval tuned for >2 year endurance under continuous typing

## Hardware Requirements

| Component | Description |
| --- | --- |
| Raspberry Pi Pico | RP2040-based board (2 MB flash) |
| USB keyboard | Any standard USB HID keyboard |
| 100 Ω resistors (×2) | Series protection on D+/D- lines |

### Wiring Diagram

```
Pico GP16 ── 100Ω ── USB D+ (green wire)
Pico GP17 ── 100Ω ── USB D- (white wire)
Pico GND  ────────── USB GND (black wire)

Do NOT connect VBUS (5 V red wire) to the Pico.
```

The sniffer taps the USB bus passively — no signal injection, no enumeration, no VBUS power.

## Installation

### PlatformIO (recommended)

```ini
[env:pico]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board = pico
framework = arduino
board_build.core = earlephilhower
board_build.f_cpu = 120000000L
monitor_speed = 115200
lib_deps =
    USBSnifferPIO_RP2040
    adafruit/Adafruit NeoPixel@^1.12.0
build_flags = -O2 -Wall -DPICO_FLASH_SIZE_BYTES=2097152
upload_protocol = picotool
```

### Arduino IDE

1. Install the [arduino-pico](https://github.com/earlephilhower/arduino-pico) core (Earle Philhower)
2. Select board: **Raspberry Pi Pico**
3. Install libraries: **USBSnifferPIO_RP2040**, **Adafruit NeoPixel**
4. Run `python3 generate_build_seed.py` once before the first build
5. Compile and upload

### First Boot

On first power-up, the system auto-generates a random encryption key and prints a build password on Serial. This password is derived from the board's unique chip ID and cannot be recovered from the firmware binary alone.

## Serial Commands

### Open Mode (default — no password)

| Command | Description |
| --- | --- |
| `DUMP` | Decrypt and display all events as text |
| `HEX` | Decrypt and display as hex |
| `EXPORT [N]` | CSV export (last N events optional) |
| `RAWDUMP` | Raw encrypted pages for offline decryption |
| `LOCK <pwd>` | Set password (enables locked mode) |
| `RPIN SET <pin>` | Set 4-digit read PIN |
| `RPIN CLEAR <pin>` | Remove read PIN |
| `LAYOUT [name]` | Show or change keyboard layout |
| `LANG [code]` | Show or change interface language |
| `LIVE [CLEAN]` | Toggle real-time keystroke streaming |
| `TIME <epoch>` | Set Unix timestamp for CSV anchoring |
| `DEL CONFIRM` | Erase all data and reset keys |
| `STAT` | System statistics and diagnostics |
| `HELP` | Show available commands |
| `STOP` | Cancel an active dump/export operation |

### Locked Mode (after `LOCK <pwd>`)

All data commands require the password as an argument (e.g. `DUMP <pwd>`).  Additional commands:

| Command | Description |
| --- | --- |
| `LOGIN <pwd>` | Unlock the system for capture |
| `PASS <old> <new>` | Change password (instant key re-wrap) |
| `UNLOCK <pwd>` | Remove password and return to open mode |

## Security Model

```
  build_seed + board_id ──► KDF ──► build_password
                                        │
  user_password + salt + board_id ──► PBKDF2 (12000 rounds)
                                        │
                                   ┌────┴────┐
                                   │   KEK   │ (128-bit)
                                   └────┬────┘
                                        │
                              XTEA-ECB encrypt
                                        │
                                   ┌────┴────┐
                                   │   DEK   │ (128-bit, random)
                                   └────┬────┘
                                        │
                              XTEA-CTR per page
                                        │
                                   Flash Log
```

* **DEK** is generated from hardware RNG (ROSC + von Neumann debiasing + timer jitter) and never touches flash in plaintext
* **KEK** is derived from the password via PBKDF2 with a dual CBC-MAC PRF, producing 128 bits per iteration
* **Board ID** (8 bytes, unique per chip) is mixed into the KDF salt, binding the key to the physical hardware
* **Nonce floor** advances with each metadata persist, detecting flash rollback attacks
* **Login lockout** enforces progressive delays after 3 failed attempts (2 s → 4 s → 8 s → ... → 30 s)

## Architecture

```
┌─────────────────────┐    ┌──────────────────────┐
│   Core 1 (Sniffer)  │    │   Core 0 (Storage)   │
│                     │    │                      │
│ PIO + DMA capture   │    │ Ring buffer consumer │
│ HID report filter   │    │ XTEA-CTR encryption  │
│ Multi-keyboard diff │    │ Flash circular log   │
│ LED status tracking │    │ Metadata journal     │
│ Heartbeat timestamp │    │ Serial CLI + i18n    │
│                     │    │ Watchdog timer       │
└────────┬────────────┘    └──────────┬───────────┘
         │    Ring Buffer (512 slots) │
         └────────────────────────────┘
```

## Flash Memory Layout

```
┌──────────────────────┐  0x10000000 (XIP_BASE)
│  Firmware (.text)    │
├──────────────────────┤  FLASH_CRYPTO_START
│  Crypto Sector (4K)  │  Key envelope + salt
├──────────────────────┤  FLASH_META_START
│  Metadata Sector (4K)│  Crash-safe state journal
├──────────────────────┤  FLASH_LOG_START
│  Event Log (1 MB)    │  Encrypted circular pages
└──────────────────────┘  End of 2 MB flash
```

## LED Status (NeoPixel on GP25)

| Color | Meaning |
| --- | --- |
| Green | Capturing (unlocked) |
| Orange | Booting / awaiting LOGIN |
| Blue | Text dump in progress |
| Cyan | Hex dump in progress |
| Red | Flash write |
| Magenta | Flash erase |
| White | Crypto operation |
| Yellow | Core 1 handshake timeout |

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting a pull request.

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.

## Acknowledgments

* [USBSnifferPIO_RP2040](https://github.com/angeloINTJ/USBSnifferPIO_RP2040) for the PIO-based USB packet capture engine
* [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) for LED status feedback
* [arduino-pico](https://github.com/earlephilhower/arduino-pico) for the RP2040 Arduino core
* Raspberry Pi Foundation for the RP2040 PIO architecture

## See Also

* [DHT22PIO_RP2040](https://github.com/angeloINTJ/DHT22PIO_RP2040) — PIO-accelerated DHT22 library by the same author
* [OneWirePIO_RP2040](https://github.com/angeloINTJ/OneWirePIO_RP2040) — PIO-accelerated DS18B20 library by the same author
