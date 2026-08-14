# Pairing walkthroughs

The same instructions the app shows on screen, with more room to explain. Work
from the **motor unit**, not the remote — the LEARN button is what decides which
remotes the opener trusts.

> **The six-second rule.** On almost every brand, *holding* LEARN for about six
> seconds wipes every remote the opener knows, including the ones in your car
> and your neighbour's keypad. Press and **release**. If you do wipe it, you can
> re-pair everything, it is just an annoying afternoon.

---

## Chamberlain / LiftMaster — Security+ 2.0

**You have this if:** the opener was made from about 2011 onward, and the LEARN
button on the back of the motor unit is **yellow**. Often branded LiftMaster,
Chamberlain, Craftsman, or sold with myQ.

**Frequencies:** pick **All bands**. These receivers listen on 310, 315 and
390 MHz, and a real remote transmits on all three every press. A single band is
often enough to get through LEARN and then unreliable afterwards — which looks
like "it paired but the door does nothing".

If the picker shows *All bands (2 of 3)*, your region is blocking 390 MHz; turn
on **Settings → All frequencies** to get all three.

1. **Find LEARN.** Get to the back of the motor unit on a ladder. You may need
   to pop off the light lens. The LEARN button is square and sits next to the
   short antenna wire, usually beside two or three small adjustment buttons.
2. **Press and release LEARN.** The small LED next to it lights up. You have
   about 30 seconds.
3. **Transmit.** Stand within a few feet of the motor unit and press **Send**.
   GarageMate sends the code once, on each of the three bands.
4. **Confirm.** The opener's LED blinks off, or the lights flash once. That
   means the remote was stored.
5. **Test.** Back out to the door screen and press OPEN.

---

## Chamberlain / LiftMaster — Security+ 1.0

**You have this if:** the opener is from roughly 1997–2011 and the LEARN button
is **purple, red, orange or green**.

GarageMate cannot generate these — the firmware exposes a generator for
Security+ 2.0 and KeeLoq, but not for Security+ 1.0. The stock app can:

1. Leave GarageMate. Open **Sub-GHz → Add Manually → Security+ 1.0**.
2. Save it with a name you'll recognise.
3. Pair it: press and release LEARN on the motor unit, then transmit from the
   stock app twice.
4. Come back to GarageMate and use **Add a door → Import saved .sub** so it
   lives in your door list with everything else.

---

## Chamberlain — vintage, DIP switches

**You have this if:** the remote has a row of tiny switches inside the battery
compartment. No rolling code.

You do **not** need to match the switch positions. The receiver learns whatever
code it hears when you press LEARN, so GarageMate sends a fresh random one.

1. Press and release the **LEARN** or **SMART** button on the motor unit.
2. Press **Send**.
3. The LED goes out or the lights blink. Test the door.

---

## CAME, Nice FLO, Princeton, Linear / Multi-Code gates

**You have this if:** it is a driveway gate or a simple garage receiver with a
small button marked LEARN, PROG, SET or RADIO on the receiver board.

These are fixed codes, so pairing is just "receiver, remember this number".

1. **Open the receiver housing** on the gate motor. The button is next to an LED,
   sometimes under a plastic cover.
2. **Press it once.** The LED lights or starts blinking — it is waiting.
3. **Press Send.** GarageMate transmits a random code derived from this door's
   serial, so it is the same every time.
4. The LED turns off or flashes to confirm.

Pick the entry that matches your brand — the bit width differs (CAME and Nice
FLO are 12-bit, Princeton 24-bit, Linear 10-bit) and the receiver only accepts
its own.

---

## KeeLoq gates (DoorHan, Beninca, AN-Motors, ...)

**You have this if:** it is a European-style gate motor with a rolling-code
remote.

KeeLoq needs the manufacturer's secret key. GarageMate uses the keystore already
on your SD card at `/ext/subghz/assets/keeloq_mfcodes`. If your manufacturer
isn't in there, the gate cannot be paired this way — the app has no way to
produce a code the receiver will accept.

1. Find **LEARN/PROG/SET** on the receiver board inside the motor housing.
2. Press it once.
3. Press **Send**.
4. The receiver LED or beeper confirms.

The default manufacturer is DoorHan. To target another, change `.keeloq_mfg` in
[`src/catalog/gm_brands.c`](../src/catalog/gm_brands.c) — the name must match a
key in the keystore exactly.

---

## Genie / Overhead Door — Intellicode

Not supported. See [PROTOCOLS.md](PROTOCOLS.md#genie--intellicode) for why and
what to do instead.

---

## One press means one press

An opener treats every *distinct* rolling code it accepts as a separate button
press, and a garage door button toggles: open, stop, close. So two codes in a
row start the door and then stop or reverse it.

GarageMate therefore sends exactly **one** code per OPEN. Where the same code
goes out on three bands, the receiver acts on whichever it hears first and
discards the others as replays, so that still counts as a single press.

If your opener sometimes misses a press, do **not** reach for a second code.
Use **Settings → Signal length** instead, which repeats the same frame within
one transmission — the equivalent of holding the button a moment longer.

## After pairing

- **Test before you climb down.** Open and close the door twice from the door
  screen.
- **Keep the old remote paired.** Pairing GarageMate does not remove anything;
  openers hold multiple remotes (typically 5–20).
- **Back up your doors.** Copy `/ext/apps_data/garagemate/doors/` somewhere
  safe. If you lose the SD card you re-pair from scratch, which means another
  trip up the ladder.
