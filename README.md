# GarageMate

A Flipper Zero app that replaces the garage and gate remotes in your car.

It does **not** copy your existing remotes. It creates a *new* virtual remote on
the Flipper and walks you through teaching your opener to accept it — the same
thing you'd do with a spare remote from the hardware store. Once paired, each
door shows up in a list and opens with one button.

```
┌──────────────────────┐     ┌──────────────────────┐
│ Your doors           │     │ 2. Press LEARN (2/4) │
│  Left bay            │     │──────────────────────│
│  Right bay           │ ──▶ │ Press and release    │
│  Side gate           │     │ LEARN once. Do not   │
│  Add a door          │     │ hold it...           │
│  Settings            │     │                      │
└──────────────────────┘     │  Back   Send   Next  │
                             └──────────────────────┘
```

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
| **Genie / Overhead Door (Intellicode)** | Intellicode | ❌ **No — see below** |

### Genie / Intellicode is not supported

The official Flipper firmware ships **no Genie protocol at all** — the string
"Genie" does not appear anywhere in the firmware binary. There is nothing to
generate and nothing to pair with, so this is not something the app can work
around. Picking "Genie / Overhead Door" in the app shows an explanation and your
actual options instead of pretending to work.

Your options, in order of how well they work:

1. **Buy a Genie remote or wireless keypad.** Cheapest and always works.
2. **Use a third-party Genie recorder app** ([jamisonderek's
   genie-recorder](https://github.com/jamisonderek/flipper-zero-tutorials/tree/main/subghz/apps/genie-recorder)),
   which records rolling codes from a Genie remote you already own and can send
   the next one. It needs the original remote — it does not pair a new one.
3. **Check the age of the unit.** Genie openers from before roughly 1995 use DIP
   switches, not Intellicode. If yours has a row of tiny switches, pick
   "Fixed-code gate" in GarageMate and it will pair.

See [docs/PROTOCOLS.md](docs/PROTOCOLS.md) for the full reasoning.

---

## ⚠️ Read this first: 390 MHz is blocked on this Flipper

This Flipper is region-provisioned as **US**, and its region table allows
roughly 304–322 MHz, 433.05–434.79 MHz and 915–928 MHz. **390 MHz is not in
that list**, and many Chamberlain/LiftMaster openers use 390 MHz — including the
`Sk_upper.sub` already on your SD card.

Verified on the device: transmitting at 315 MHz and 433.92 MHz succeeds, and at
390 MHz the radio is refused.

What to do about it:

- **Try 315 MHz first.** Most Security+ 2.0 receivers listen on 310, 315 *and*
  390 MHz, so pairing at 315 MHz usually works even on a "390 MHz" opener. This
  is the fix that requires nothing else.
- GarageMate marks unusable frequencies as `(blocked)` in the frequency picker,
  so you find out during setup rather than on a ladder.
- If you genuinely need 390 MHz, that requires changing your Flipper's region
  enforcement (a firmware/region-data change). This repo does not do that for
  you — see [docs/TROUBLESHOOTING.md](docs/TROUBLESHOOTING.md).

---

## Install

It is **already installed** on the connected Flipper at
`SD Card/apps/Sub-GHz/garagemate.fap` — open **Apps → Sub-GHz → GarageMate**.

To build it again (build output lands in `dist/`, which is not committed):

```bash
pip3 install ufbt

# Pin the SDK to the firmware your Flipper actually runs (see below)
ufbt update -t f7 --url https://update.flipperzero.one/builds/firmware/1.3.4/flipper-z-f7-sdk-1.3.4.zip

ufbt              # build
ufbt launch       # build, upload and start it on a connected Flipper
```

To put it on another Flipper, copy `dist/garagemate.fap` to
`SD Card/apps/Sub-GHz/` with qFlipper or the mobile app.

### Matching the SDK to your firmware

A `.fap` only loads if its API version matches the firmware. Check yours:

```bash
# firmware.version and firmware.api.major
ufbt cli
> info device
```

This repo is built and tested against **firmware 1.3.4, API 86.0, hardware
target 7**. For a different firmware, change the version in the `ufbt update`
URL, or run plain `ufbt update` for the current release.

---

## Using it

### Adding a door

1. **Apps → Sub-GHz → GarageMate → Add a door**
2. **Pick your opener.** The hint under each name ("yellow LEARN", "DIP
   switches") is usually enough to identify it. If unsure, look at the motor
   unit, not the remote.
3. **Pick the frequency.** It is printed on the back of your existing remote.
   Anything marked `(blocked)` cannot be transmitted by this Flipper.
4. **Name it** — "Left bay", "Side gate".
5. **Follow the pairing steps.** The app walks you through finding the LEARN
   button, pressing it, and transmitting. Step 3 has a **Send** button.

Full per-brand instructions: [docs/PAIRING.md](docs/PAIRING.md).

> **Do not hold LEARN for six seconds.** That erases every remote paired to the
> opener, including the ones in your car.

### Opening a door

Pick it from the list and press **OK**. Turn on *Hold to open* in Settings if
you'd rather not open the garage from your pocket.

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
| `Repeats` | Button presses sent per open. `0` follows the brand's own default. |
| `Feedback` | LED/vibro/beep when transmitting. |
| `HoldToOpen` | Require a long press on OPEN. |

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
- ✅ Settings persist; Genie explanation path, help and wizard screens all
  navigate without crashing
- ❌ 390 MHz refused by the device's region (see above)

**Not verified:** pairing against a real opener. That needs the physical motor
unit and its LEARN button, so the last step is yours. Security+ 1.0 generation
is not implemented at all — the firmware exposes no generator for it.

---

## Licence

MIT — see [LICENSE](LICENSE).

Use this on doors you own or are authorised to open.
