# PlayStation Trophy Display -- trophy display (round AMOLED screen)

> Ready-to-paste text for the MakerWorld page description.
> Sections in `[...]` still need real print info (material, settings,
> time) -- everything else is verified and accurate.

## Hook

A small connected display that shows your PlayStation level, trophies
and stats in real time -- sitting on your desk, next to your PC or your
gaming setup. Round AMOLED screen, data updates automatically, zero
maintenance once set up.

## What it is

This project combines a 3D-printed part with a small computer (an
ESP32-S3 board with a built-in touchscreen) that connects to your
PlayStation account through [Pocket PSN](https://pocketpsn.com) to
continuously display:

- Your PSN level and progress
- Your total trophy count, including Platinums
- Your stats: games completed, completion rate, playtime, world rank
- Data stays on screen even without a connection (offline cache)

No app to install on your PC or phone: everything is set up directly
on the screen, over Wi-Fi.

## Screenshots

> Generated from the simulator (demo data) -- the real screen looks
> identical. Upload these as images on the MakerWorld page (the
> Markdown link below won't render as-is on MakerWorld, it's just a
> reference).

| Welcome | Dashboard | Trophies | Statistics |
|---|---|---|---|
| ![Welcome](../screenshots/welcome.png) | ![Dashboard](../screenshots/dashboard.png) | ![Trophies](../screenshots/trophies.png) | ![Statistics](../screenshots/statistics.png) |

## What you need

- A [Waveshare ESP32-S3-Touch-AMOLED-1.75](https://www.waveshare.com/esp32-s3-touch-amoled-1.75.htm)
  board (round 466x466 screen, capacitive touch) -- not included, buy
  separately
- A USB-C cable (data-capable, not charge-only)
- An active [Pocket PSN](https://pocketpsn.com) account (free
  third-party service that provides PlayStation trophy data)
- [PRINT MATERIAL -- e.g. PLA, X g]
- [SCREWS / HEAT-SET INSERTS if needed, quantity and size]

## 3D printing

- Recommended material: [TO FILL IN]
- Color used in the photos: [TO FILL IN]
- Layer height: [TO FILL IN]
- Infill: [TO FILL IN]
- Supports: [TO FILL IN -- yes/no, which parts]
- Estimated total print time: [TO FILL IN]
- Recommended orientation: [TO FILL IN]

## Assembly

1. [Step 1 -- e.g. insert the Waveshare board into the front housing]
2. [Step 2 -- e.g. secure with the provided screws]
3. [Step 3 -- e.g. route the USB-C cable through the rear opening]
4. [Step 4 -- e.g. close the housing]

## Installing the firmware (no technical skills required)

1. Open **Google Chrome** or **Microsoft Edge** (required -- other
   browsers won't work for this step).
2. Plug the board into your computer via USB-C.
3. Go to the install page:
   **https://kevtrs.github.io/PlayStation-Trophy-Display-AMOLED/webinstall/**
4. Click **Install** and follow the on-screen instructions.
5. Once installation is complete, unplug and replug the board.

No software to download, no command line.

## First-time setup

1. The board creates its own Wi-Fi network: `TrophyDisplay-Setup`.
2. Connect to that network from your phone or computer, then open
   `http://192.168.4.1/` in a browser.
3. Select your home Wi-Fi network, enter its password, enter your PSN
   username, then click **Save** once.
4. The board restarts and connects automatically. Your trophies show
   up within seconds.

## Source code

The full firmware is open source:
**https://github.com/Kevtrs/PlayStation-Trophy-Display-AMOLED**

## Credits

- PlayStation data provided by [Pocket PSN](https://pocketpsn.com).
- Design and development: Kevin Torres.

## License

Firmware under the MIT license. [3D MODEL LICENSE -- specify on the
MakerWorld page itself, e.g. CC BY-NC-SA 4.0]
