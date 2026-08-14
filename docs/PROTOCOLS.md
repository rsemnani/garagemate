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

### What actually works

1. **A genuine Genie remote or wireless keypad.** Around $20–35, works
   perfectly, and pairs with the same LEARN-button dance.
2. **[genie-recorder](https://github.com/jamisonderek/flipper-zero-tutorials/tree/main/subghz/apps/genie-recorder)**
   — a separate Flipper app that implements the Genie protocol in the app rather
   than the firmware. You press the button on a Genie remote you already own
   while it records rolling codes; it can then send the next code in the
   sequence. Note the shape of this: it *borrows* an existing remote's sequence.
   It does not pair a new remote, and it needs the original in hand.
3. **Check whether it is actually Intellicode.** Genie units from before roughly
   1995 use DIP switches. If the remote has a row of tiny switches, it is a
   fixed code — choose "Fixed-code gate" in GarageMate and it will pair
   normally.

Third-party firmware forks have carried Genie patches at various times; if you
go that route, verify support in the fork you install rather than assuming.

---

## Frequencies and region

Two independent checks must both pass before the radio transmits:

1. **Hardware band** — the CC1101 supports roughly 300–348, 387–464 and
   779–928 MHz.
2. **Region policy** — if the Flipper has been provisioned with region data
   (`/int/.region_data`), that table narrows it further.

This device is provisioned **US**, which permits about 304–322 MHz,
433.05–434.79 MHz and 915–928 MHz. Measured on the hardware:

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

## RAW captures

`.sub` files with `Protocol: RAW` are recorded sample streams, not protocol
payloads. Transmitting them needs the file-encoder worker, which GarageMate does
not drive, so they are rejected at import with a message pointing at the stock
Sub-GHz app — which replays them natively.

Worth knowing: replaying a RAW capture of a *rolling-code* remote will not open
anything, because the opener has already seen that code. RAW replay only helps
for fixed-code gates.
