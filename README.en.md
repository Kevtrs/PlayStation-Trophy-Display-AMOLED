# PlayStation Trophy Display AMOLED

*[Version française](README.md)*

A desktop display that shows your PlayStation trophies in real time
(level, stats, trophy breakdown) on a round AMOLED screen, powered by an
ESP32-S3 and [Pocket PSN](https://pocketpsn.com) data.

![Firmware](https://img.shields.io/badge/firmware-ESP32--S3-blue)
![License](https://img.shields.io/badge/license-MIT-green)

<p align="center">
  <img src="docs/screenshots/welcome.png" width="180" alt="Welcome screen">
  <img src="docs/screenshots/dashboard.png" width="180" alt="Dashboard">
  <img src="docs/screenshots/trophies.png" width="180" alt="Trophies screen">
  <img src="docs/screenshots/statistics.png" width="180" alt="Statistics screen">
</p>
<p align="center"><sub>Simulator captures (demo data) -- rendering on the real round screen is identical.</sub></p>

## Install (the easy way)

No software to install: open the web installer from **Google Chrome or
Microsoft Edge**, plug the board in over USB, and click "Install".

**[→ Web installer](https://kevtrs.github.io/PlayStation-Trophy-Display-AMOLED/webinstall/)**

See [webinstall/README.md](webinstall/README.md) for details (supported
browsers, publishing a new version).

## Hardware required

- [Waveshare ESP32-S3-Touch-AMOLED-1.75](https://www.waveshare.com/esp32-s3-touch-amoled-1.75.htm)
  (466x466 round display, CST9217 capacitive touch)
- A USB-C **data** cable (not a charge-only cable)
- An active [Pocket PSN](https://pocketpsn.com) account (the service that
  provides PlayStation trophy data)

## Features

- Dashboard: PSN level, progress, total trophies
- "Trophies" screen: Platinum/Gold/Silver/Bronze breakdown
- Detailed statistics (completed games, completion rate, play time...)
- Built-in Wi-Fi setup portal (`TrophyDisplay-Setup` access point on
  first boot, no app to install)
- Offline cache: the last known data stays on screen without a network
- Built-in demo mode (no account required to try out the display)

## Traditional install (PlatformIO)

To build it yourself or contribute to the code:

1. Install [Python](https://www.python.org/downloads/) then
   [PlatformIO](https://platformio.org/) (`pip install platformio`).
2. Clone this repository.
3. Run `INSTALLER_ET_LANCER.bat` (Windows) — detects the board, builds,
   flashes, and opens the serial monitor automatically.

See [PREMIER_DEMARRAGE.md](PREMIER_DEMARRAGE.md) (French) for the detailed
procedure and [DEPANNAGE_MATERIEL.md](DEPANNAGE_MATERIEL.md) (French) for
hardware troubleshooting.

## First boot

1. The board creates its own Wi-Fi network: `TrophyDisplay-Setup`.
2. Connect to that network, then open `http://192.168.4.1/`.
3. Select your Wi-Fi network, enter the password, fill in your PSN
   username, then click **Save** once.
4. The board restarts and connects automatically.

## Development

- `simulator/`: PC (SDL2) simulator of the interface, to iterate on the
  design without hardware — see `simulator/README.md`.
- `tools/`: Pocket PSN protocol investigation tools (one-off use, not
  required to use the firmware).
- `AUDIT.md` / `HANDOFF_PROGRESS.md` / `PROJECT_STATUS.md` (French):
  detailed log of every technical decision and discovery made during
  development — useful if you want to understand the "why" behind a
  choice, or to contribute.

## Credits

- PlayStation data provided by [Pocket PSN](https://pocketpsn.com).
- Project inspired by the original firmware by
  [tomtechie](https://github.com/tomtechie/Playstation-Trophies-ESP-Display),
  who put us in touch with Pocket PSN for this project.
- Design and development: Kevin Torres.

## License

[MIT](LICENSE) — see the `LICENSE` file for the full text.
