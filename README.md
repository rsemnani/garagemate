# GarageMate

A Flipper Zero app that replaces the garage and gate remotes in your car.

It does **not** copy your existing remotes. It creates a *new* virtual remote on
the Flipper and walks you through teaching your opener to accept it — the same
thing you'd do with a spare remote from the hardware store. Once paired, each
door shows up in a list and opens with one button.

![Adding a door in GarageMate: brand picker, frequency picker, the four pairing
steps, and the finished door screen](docs/walkthrough.gif)

<sub>Drawn from the app's screen definitions at the Flipper's native 128×64 and
scaled up — see [`tools/make_demo_gif.py`](tools/make_demo_gif.py).</sub>

---

## Why pairing, not cloning

Every opener made in the last ~15 years uses a **rolling code**: the remote
sends a different number on every press, and the opener refuses any number it
has already seen. Recording one press and replaying it does nothing, by design.

The supported way to add a remote is to press the **LEARN** button on the motor
unit and transmit — which requires you to be standing at the opener. GarageMate
automates the radio half and talks you through the physical half.

---

## What works on stock firmware

| Opener | Protocol | GarageMate can pair it |
| --- | --- | --- |
| Chamberlain / LiftMaster, yellow LEARN (2011+) | Security+ 2.0 | ✅ Yes |
| Chamberlain / LiftMaster, coloured LEARN (1997–2011) | Security+ 1.0 | ⚠️ Via stock app, then import |
| Chamberlain vintage, DIP switches | Cham_Code | ✅ Yes |
| CAME, Nice FLO, Princeton, Linear / Multi-Code gates | fixed code | ✅ Yes |
| DoorHan, Beninca, AN-Motors and other KeeLoq gates | KeeLoq | ✅ Yes |
| **Genie / Overhead Door (Intellicode)** | Intellicode | ✅ **Learn-and-replay — see below** |

### Genie / Intellicode — learned from your own remote

The official Flipper firmware ships **no Genie protocol at all** — the string
"Genie" does not appear anywhere in the firmware binary — and Intellicode's
rolling code cannot be computed, because the algorithm lives inside the remote.
So neither "clone a remote" nor "generate a new one" is possible for anyone.

