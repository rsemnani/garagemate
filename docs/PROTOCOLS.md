# What is supported, and why

The constraint that shapes this whole app: a Flipper application can only call
functions the firmware **exports** in its API table. It cannot reach into
firmware internals. So GarageMate can drive exactly the Sub-GHz generators the
firmware chooses to publish, and no others.

On firmware 1.3.4 (API 86.0) the published Sub-GHz generators are:

```
subghz_protocol_secplus_v2_create_data   (void*, FlipperFormat*, serial, btn, cnt, preset)
subghz_protocol_keeloq_create_data       (void*, FlipperFormat*, serial, btn, cnt, mfg, preset)
subghz_protocol_secplus_v1_check_fixed   (uint32_t)   -- validation only, not a generator
```

That is the entire list. You can check it yourself:

```bash
grep -E "^Function,\+,subghz_protocol" \
  ~/.ufbt/current/sdk_headers/f7_sdk/targets/f7/api_symbols.csv
```

## The three ways GarageMate makes a payload

| Kind | How | Brands |
| --- | --- | --- |
| `GmProtoSecPlusV2` | `subghz_protocol_secplus_v2_create_data` | Chamberlain/LiftMaster Security+ 2.0 |
| `GmProtoKeeLoq` | `subghz_protocol_keeloq_create_data` | DoorHan, Beninca, AN-Motors, ... |
| `GmProtoFixed` | Written by hand — a fixed code is just an N-bit number | Cham_Code, CAME, Nice FLO, Princeton, Linear |
| `GmProtoManual` | No payload; shows an explanation | Security+ 1.0, Genie |

Fixed codes need no generator: the file is a header plus `Protocol`, `Bit`,
`Key` and optionally `TE`, and the firmware's own encoder turns that into a
waveform. GarageMate derives the key from the door's serial so a door always
transmits the same number.

### Security+ 2.0: the serial pattern is not optional

A Security+ 2.0 remote's serial cannot be an arbitrary random number. The stock
firmware's own generator masks it before use:

```c
key = (key & 0x7FFFF3FC); // 850LM pairing
subghz_txrx_gen_secplus_v2_protocol(txrx, "AM650", 310000000, key, 0x68, 0xE500000);
```

That mask forces bits 31, 11, 10, 1 and 0 (`0x80000C03`) to zero, so the serial
matches the shape of a real 850LM remote. Two things follow, and both were
learned the hard way against a Chamberlain 050ACTWF:

- **A non-conforming serial can pair and still never work.** The opener happily
  stores the remote during LEARN — you hear the confirmation click — and then
  ignores every later press. The failure is silent and looks exactly like a
  range or frequency problem.
- **The counter should not start at zero.** Stock begins at `0xE500000`, well
  away from the bottom of the 28-bit range, as a real remote would.

GarageMate carries both as `GmBrand::serial_mask` and `GmBrand::counter_start`,
applied by `gm_door_apply_brand()`.

### Tri-band receivers

Security+ 2.0 receivers listen on **310, 315 and 390 MHz**, and a genuine remote
transmits each press on all three. Sending on only one band is unreliable: it
may be enough for LEARN and then intermittent or dead in normal use.

A door with `AllBands` set transmits every press on all three. The important
detail is that a press consumes **one** rolling counter value which is then
reused across the bands — incrementing per band would burn three codes per
press and drift out of sync with the receiver. Bands the region forbids are
skipped rather than failing the press, so a locked-down Flipper still works on
whatever bands it does permit.

### One open, one code

An opener treats each distinct rolling code it accepts as a button press, and
the button toggles direction. Two codes therefore start the door and then stop
or reverse it — which is what happens if you model "repeat for reliability" as
"send it again".

So `gm_radio_press()` emits exactly one code. Reliability instead comes from the
optional `Repeat` field in the payload, which the protocol encoders read to
decide how many times to repeat the frame within a single transmission:

```c
//optional parameter parameter
flipper_format_read_uint32(flipper_format, "Repeat", (uint32_t*)&instance->encoder.repeat, 1);
```

It is appended after `Key`/`Secplus_packet_1` because the parser reads forwards,
and it is the equivalent of holding a real remote's button a moment longer. The
same code going out on three bands is still one press: the receiver acts on the
first and rejects the rest as replays.

### A trap worth documenting

`SubGhzRadioPreset.name` must hold the **short** preset name (`AM650`). The
serialiser expands it to `FuriHalSubGhzPresetOok650Async` when writing the file.
Passing the long name in makes the serialiser fall through to "custom preset"
and emit a file with an empty `Custom_preset_data` — which looks plausible and
does not work. This cost a debugging round; see `GM_PRESET_SHORT` in
[`src/radio/gm_generator.h`](../src/radio/gm_generator.h).

---

## Security+ 1.0

Chamberlain's older rolling code. The protocol is present in the firmware for
*decoding*, and the stock app can create one through **Add Manually**, but the
generator is not exported to applications — only
`subghz_protocol_secplus_v1_check_fixed`, which validates a fixed part.

Rather than reimplement the encoding and risk producing subtly wrong codes that
fail on a ladder, GarageMate points you at **Sub-GHz → Add Manually →
Security+ 1.0** and then imports the result. Same outcome, no guessing.

---

## Genie / Intellicode

**The official firmware contains no Genie protocol whatsoever.** Not a decoder,
not an encoder, not a name:

```bash
strings ~/.ufbt/current/firmware.elf | grep -ci genie
# 0
```

