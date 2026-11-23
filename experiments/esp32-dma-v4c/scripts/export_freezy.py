#!/usr/bin/env python3
"""Convert a complete TTGO FREEZY2 serial capture into a verified Freezy text dump."""
import argparse
import binascii
from dataclasses import dataclass
from pathlib import Path
import re

WIDTH, HEIGHT = 128, 32
PIXELS = WIDTH * HEIGHT
ROW = re.compile(rb"[0-3]{128}\r\n")
STAMP = re.compile(rb"0x[0-9A-F]{8}\r\n")
START = re.compile(rb"(?m)^FREEZY2 START\r?$")
PACKET = re.compile(rb"(?m)^FREEZY2 (\d+) (\d+) (\d+) (\d+) (\d+) ([0-9A-F]{8}) ")
END = re.compile(rb"(?m)^FREEZY2 END built=(\d+) sent=(\d+) dropped=(\d+) txErrors=(\d+) inputDrops=(\d+)\r?$")
PACKED_PIXELS = PIXELS // 4
UNPACK = tuple(bytes((byte >> shift) & 3 for shift in (6, 4, 2, 0)) for byte in range(256))
ASCII = bytes.maketrans(bytes(range(4)), b"0123")


@dataclass(frozen=True)
class Frame:
    number: int
    us: int
    epoch: int
    first: int
    last: int
    pixels: bytes


def read_capture(path):
    log = path.read_bytes()
    starts, summaries = list(START.finditer(log)), list(END.finditer(log))
    if len(starts) != 1 or len(summaries) != 1 or starts[0].end() >= summaries[0].start():
        raise ValueError("expected one complete FREEZY2 run with matching start/end markers")
    summary = summaries[0]
    frames, malformed = [], 0
    next_header = starts[0].end()
    for match in PACKET.finditer(log, next_header, summary.start()):
        if match.start() < next_header:
            continue
        start = match.end()
        end = start + PACKED_PIXELS
        if end >= summary.start() or log[end:end + 1] != b"\n":
            malformed += 1
            continue
        pixels = b"".join(UNPACK[byte] for byte in log[start:end])
        if binascii.crc32(pixels) != int(match[6], 16):
            malformed += 1
            continue
        next_header = end + 1
        n, us, epoch, first, last = map(int, match.groups()[:5])
        if n < 1 or last != first + 2:
            raise ValueError(f"invalid frame sequence at frame {n}")
        if frames and (n <= frames[-1].number or us <= frames[-1].us
                       or epoch < frames[-1].epoch or first <= frames[-1].last):
            raise ValueError(f"acquisition order reversed at frame {n}")
        frames.append(Frame(n, us, epoch, first, last, pixels))

    if not frames:
        raise ValueError("no CRC-verified FREEZY2 pixel frames")
    summary = tuple(map(int, summary.groups()))
    built, sent, dropped, tx_errors, input_drops = summary
    if sent + dropped + tx_errors != built or frames[-1].number > built or len(frames) > sent:
        raise ValueError("inconsistent device export totals")
    print(f"Run: built={built}, sent={sent}, queue drops={dropped}, TX errors={tx_errors}, "
          f"input drops={input_drops}, host missing/corrupt={sent - len(frames)}, "
          f"malformed/CRC-failed records={malformed}")
    missing = built - len(frames)
    if missing:
        print(f"Run: {missing} completed frames unavailable; "
              f"leading={frames[0].number - 1}, trailing={built - frames[-1].number}")
    return frames


def write_dump(output, frames):
    previous = 999
    origin = frames[0].us
    for frame in frames:
        stamp = 1000 + (frame.us - origin + 500) // 1000
        if not previous < stamp <= 0xFFFFFFFF:
            raise ValueError("timestamps must increase at millisecond precision")
        if len(frame.pixels) != PIXELS or max(frame.pixels) > 3:
            raise ValueError("invalid four-shade image")
        digits = frame.pixels.translate(ASCII)
        output.write(f"0x{stamp:08X}\r\n".encode("ascii"))
        for start in range(0, PIXELS, WIDTH):
            output.write(digits[start:start + WIDTH] + b"\r\n")
        output.write(b"\r\n")
        previous = stamp


def verify_dump(source):
    count, previous = 0, 999
    while line := source.readline():
        if not STAMP.fullmatch(line):
            raise ValueError(f"invalid timestamp syntax at frame {count + 1}")
        stamp = int(line, 16)
        if (count == 0 and stamp != 1000) or stamp <= previous:
            raise ValueError(f"invalid timestamp order at frame {count + 1}")
        for row in range(HEIGHT):
            if not ROW.fullmatch(source.readline()):
                raise ValueError(f"invalid frame {count + 1}, row {row + 1}")
        if source.readline() != b"\r\n":
            raise ValueError(f"missing empty separator at frame {count + 1}")
        count += 1
        previous = stamp
    if not count:
        raise ValueError("empty dump")
    return count, previous - 1000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("capture", type=Path)
    parser.add_argument("--output", type=Path, default=Path("fish-tales-attract.txt"))
    parser.add_argument("--seconds", type=int, choices=range(16, 21), default=18)
    args = parser.parse_args()
    try:
        frames = read_capture(args.capture)
        start = frames[0].us
        clip = [frame for frame in frames if frame.us - start <= args.seconds * 1_000_000]
        if clip[-1].us - start < 15_000_000:
            raise ValueError("capture does not contain a 15-second clip")
        gaps = 0
        for before, after in zip(clip, clip[1:]):
            elapsed = after.us - before.us
            missing = after.number - before.number - 1
            skipped = after.first - before.last - 1
            # A normal three-refresh frame interval is about 24.6 ms.
            if missing or skipped or after.epoch != before.epoch or elapsed > 40_000:
                gaps += 1
                print(f"Gap {before.number}->{after.number}: {elapsed / 1000:.3f} ms, "
                      f"missing completed={missing}, skipped refresh IDs={skipped}, "
                      f"epoch {before.epoch}->{after.epoch}")
        with args.output.open("x+b") as output:
            write_dump(output, clip)
            output.seek(0)
            count, duration = verify_dump(output)
        if count != len(clip):
            raise ValueError("verified frame count differs from export count")
        print(f"{args.output.resolve()}: {count} frames, {duration / 1000:.3f} s, "
              f"{gaps} detected gaps (first-to-last timestamp, verified)")
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