GarageMate takes the one route that *does* work, the same principle as
[ratgdo](https://github.com/ratgdo) but over the air instead of over wires:
**it becomes a remote you already own.** It bundles a from-scratch Genie
decoder/encoder (the firmware has none), learns real codes off the air as you
press your remote, and replays them.

How to use it — no wiring, no soldering:

1. Take your Genie remote **away from the garage**, out of the opener's range.
2. In GarageMate: add a Genie door, then **learn** — press your remote next to
   the Flipper a handful of times. Each press stores one code the opener has not
   seen.
3. At the garage, press **OPEN**. The Flipper sends the next stored code; the
   opener accepts the small forward jump and re-syncs to it.
4. When the batch runs low, learn again. Your real remote keeps working
   alongside it.

Why this is sound rather than a security break: Genie's sequence is only 16-bit
and cyclic, you are duplicating the forward codes of **your own** remote, and
the opener resyncs to them exactly as it would to that remote. Details and the
protocol port are in [docs/GENIE.md](docs/GENIE.md).

> Older Genie units (pre-1995) use DIP switches, not Intellicode — a row of tiny
> switches on the remote. Pick "Fixed-code gate" for those; no capture needed.

### Per-door icons

Each door carries an icon so the list is scannable — a **garage door** (the
default), a **gate**, or a **light / other** for the RF relays these clickers
often drive (shop lights, pumps, outlets). Change it from a door's
**More → Change icon**.

---

## Frequencies: turn on "All frequencies"

A US-provisioned Flipper permits roughly 304–322 MHz, 433.05–434.79 MHz and
915–928 MHz. **390 MHz is not in that list** — and plenty of Chamberlain and
LiftMaster openers use 390 MHz, so out of the box those are unreachable.

**Settings → All frequencies → ON** fixes that. It widens the region table to
everything the CC1101 can physically tune (300–348, 387–464, 779–928 MHz), so
390 MHz — along with 310, 318 and 868 MHz — becomes usable.

Measured on real hardware, same 390 MHz door either way:

| All frequencies | Result |
| --- | --- |
| OFF | refused — counter never advanced |
| ON | transmitted — counter advanced 0 → 2 |

How it works, and why it needs no custom firmware: the firmware decides whether
it may transmit purely by walking a band table
(`furi_hal_subghz_set_frequency()` drops the radio to receive-only when
`furi_hal_region_is_frequency_allowed()` says no), and `furi_hal_region_set()`
is exported to applications. GarageMate swaps in a wider table.

- The change is **RAM-only**. It is never written to flash, GarageMate puts the
  original table back when it exits, and a reboot would clear it regardless.
- It applies **only while GarageMate is running**. The stock Sub-GHz app is
  unaffected — import a signal into GarageMate if you want to send it at 390 MHz.
- For a device-wide unlock you need custom firmware; see
  [docs/PROTOCOLS.md](docs/PROTOCOLS.md#unlocking-frequencies).

390 MHz garage openers are legal to operate in the US — that is the band the
openers themselves are licensed under (FCC Part 15.231). The Flipper's stock
region table is simply more conservative than the rules require. Keep to
equipment you own.

> Still worth trying 315 MHz first: Security+ 2.0 receivers generally listen on
> 310, 315 *and* 390 MHz, so a 315 MHz pairing often works on a "390 MHz"
> opener.

---

## Install

GarageMate builds with [ufbt](https://github.com/flipperdevices/flipperzero-ufbt),
the official Flipper application build tool. You need Python 3 and a USB cable.

### 1. Check which firmware your Flipper runs

A `.fap` only loads if it was built against a **matching firmware API version**,
so this step is not optional. On the Flipper: **Settings → About**. Or over USB:

```bash
pip3 install ufbt
ufbt cli
>: info device        # read firmware.version and firmware.api.major
```

### 2. Build

```bash
git clone https://github.com/rsemnani/garagemate.git
cd garagemate

# Pin the SDK to YOUR firmware version — substitute it into the URL
ufbt update -t f7 --url https://update.flipperzero.one/builds/firmware/1.3.4/flipper-z-f7-sdk-1.3.4.zip

ufbt                  # -> dist/garagemate.fap
```

If your Flipper is on the current official release, plain `ufbt update` fetches
the right SDK and you can skip the URL.

### 3. Put it on the Flipper

With the Flipper connected:

```bash
ufbt launch           # build, upload and start it
```

Or copy `dist/garagemate.fap` onto the SD card at `apps/Sub-GHz/` using
[qFlipper](https://flipperzero.one/update) or the mobile app.

Either way it then appears on the Flipper under **Apps → Sub-GHz → GarageMate**.

> Built and tested against firmware **1.3.4 (API 86.0, hardware target 7)**.
> Other 1.x firmwares should work once the SDK is pinned to match; the app uses
> only published API symbols.

---

## Using it

### Adding a door

1. **Apps → Sub-GHz → GarageMate → Add a door**
2. **Pick your opener.** The hint under each name ("yellow LEARN", "DIP
   switches") is usually enough to identify it. If unsure, look at the motor
   unit, not the remote.
3. **Pick the frequency.** For Chamberlain/LiftMaster choose **All bands** — the
   receiver listens on 310, 315 and 390 MHz and a real remote uses all three.
   For other brands it is printed on the back of your existing remote. Anything
   marked `(blocked)` is outside your Flipper's region — see above.
4. **Name it** — "Left bay", "Side gate".
5. **Follow the pairing steps.** The app walks you through finding the LEARN
   button, pressing it, and transmitting. Step 3 has a **Send** button.

Full per-brand instructions: [docs/PAIRING.md](docs/PAIRING.md).

> **Do not hold LEARN for six seconds.** That erases every remote paired to the
> opener, including the ones in your car.

### Opening a door

Pick it from the list and press **OK**. Turn on *Hold to open* in Settings if
you'd rather not open the garage from your pocket.

### Two presses from the desktop

GarageMate reopens on the door you used last, so pulling up to the garage is one
press to launch and one to open. **Back** steps out to the full list as usual.

To get the launch down to a single press, bind the app to a desktop button:
**Settings → Desktop → Set Quick Access Apps → Default Mode → Left - Press**,
then choose GarageMate under *Sub-GHz*. From the desktop, **Left** now goes
straight to your last door.

Stock firmware only lets you bind *Left - Press*, *Right - Press*, *Left - Hold*
and *Right - Hold*. The Up and Down buttons are hardcoded (lock menu, archive and
debug) and cannot be reassigned without building custom firmware.

Note that a quick-access button plus one **OK** opens the door with no other
confirmation. That is the point, but if the Flipper rides loose in a pocket or a
door pocket, turn on *Hold to open*.

### Importing an existing signal

**Add a door → Import saved .sub** adopts a signal made by the stock Sub-GHz
app, which is how Security+ 1.0 remotes get in. RAW captures are rejected on
import — replay those in the stock app instead.

---

## Customising it

### Settings

**Settings** in the app, stored at
`/ext/apps_data/garagemate/settings.conf` as plain text:

| Setting | Meaning |
| --- | --- |
| `FrameRepeat` | How many times the frame repeats within one press. `0` follows the brand. Lengthens a single press; never sends a second code. |
| `Feedback` | LED/vibro/beep when transmitting. |
| `HoldToOpen` | Require a long press on OPEN. |
| `UnlockFrequencies` | Widen the region table to the radio's full range. |

### Doors

Each door is a small text file in `/ext/apps_data/garagemate/doors/`:

```
Filetype: GarageMate Door
Version: 1
Name: Left bay
Brand: chamberlain_sp2
Frequency: 315000000
Serial: 12345678
Button: 104
Counter: 7
Managed: true
```

You can edit these by hand, back them up, or copy them to another Flipper.
`Counter` is the rolling counter — if a door ever stops responding, bumping it
forward by a few hundred re-syncs it with the opener.

### Adding a brand

Everything brand-specific lives in one table in
[`src/catalog/gm_brands.c`](src/catalog/gm_brands.c). Add an entry, and the
picker, frequency list, walkthrough and radio all pick it up — no other file
changes:

```c
{
    .id = "my_gate",                       // stable, stored in .door files
    .display = "My gate brand",
    .hint = "Fixed code, 12 bit",
    .kind = GmProtoFixed,
    .protocol = "CAME",                    // firmware protocol name
    .bits = 12,
    .freqs = {433920000},
    .freq_count = 1,
    .repeats = 6,
    .rolling = false,
    .steps = steps_fixed_gate,             // reuse or write a new walkthrough
    .step_count = COUNT_OF(steps_fixed_gate),
},
```

Walkthrough steps are plain strings above the table. Mark the step that should
get a **Send** button with `.transmit = true`.

---

## How it is put together

```
application.fam            App manifest (ufbt)
icons/                     10x10 app icon
tools/make_icon.py         Regenerates the icon from readable pixel art
tools/flipper_cli.py       Runs CLI commands and injects button presses,
                           for smoke-testing on real hardware
tools/make_demo_gif.py     Renders the README walkthrough GIF
src/
  garagemate.c             Entry point, wiring, shared helpers
  garagemate_i.h           Shared application state
  catalog/gm_brands.*      Brand table + pairing walkthroughs
  model/gm_door.*          Door record
  model/gm_door_store.*    Loading/saving doors
  model/gm_settings.*      Preferences
  model/gm_paths.h         SD card locations
  radio/gm_generator.*     Door -> transmittable payload
  radio/gm_radio.*         CC1101 setup and transmission
  scenes/                  One file per screen
docs/                      Pairing, protocol and troubleshooting notes
```

Two design decisions worth knowing:

- **Rolling codes are re-derived, never stored.** A door keeps its serial,
  button and counter; the payload is rebuilt immediately before every press.
  The `.door` file stays the single source of truth for the counter, so doors
  survive being copied or hand-edited.
- **Exports are ordinary `.sub` files** under `/ext/subghz/garagemate/`, so the
  stock Sub-GHz app can open them. GarageMate never takes ownership of your
  signals.

---

## What has been verified on hardware

Tested against a Flipper Zero on firmware 1.3.4 (API 86.0, target 7):

- ✅ Builds clean with `-Werror`; installs and runs; exits without leaks
  (heap steady at ~101 KB free)
- ✅ Door records load, save, and persist the rolling counter across presses
- ✅ Security+ 2.0 — payload generated and transmitted at 315 MHz; counter
  advanced 0 → 2 over a two-press open and was written back to disk
- ✅ Cham_Code fixed code — 9-bit payload generated and exported
- ✅ KeeLoq — transmitted at 433.92 MHz
- ✅ Exported `.sub` files match the stock format
- ✅ Settings persist; help and wizard screens navigate without crashing
- ✅ Genie learn-and-replay — the receiver comes up and tears down cleanly on
  the capture screen (heap fully recovered, no leak), the sequence file persists,
  a stored code transmits and advances the playback cursor, and an empty batch
  fails safely instead of crashing. *Not verified: decoding a live Genie remote
  or operating a real Genie opener — needs the physical hardware.*
- ✅ Per-door icons render in the custom list and persist across restarts
- ✅ Launching reopens the last door: selecting it writes `LastDoor` to
  `settings.conf`, and a fresh launch lands two scenes deep — one **Back**
  returns to the list, a second exits the app
- ✅ Security+ 2.0 serials conform to the `0x7FFFF3FC` 850LM pattern and the
  counter starts at `0xE500000`, both checked by reading the generated record
  off the SD card
- ✅ Tri-band — one OPEN advances the rolling counter by exactly 1, so a press
  sends a single code across all three frequencies rather than burning three,
  and never reads as two button presses to the opener
- ✅ The generated payload carries `Repeat: 10` after `Secplus_packet_1`, where
  the protocol's forward-reading parser will find it
- ✅ **All frequencies** — A/B tested on one 390 MHz door: refused with the
  setting off, transmitted with it on. The original region table is restored on
  exit (a later run with the setting off is blocked again), and six toggles left
  the heap where it started

**Not verified:** pairing against a real opener. That needs the physical motor
unit and its LEARN button, so the last step is yours. Security+ 1.0 generation
is not implemented at all — the firmware exposes no generator for it.

---

## Licence

MIT — see [LICENSE](LICENSE).

Use this on doors you own or are authorised to open.
