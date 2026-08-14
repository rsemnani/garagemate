#!/usr/bin/env python3
"""Talk to a Flipper Zero over its USB serial CLI.

Two uses: run CLI commands, and drive the UI by injecting button presses --
which is what makes automated smoke tests on real hardware possible.

    python3 tools/flipper_cli.py cmd "info device" "loader info"
    python3 tools/flipper_cli.py tap ok right down ok
    python3 tools/flipper_cli.py tap ok:long

Requires pyserial (`pip3 install pyserial`). Close qFlipper and any other tool
holding the port first -- the Flipper allows a single CLI session.
"""

import argparse
import glob
import sys
import time

try:
    import serial
except ImportError:
    sys.exit("pyserial is required: pip3 install pyserial")

PROMPT = b">:"
BAUD = 230400

VALID_KEYS = ("up", "down", "left", "right", "ok", "back")


def find_port() -> str:
    """Locate the Flipper's serial device."""
    candidates = glob.glob("/dev/cu.usbmodemflip_*")  # macOS
    candidates += glob.glob("/dev/serial/by-id/*Flipper*")  # Linux
    if not candidates:
        sys.exit("No Flipper found. Pass --port explicitly if it is elsewhere.")
    return sorted(candidates)[0]


def open_session(port: str) -> "serial.Serial":
    ser = serial.Serial(port, BAUD, timeout=0.5)
    time.sleep(0.4)
    ser.write(b"\r\n")
    time.sleep(0.4)
    ser.reset_input_buffer()
    return ser


def run_command(ser, command: str, timeout: float = 6.0) -> str:
    """Send one CLI command and return its output without the echo or prompt."""
    ser.reset_input_buffer()
    ser.write(command.encode() + b"\r\n")
    ser.flush()

    buf = b""
    deadline = time.time() + timeout
    while time.time() < deadline:
        chunk = ser.read(4096)
        if chunk:
            buf += chunk
            if buf.rstrip().endswith(PROMPT):
                break
        else:
            time.sleep(0.05)

    lines = [l for l in buf.decode("utf-8", "replace").splitlines() if l.strip() not in ("", ">:")]
    if lines and command in lines[0]:
        lines = lines[1:]  # drop the echoed command
    return "\n".join(l.rstrip() for l in lines)


def tap(ser, key: str, hold: bool = False, settle: float = 0.45) -> None:
    """Press and release one button.

    ViewDispatcher discards a `short`/`long` event that was not preceded by a
    `press` for the same key ("non-complementary input"), so a lone
    `input send ok short` looks like it worked and does nothing. All three
    events must be sent.
    """
    for event in ("press", "long" if hold else "short", "release"):
        ser.write(f"input send {key} {event}\r\n".encode())
        ser.flush()
        time.sleep(0.15)
        ser.reset_input_buffer()
    time.sleep(settle)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("cmd", "tap"))
    parser.add_argument("args", nargs="+", help="commands, or keys such as 'ok' / 'ok:long'")
    parser.add_argument("--port", help="serial device (auto-detected by default)")
    opts = parser.parse_args()

    # Validate before touching the serial port, so a typo does not require a
    # connected Flipper to discover.
    taps = []
    if opts.mode == "tap":
        for spec in opts.args:
            key, _, modifier = spec.partition(":")
            if key not in VALID_KEYS:
                sys.exit(f"Unknown key '{key}'. Expected one of: {', '.join(VALID_KEYS)}")
            if modifier not in ("", "long"):
                sys.exit(f"Unknown modifier '{modifier}'. Only ':long' is supported.")
            taps.append((key, modifier == "long"))

    with open_session(opts.port or find_port()) as ser:
        if opts.mode == "cmd":
            for command in opts.args:
                print(f"===== {command} =====")
                print(run_command(ser, command))
                print()
            return

        for key, hold in taps:
            tap(ser, key, hold=hold)
            print(f"tap {key}{' (long)' if hold else ''}")


if __name__ == "__main__":
    main()
