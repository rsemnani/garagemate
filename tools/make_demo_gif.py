#!/usr/bin/env python3
"""Render the pairing walkthrough as an animated GIF for the README.

These frames are drawn from the app's real screen definitions at the Flipper's
native 128x64 and then scaled up with nearest-neighbour, so the result is a
faithful rendering of the UI rather than a photograph of the device.

    python3 tools/make_demo_gif.py

Requires Pillow (`pip3 install pillow`).
"""

from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    raise SystemExit("Pillow is required: pip3 install pillow")

WIDTH, HEIGHT = 128, 64
SCALE = 5

BACKGROUND = (255, 138, 22)
INK = (24, 20, 16)

# Candidate monospace faces, in preference order; falls back to Pillow's
# built-in bitmap font so this still runs on a bare system.
FONT_PATHS = (
    "/System/Library/Fonts/Menlo.ttc",
    "/System/Library/Fonts/Monaco.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
)


def load_font(size: int):
    for path in FONT_PATHS:
        if Path(path).exists():
            try:
                return ImageFont.truetype(path, size)
            except OSError:
                continue
    return ImageFont.load_default()


# Roughly matches the device's primary/secondary fonts: about 30 characters of
# secondary text fit across the 128px screen.
FONT_PRIMARY = load_font(9)
FONT_SECONDARY = load_font(7)


def new_frame() -> tuple:
    image = Image.new("RGB", (WIDTH, HEIGHT), BACKGROUND)
    return image, ImageDraw.Draw(image)


def wrap(text: str, font, max_width: int) -> list:
    """Greedy word wrap against the rendered pixel width."""
    lines, current = [], ""
    for word in text.split():
        candidate = f"{current} {word}".strip()
        if font.getlength(candidate) <= max_width or not current:
            current = candidate
        else:
            lines.append(current)
            current = word
    if current:
        lines.append(current)
    return lines


def draw_header(draw, title: str) -> None:
    draw.text((2, 1), title, font=FONT_PRIMARY, fill=INK)
    draw.line((0, 12, WIDTH, 12), fill=INK)


def draw_menu(draw, items: list, selected: int) -> None:
    """Submenu styling: the selected row is inverted, as on the device."""
    for index, label in enumerate(items):
        top = 15 + index * 12
        if top + 11 > HEIGHT:
            break
        if index == selected:
            draw.rounded_rectangle((1, top, WIDTH - 2, top + 10), radius=2, fill=INK)
            draw.text((5, top + 1), label, font=FONT_SECONDARY, fill=BACKGROUND)
        else:
            draw.text((5, top + 1), label, font=FONT_SECONDARY, fill=INK)


def draw_buttons(draw, left=None, center=None, right=None) -> None:
    top = HEIGHT - 11
    if left:
        draw.rounded_rectangle((1, top, 1 + 8 + int(FONT_SECONDARY.getlength(left)), HEIGHT - 1),
                               radius=2, fill=INK)
        draw.text((6, top + 1), left, font=FONT_SECONDARY, fill=BACKGROUND)
    if center:
        span = int(FONT_SECONDARY.getlength(center)) + 12
        x0 = (WIDTH - span) // 2
        draw.rounded_rectangle((x0, top, x0 + span, HEIGHT - 1), radius=2, fill=INK)
        draw.text((x0 + 6, top + 1), center, font=FONT_SECONDARY, fill=BACKGROUND)
    if right:
        span = int(FONT_SECONDARY.getlength(right)) + 12
        draw.rounded_rectangle((WIDTH - 2 - span, top, WIDTH - 2, HEIGHT - 1), radius=2, fill=INK)
        draw.text((WIDTH - span + 3, top + 1), right, font=FONT_SECONDARY, fill=BACKGROUND)


def frame_menu(title: str, items: list, selected: int):
    image, draw = new_frame()
    draw_header(draw, title)
    draw_menu(draw, items, selected)
    return image


def frame_step(title: str, body: str, left=None, center=None, right=None):
    image, draw = new_frame()
    draw_header(draw, title)
    lines = wrap(body, FONT_SECONDARY, WIDTH - 6)
    assert len(lines) <= 4, f"step body needs {len(lines)} lines, only 4 fit: {body!r}"
    for index, line in enumerate(lines):
        draw.text((3, 16 + index * 9), line, font=FONT_SECONDARY, fill=INK)
    draw_buttons(draw, left, center, right)
    return image


def frame_popup(header: str, text: str):
    image, draw = new_frame()
    draw.text((WIDTH // 2, 18), header, font=FONT_PRIMARY, fill=INK, anchor="mm")
    draw.text((WIDTH // 2, 36), text, font=FONT_SECONDARY, fill=INK, anchor="mm")
    return image


def frame_door(name: str, brand: str, bands: str, state: str):
    image, draw = new_frame()
    draw_header(draw, name)
    for index, line in enumerate((brand, bands, state)):
        draw.text((3, 15 + index * 10), line, font=FONT_SECONDARY, fill=INK)
    draw_buttons(draw, center="OPEN", right="More")
    return image


def build_frames() -> list:
    """(image, duration_ms) pairs telling the add-a-door story."""
    return [
        (frame_menu("Your doors", ["Left bay", "Side gate", "Add a door", "Settings"], 2), 1500),
        (frame_menu("Pick your opener",
                    ["Chamberlain / LiftMaster", "Chamberlain (older)",
                     "Chamberlain (vintage)", "Genie / Overhead Door"], 0), 1800),
        (frame_menu("Frequency",
                    ["All bands (3 of 3)", "315.00 MHz", "310.00 MHz", "390.00 MHz"], 0), 1800),
        (frame_step("1. Find LEARN  (1/4)",
                    "Look for a square LEARN button next to the antenna wire. "
                    "On Security+ 2.0 it is YELLOW.",
                    right="Next"), 2400),
        (frame_step("2. Press LEARN  (2/4)",
                    "Press and release LEARN once. Do not hold it. The LED beside "
                    "it turns on.",
                    left="Back", right="Next"), 2400),
        (frame_step("3. Transmit  (3/4)",
                    "Press OK to send the new remote code on 315, 310 and 390 MHz.",
                    left="Back", center="Send", right="Next"), 2400),
        (frame_popup("Sent", "Left bay"), 1200),
        (frame_step("4. Confirm  (4/4)",
                    "The opener lights flash once. That means the remote was "
                    "accepted.",
                    left="Back", right="Done"), 2200),
        (frame_door("Left bay", "Chamberlain / LiftMaster", "All 3 bands",
                    "Rolling code, sent 2"), 2600),
    ]


def main() -> None:
    frames = build_frames()
    scaled = [
        image.resize((WIDTH * SCALE, HEIGHT * SCALE), Image.NEAREST) for image, _ in frames
    ]

    out = Path(__file__).resolve().parent.parent / "docs" / "walkthrough.gif"
    out.parent.mkdir(parents=True, exist_ok=True)
    scaled[0].save(
        out,
        save_all=True,
        append_images=scaled[1:],
        duration=[duration for _, duration in frames],
        loop=0,
        optimize=True,
    )
    print(f"wrote {out} ({len(frames)} frames, {out.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