So there is no generator to call and no protocol name to hand to
`subghz_transmitter_alloc_init`. This is not a limitation GarageMate can code
around — the capability does not exist in the firmware it runs on.

Intellicode is also a rolling code, so recording one press and replaying it does
not open anything.

### So GarageMate implements Genie itself

Rather than declare Genie unsupported, GarageMate carries its own Genie protocol
(the firmware has none) and uses it to **learn codes from a remote you own and
replay them** — the "become your own remote" approach. The 16-bit cyclic
sequence and the forward-resync window are what make it work. Full write-up,
including the protocol port and its honest limits, is in [GENIE.md](GENIE.md).

If you'd rather not: a genuine Genie remote/keypad is ~$20–35, and pre-1995
Genie units use DIP switches (pick "Fixed-code gate" for those).

---

## Frequencies and region

Two independent checks must both pass before the radio transmits:

1. **Hardware band** — the CC1101 supports roughly 300–348, 387–464 and
   779–928 MHz.
2. **Region policy** — if the Flipper has been provisioned with region data
   (`/int/.region_data`), that table narrows it further.

A **US**-provisioned device permits about 304–322 MHz, 433.05–434.79 MHz and
915–928 MHz. Measured on hardware:

| Frequency | Result |
| --- | --- |
| 315.00 MHz | transmits |
| 390.00 MHz | refused by region |
| 433.92 MHz | transmits |

390 MHz being excluded is awkward, because plenty of Chamberlain and LiftMaster
openers use it. In practice Security+ 2.0 receivers listen on 310/315/390
simultaneously, so pairing at **315 MHz** usually works anyway.

GarageMate checks both conditions up front and labels unusable frequencies
`(blocked)` in the picker, so a region problem shows up during setup instead of
as a mystery failure at the opener.

> A note on the region check itself: `furi_hal_region_is_frequency_allowed()`
> returns `false` for *everything* when no region data is present, so calling it
> unconditionally silently blocks all transmission. GarageMate consults it only
> when `furi_hal_region_get()` is non-NULL, and otherwise trusts the hardware
> band check.

---

## Unlocking frequencies

**Settings → All frequencies → ON.** No custom firmware required.

### Why it works

Region enforcement in the official firmware is one table lookup and nothing
else. In `targets/f7/furi_hal/furi_hal_subghz.c`:

```c
// furi_hal_subghz_set_frequency()
if(furi_hal_region_is_frequency_allowed(value)) {
    furi_hal_subghz.regulation = SubGhzRegulationTxRx;
} else {
    furi_hal_subghz.regulation = SubGhzRegulationOnlyRx;   // TX silently dropped
}
```

and both `furi_hal_subghz_set_tx()` and the async TX path refuse when
`regulation != SubGhzRegulationTxRx`. `furi_hal_region_is_frequency_allowed()`
in turn just walks `region->bands[]`.

That table is swappable at runtime, and the setter is **exported to
applications**:

```
Function,+,furi_hal_region_set,void,FuriHalRegion*
```

The firmware even ships an unlocked table of its own — `furi_hal_region_zero`,
country code `"00"`, spanning 0–1000 MHz — for developer-edition units.

GarageMate installs a table covering the CC1101's real tuning ranges
(300–348, 387–464, 779–928 MHz). That is narrower than `region_zero` on purpose:
there is nothing to gain from advertising frequencies the radio cannot reach.

### Ownership trap

`furi_hal_region_set()` **frees the table it replaces**:

```c
if(furi_hal_dynamic_region) free(furi_hal_dynamic_region);
furi_hal_dynamic_region = region;
```

So saving the original pointer and handing it back later would be a
use-after-free. `gm_region_unlock()` keeps a heap *copy* of the original and
hands that copy back on exit, at which point the HAL frees ours. Each table is
owned by exactly one party at a time. See
[`src/radio/gm_region.c`](../src/radio/gm_region.c).

### Scope and reversibility

- RAM only — never written to flash.
- Restored when GarageMate exits, and cleared by a reboot regardless.
- Applies only to GarageMate. The stock Sub-GHz app still sees the original
  region, so import a signal into GarageMate to send it on a widened band.

### If you want it device-wide

That needs custom firmware. All three of these are open source and remove or
bypass region locking:

| Firmware | Notes |
| --- | --- |
| [Momentum](https://github.com/Next-Flip/Momentum-Firmware) | Continuation of Xtreme, includes most Unleashed features. `MNTM → Protocols → Sub-GHz Bypass Region Lock`, plus `Extend Freq Bands` for 281–361 / 378–481 / 749–962 MHz |
| [Unleashed](https://github.com/DarkFlippers/unleashed-firmware) | Long-running region-free fork |
| [RogueMaster](https://github.com/RogueMaster/flipperzero-firmware-wPlugins) | Unleashed plus a large pile of extras |

Note that the "extended bands" those forks offer go **beyond** what the CC1101
is specified for, which is why they carry hardware-risk warnings. Garage doors
need nothing outside the normal ranges, so the in-app unlock covers this use
case without going there.

---

## RAW captures

`.sub` files with `Protocol: RAW` are recorded sample streams, not protocol
payloads. Transmitting them needs the file-encoder worker, which GarageMate does
not drive, so they are rejected at import with a message pointing at the stock
Sub-GHz app — which replays them natively.

Worth knowing: replaying a RAW capture of a *rolling-code* remote will not open
anything, because the opener has already seen that code. RAW replay only helps
for fixed-code gates.
