# Troubleshooting

## The app won't start / "API version mismatch"

A `.fap` only loads on the firmware API it was built against. Check what your
Flipper runs:

```bash
ufbt cli
> info device        # look at firmware.version and firmware.api.major
```

Then rebuild against that version:

```bash
ufbt update -t f7 --url https://update.flipperzero.one/builds/firmware/<VERSION>/flipper-z-f7-sdk-<VERSION>.zip
ufbt
```

This repo targets **1.3.4 / API 86.0**. Plain `ufbt update` fetches the latest
release SDK instead, which will not load on older firmware.

---

## The frequency I need says "(blocked)"

Your Flipper's region data forbids it. On a US-provisioned device that includes
**390 MHz**, which several Chamberlain and LiftMaster openers use.

**Turn on Settings → All frequencies.** That widens the region table to
everything the radio supports (300–348, 387–464, 779–928 MHz), which covers
390 MHz. It applies only while GarageMate is running, is never written to flash,
and is undone when you exit. Details:
[PROTOCOLS.md](PROTOCOLS.md#unlocking-frequencies).

Also worth trying: **315 MHz**. Security+ 2.0 receivers generally listen on 310,
315 and 390 MHz at once, so a 315 MHz pairing often works on a "390 MHz" opener.

For a device-wide unlock that also covers the stock Sub-GHz app, you need custom
firmware — Momentum, Unleashed or RogueMaster, listed in
[PROTOCOLS.md](PROTOCOLS.md#if-you-want-it-device-wide).

---

## Pairing didn't take

Work through these in order — the first three cover most cases.

1. **Wrong frequency.** The single most common cause. It is printed on the back
   of your existing remote. If your brand offers more than one, try the other.
2. **Too far away.** Be within a few feet of the motor unit while sending. The
   Flipper's antenna is much weaker than a real remote's.
3. **LEARN timed out.** You get roughly 30 seconds after pressing LEARN. Press
   it again and send promptly.
4. **Wrong button on the motor unit.** LEARN is usually next to the antenna
   wire. Do not use the up/down travel-limit buttons.
5. **Opener memory full.** Most units hold 5–20 remotes. If yours is full it
   silently ignores new ones — clear it (hold LEARN ~6 s, which erases
   everything) and re-pair *all* your remotes.
6. **It is a Genie.** See [PROTOCOLS.md](PROTOCOLS.md#genie--intellicode).

---

## It paired — I heard the click — but the door never responds

Classic Security+ 2.0 symptom, and usually one of two things.

**1. The door is on a single band.** These receivers listen on 310, 315 and
390 MHz. One band can be enough to get through LEARN and then unreliable in
normal use. Re-add the door and choose **All bands**.

**2. The door was created before GarageMate masked serials.** Security+ 2.0
remotes must have bits `0x80000C03` clear in their serial to look like a real
850LM. Earlier versions generated a fully random serial, which an opener will
accept during LEARN and then ignore forever.

Check your `.door` file — if `Serial` fails this, the door cannot be fixed by
re-pairing alone:

```bash
python3 -c "s=<Serial from the file>; print('conforms:', s & 0x80000C03 == 0)"
```

Either way the fix is the same: **delete the door and add it again**, choosing
All bands. The new remote gets a conforming serial, so it is a different remote
as far as the opener is concerned and must be paired fresh. Clearing the old one
out of the opener is optional — it will simply sit unused in its memory.

## The door starts, then stops or reverses

Something is sending two codes where one was meant. The opener reads each
distinct rolling code as another press of the button, and the button toggles —
so the first code starts the door and the second stops or reverses it.

GarageMate sends exactly one code per OPEN, so if you see this:

- Check **Settings → Signal length**. It should lengthen a single press, not
  send extra codes; any value here is safe.
- Make sure you are not also pressing a wall button or another remote.
- If you are on a version before this behaviour was fixed, update — earlier
  builds sent two codes per open.

To make a marginal signal more reliable, raise **Signal length**. Never send a
second press.

## It worked, then stopped

Rolling-code openers track a counter and reject codes that look too old. If the
opener saw codes from another remote in the meantime, or the `.door` file was
restored from a backup, the counter can fall behind.

Edit the door file at `/ext/apps_data/garagemate/doors/<id>.door` and increase
`Counter` by a few hundred:

```
Counter: 812
```

Openers accept a forward jump within a window, but never a value they have
already seen — so always jump **forward**, never back.

---

## The door list is empty after copying files

`gm_store_load_all` skips records it cannot parse rather than failing the whole
scan, so one malformed file hides only itself. Check that each file:

- ends in `.door` and sits in `/ext/apps_data/garagemate/doors/`
- starts with `Filetype: GarageMate Door` and `Version: 1`
- has all of `Name`, `Brand`, `Frequency`, `Serial`, `Button`, `Counter`,
  `Managed` — in that order (the parser reads forwards)
- uses a `Brand` value that exists in `src/catalog/gm_brands.c`

The list caps at 24 doors (`GM_DOORS_MAX`).

---

## Exported .sub doesn't work in the stock app

Check the `Preset:` line. It should read `FuriHalSubGhzPresetOok650Async`. If it
says `FuriHalSubGhzPresetCustom` with an empty `Custom_preset_data`, the preset
name was passed to the generator in the wrong form — see the note in
[PROTOCOLS.md](PROTOCOLS.md#a-trap-worth-documenting).

---

## Debugging on the device

`tools/flipper_cli.py` wraps the Flipper's serial CLI (230400 baud, one session
at a time — close qFlipper first):

```bash
python3 tools/flipper_cli.py cmd "info device" "loader info" "free"
python3 tools/flipper_cli.py cmd "storage list /ext/apps_data/garagemate/doors"
```

Useful commands: `info device` (firmware version, API, region), `loader info`
(which app is running), `free` (heap, for spotting leaks), `storage read <path>`.

It can also drive the UI, which is what makes automated smoke tests on real
hardware possible:

```bash
python3 tools/flipper_cli.py tap ok right down ok
```

**The gotcha it exists to hide:** `ViewDispatcher` discards a `short` or `long`
event that was not preceded by a `press` for the same key — it logs
"non-complementary input, discarding" and moves on. So this does nothing at all,
while looking like it worked:

```
input send ok short
```

All three events must be sent:

```
input send ok press
input send ok short
input send ok release
```

Key and type names are lowercase (`ok`, `back`, `up`, `down`, `left`, `right` /
`press`, `release`, `short`, `long`); capitalised names are rejected with a
usage message.
