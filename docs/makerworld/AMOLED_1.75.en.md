# PlayStation Trophy Display -- trophy display (round AMOLED screen)

> Ready-to-paste text for the MakerWorld page description.
> Settings and estimates extracted from the provided Bambu Studio
> project (`PocketPSN Trophy.3mf`, sliced on a Bambu Lab X1 Carbon).
> May vary slightly with a different printer.

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
- PLA filament -- white for the body, blue for the embossed PlayStation
  logo and accent shape (optional: printable in a single color if you
  don't have an AMS/filament changer)
- 3 small magnets (case closure -- no screws needed for the case itself)

## 3D printing

The case is **2 parts**: a front cover and a back shell, held together
with magnets (no screws). The back shell carries the PlayStation logo
and a decorative accent, embossed in a second color.

- Printer used for the design: Bambu Lab X1 Carbon (0.4 mm nozzle)
- Material: PLA -- white (body), blue (embossed logo/accent)
- Profile: "0.28mm Extra Draft" (fast print)
- Layer height: 0.28 mm (first layer 0.2 mm)
- Wall loops: 2
- Infill: 5%
- Supports: yes (tree, automatic)
- Filament weight: ~31 g (white + blue)
- Print time: ~55 min

*(Bambu Studio estimates, printer and profile above -- may vary with
other hardware.)*

## Assembly

1. Print the front cover and back shell (see settings above).
2. Insert the 3 magnets into their sockets on the back shell.
3. Place the Waveshare board into the front cover.
4. Route the USB-C cable through the provided opening.
5. Close with the back shell -- the magnets hold everything together,
   no screws needed.

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
