# Genie Intellicode: learn-and-replay

Genie is the interesting one. Every other brand GarageMate supports is either a
fixed code or has a generator the firmware hands out. Genie has neither — so it
took a different approach, and that approach is the most technically involved
part of this project.

## The constraint

Two hard facts, both verifiable:

1. **The official firmware has no Genie protocol.** Not a decoder, not an
   encoder, not the string:

   ```bash
   strings firmware.elf | grep -ci genie   # 0
   ```

2. **Intellicode's next code cannot be computed.** It is a rolling code whose
   generator lives inside the remote's chip. Nobody has published a way to
   produce code *N+1* from code *N*. Confirmed by the author of the community
   Genie tooling: the only way to get a remote's codes is to capture them from
   the remote itself.

So "clone a captured press" fails (the opener rejects an already-seen code), and
"generate a fresh remote" fails (the algorithm is secret). Both dead ends.

## The opening

Genie's rolling sequence is only **16 bits** and it **wraps** — after 65,536
presses it returns to the start. That is tiny, and it is the whole opportunity.

Rolling-code receivers accept a code that is *ahead* of the last one they saw,
within a forward resync window, and then advance their own counter to it. So if
you hold codes the opener has not seen yet, you can send them in order and the
opener will follow along — exactly as it follows your real remote.

## The approach: become your own remote

This is the [ratgdo](https://github.com/ratgdo) principle — take control of
hardware you own that the vendor won't let you control — but done over the air
instead of by wiring into the opener's terminals:

1. **Learn (away from the garage).** Take your Genie remote out of the opener's
   range and press it next to the Flipper. GarageMate decodes each transmission
   and stores the code. Because the opener never heard these presses, every
   stored code is one it has not seen — i.e. still "ahead".

2. **Replay (at the garage).** Press OPEN. GarageMate sends the next stored
   code. The opener sees a small forward jump, accepts it, and resyncs its
   counter to that code.

3. **Top up.** Each OPEN spends one stored code. When the batch runs low, learn
   again. Your real remote keeps working the whole time — you and it advance
   through the same cyclic sequence, so you stay roughly in step.

No wiring, no soldering, no multi-day capture. You are duplicating the forward
codes of a remote you own, on a door you own, and the opener treats them exactly
as it treats that remote.

## What GarageMate had to build

The firmware provides no Genie support, so GarageMate carries its own protocol
in [`src/protocols/gm_genie.c`](../src/protocols/gm_genie.c):

- **Decoder** — an OOK state machine (200 µs short / 400 µs long symbols, 64-bit
  frames, tolerant of the few extra status bits some remotes append) that turns
  raw radio edges into a code. This is the part that reads real hardware and is
  ported faithfully from Derek Jamison's Genie Recorder (MIT) rather than
  re-derived by guessing.
- **Encoder** — lays the same waveform back down for a stored code, replayed
  verbatim.
- **A registry and a receive chain** — the firmware's protocol registry has no
  Genie entry, so GarageMate builds its own registry
  ([`gm_registry.c`](../src/protocols/gm_registry.c)) and stands up a
  worker + receiver to run the decoder over live radio
  ([`gm_capture.c`](../src/radio/gm_capture.c)).

What was deliberately **left out** of the port: the upstream app's `.gne` files,
its thread-name checks, and its next-code arithmetic. GarageMate stores exact
captured codes in its own sequence file and manages playback itself, so the
protocol only ever moves a raw 64-bit value on and off the air.

The captured run lives at
`/ext/apps_data/garagemate/genie/<door_id>.genie` — a plain FlipperFormat file
with the frequency, the playback cursor (`Next`), and the codes. Inspect or back
it up like any other.

## Honest limitations

- **You must own a Genie remote.** This duplicates your remote; it does not
  break Intellicode or conjure access you don't have.
- **Batch size = presses before re-learning.** Capture more codes for more
  presses between learns (up to `GM_GENIE_MAX_CODES`).
- **Over-the-air decode needs a clean signal.** Hold the remote close; a noisy
  capture just means fewer codes stored that session.
- **Verified in this repo:** the decoder/encoder compile and register; the
  capture screen brings the receiver up and tears it down cleanly on real
  hardware; a stored code transmits and advances the sequence. **Not verified:**
  decoding a live Genie remote and operating a real Genie opener — that needs the
  physical remote and door, which is your step.

## If you'd rather not

- A genuine Genie remote or wireless keypad is ~$20–35 and always works.
- Pre-1995 Genie units use DIP switches, not Intellicode — pick "Fixed-code
  gate" in GarageMate for those.
