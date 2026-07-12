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

The Arduino IDE is a fully supported alternative to PlatformIO. Follow the steps below to set up your environment, configure the board, and flash the firmware.

> **Important:** This project **requires the Earle Philhower arduino-pico core** — the official Arduino Mbed OS core for RP2040 does **not** support PIO (Programmable I/O) peripherals. Do not install the "Arduino Mbed OS RP2040 Boards" package.

#### 1. Install Arduino IDE

Download and install **Arduino IDE 2.x** (recommended) from [arduino.cc](https://www.arduino.cc/en/software). Version 1.8.x also works, but the screenshots and menus reference the 2.x UI.

#### 2. Add the arduino-pico Board Package

1. Open Arduino IDE and go to **File → Preferences** (or **Arduino IDE → Settings** on macOS).
2. In the **Additional Boards Manager URLs** field, paste the following URL:
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
   If you already have other URLs in this field, click the icon on the right and add the URL on a new line.
3. Click **OK** to close Preferences.

#### 3. Install the RP2040 Core

1. Open **Tools → Board → Boards Manager** (or click the board icon in the left sidebar).
2. Type **"Raspberry Pi Pico"** in the search box.
3. Find **"Raspberry Pi Pico/RP2040" by Earle F. Philhower, III** — verify the author name carefully.
4. Click **Install** (latest version recommended).
5. Wait for the installation to complete — it downloads the compiler toolchain (~200 MB).

#### 4. Select and Configure the Board

1. Go to **Tools → Board → Raspberry Pi Pico/RP2040** and select **Raspberry Pi Pico**.
2. Now configure the following board settings in the **Tools** menu. **Bold** values are required; others can be left at default:

| Setting | Value | Notes |
|---|---|---|
| **Board** | Raspberry Pi Pico | |
| **CPU Speed** | **120 MHz** | Required for stable PIO USB sniffing |
| **Flash Size** | **2MB** | Must match `-DPICO_FLASH_SIZE_BYTES=2097152` |
| USB Stack | Pico SDK | Default — works correctly |
| Debug Level | None | Use "USB (HWCDC)" for serial-only monitoring |
| Optimize | **Optimize (-O2)** | Important for PIO timing deadlines |
| C++ Exceptions | Disabled | Keeps binary size small |
| IP/Bluetooth Stack | IPv4 Only | Not used by the sniffer |
| Debug Port | Disabled | |

> ⚠️ If **CPU Speed** is left at the default 133 MHz, the PIO state machine timing will be off and USB capture will produce malformed packets. **120 MHz** is mandatory.

#### 5. Install Required Libraries

1. Open **Tools → Manage Libraries** (or the library icon in the left sidebar).
2. Install each of these libraries by searching and clicking **Install**:

| Library | Author | Version | Purpose |
|---|---|---|---|
| **USBSnifferPIO_RP2040** | angeloINTJ | latest | PIO-based USB D+/D- signal capture engine |
| **Adafruit NeoPixel** | Adafruit | ≥ 1.12.0 | Status LED feedback on GP25 |

3. Close the Library Manager when done.

#### 6. Open the Project

1. **Clone or download** this repository to your computer:
   ```bash
   git clone https://github.com/angeloINTJ/KeyloggerSniffer_PIO.git
   ```
2. In Arduino IDE, go to **File → Open** and select `KeyloggerSniffer_PIO.ino` from the cloned directory.
3. Arduino IDE will open the `.ino` file and automatically pull in all companion files (`.cpp`, `.h`) from the same folder. You should see multiple tabs in the IDE — the main sketch plus `CryptoEngine.cpp`, `SnifferWorker.cpp`, `KeylogConfig.h`, and the keyboard layout headers.

> **Note:** All project files must remain in the same directory as `KeyloggerSniffer_PIO.ino`. Arduino IDE compiles everything from that folder — do not move individual files elsewhere.

#### 7. Generate the Build Seed (one-time)

Before the first build, you must generate a cryptographic seed file. This creates `BuildSeed.h` with a random 16-byte XOR-obfuscated seed used to derive the initial encryption password.

Open a terminal in the project directory and run:

```bash
python3 generate_build_seed.py
```

You should see confirmation that `BuildSeed.h` was written. Run this command **once per installation** — if you re-run it, all previously encrypted flash data will become unreadable (a new key pair is generated).

> If you don't have Python 3 installed, install it from [python.org](https://www.python.org/) or your system package manager (`sudo apt install python3` on Debian/Ubuntu).

#### 8. Connect the Pico in BOOTSEL Mode

1. **Disconnect** the Pico from USB if already connected.
2. **Hold down the BOOTSEL button** (the white button on the Pico board).
3. While holding BOOTSEL, **connect the Pico to your computer** via USB.
4. **Release BOOTSEL** after the USB cable is connected.
5. The Pico should appear as a USB mass storage device called **"RPI-RP2"** (or a new COM port if using picotool).

> If the Pico does not appear, try a different USB cable — some cables are power-only and do not carry data.

#### 9. Compile and Upload

1. In Arduino IDE, click the **Verify** button (✓ checkmark) first to compile without uploading. This catches errors before attempting to flash.
2. Once compilation succeeds, click **Upload** (→ arrow).
3. If the Pico is in BOOTSEL/mass-storage mode, the IDE will flash directly. If the Pico is already running, the IDE will attempt to auto-reset it via the USB serial port (1200 bps touch).
4. Wait for **"Done uploading"** in the status bar. The Pico will reboot automatically and the NeoPixel LED should light up **orange** (booting).

> **Compilation time:** The first build takes 2–4 minutes because the IDE compiles the entire arduino-pico core. Subsequent builds are incremental and much faster (~10–30 seconds).

#### 10. Connect the Serial Monitor

1. Go to **Tools → Port** and select the COM port that appeared after uploading (e.g., `COM3` on Windows, `/dev/ttyACM0` on Linux, `/dev/cu.usbmodem*` on macOS).
2. Open **Tools → Serial Monitor** (or press `Ctrl+Shift+M`).
3. Set the baud rate to **115200** at the bottom of the Serial Monitor window.
4. Press the Pico's **RESET** button (or disconnect/reconnect USB) to reboot. You should see:
   ```
   ====================================================
    USB Keyboard Sniffer v1.0.0 (Encrypted, Passive)
   ====================================================
   Flash log   : 0 events
   Build password: XXXX-XXXX-XXXX
   Keyboard layout: US | Language: EN
   Type HELP for available commands.
   ```

> **Save the build password!** It is derived from the random seed and the Pico's unique chip ID — it cannot be recovered from the firmware binary. Store it somewhere safe.

#### 11. Connect the USB Keyboard Tap

With the firmware running and serial monitor open:

1. **Disconnect power** from the Pico before wiring.
2. Connect the USB keyboard tap wires according to the [Wiring Diagram](#wiring-diagram).
3. Reconnect power to the Pico.
4. Type on the keyboard — you should see the NeoPixel glow **green** (capturing, unlocked).
5. Use `DUMP` in the Serial Monitor to decrypt and review captured keystrokes.

---

### Troubleshooting (Arduino IDE)

<details>
<summary><b>Compile error: "USBSnifferPIO_RP2040.h: No such file or directory"</b></summary>

The library was not installed or the IDE cannot find it. Re-install **USBSnifferPIO_RP2040** via the Library Manager and restart the IDE. Verify it appears under **Sketch → Include Library → Contributed Libraries**.
</details>

<details>
<summary><b>Compile error: "PICO_FLASH_SIZE_BYTES" undeclared</b></summary>

The flash size define was not passed to the compiler. Go to **Tools → Flash Size** and ensure **2MB** is selected (not "2MB (no FS)"). If the issue persists, the arduino-pico core version may be outdated — update it in the Boards Manager.
</details>

<details>
<summary><b>Upload fails: "No drive to deploy" or "Cannot find RPI-RP2"</b></summary>

The Pico is not in BOOTSEL mode. Disconnect the USB cable, hold the BOOTSEL button, reconnect USB, then release BOOTSEL. The Pico should appear as a USB drive. Try uploading again. If it still fails, try a different USB port or cable.
</details>

<details>
<summary><b>Serial Monitor shows garbage or nothing</b></summary>

Ensure the baud rate is set to **115200** in the Serial Monitor. If the port menu shows no ports, install the [USB Serial driver](https://github.com/earlephilhower/arduino-pico#installing-via-arduino-boards-manager) for your OS (Windows users may need to install drivers via Zadig).
</details>

<details>
<summary><b>PIO capture produces malformed/missing packets</b></summary>

CPU speed is likely wrong. Go to **Tools → CPU Speed** and verify **120 MHz** is selected. Also confirm **Optimize** is set to **Optimize (-O2)**. Recompile and re-upload after changing these settings.
</details>

<details>
<summary><b>NeoPixel LED does not light up</b></summary>

Check the **Adafruit NeoPixel** library is installed (version ≥ 1.12.0). If installed but no light, the GP25 pin may be damaged or the LED may require a level shifter on some Pico clones. The sniffer works without the LED — it is purely cosmetic feedback.
</details>

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
