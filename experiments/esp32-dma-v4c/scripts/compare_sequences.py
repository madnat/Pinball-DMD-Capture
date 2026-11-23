#!/usr/bin/env python3
"""Compare display refresh sequences preceding V4c stops with other captures."""

import argparse
from dataclasses import dataclass
from itertools import groupby
from pathlib import Path
import re
import zlib

from analyse_log import rd_history_summary


@dataclass
class Capture:
    path: Path
    outcome: str
    crcs: list[int]
    row_summary: list[str]


def load_capture(path, allow_recovery=False, allow_replica=False):
    text = path.read_text(encoding='utf-8', errors='replace')
    if text.count('ATTRACT SEQUENCE enabled:') != 1 or text.count('V4c FINAL ') != 1:
        raise ValueError('expected one sequence build startup and one FINAL')
    startup, final = text.split('V4c FINAL ', 1)
    if 'mode=MOVING' not in startup:
        raise ValueError('missing MOVING startup')
    counts = re.match(r'frames=(\d+)/(\d+) processed=(\d+) ', final)
    if not counts:
        raise ValueError('missing FINAL frame counts')
    frames, target, processed = map(int, counts.groups())
    if not 0 <= frames == processed <= target == 4096:
        raise ValueError('incomplete or invalid worker frame counts')
    result = re.findall(r'^(STOP|RESULT): (.+)$', final, re.MULTILINE)
    if len(result) != 1:
        raise ValueError('expected one STOP or RESULT')
    kind, outcome = result[0]
    if kind == 'RESULT':
        if frames != target:
            raise ValueError('unrecognised or incomplete pass')
        if outcome.startswith('MOVING COUNTS/RD STREAM PASS'):
            outcome = 'PASS'
        elif allow_recovery and 'AUTO RECOVERY enabled:' in startup and outcome.startswith('RECOVERY VALIDATED OUTPUT PASS'):
            outcome = 'RECOVERED_PASS'
        elif allow_replica and 'DMD REPLICA enabled:' in startup and outcome.startswith('REPLICA VALIDATED OUTPUT PASS'):
            outcome = 'REPLICA_PASS'
        else:
            raise ValueError('unrecognised or incomplete pass')

    blocks = re.findall(r'^SEQ BEGIN frames=(\d+)\n(.*?)^SEQ END frames=(\d+)$',
                        final, re.MULTILINE | re.DOTALL)
    if len(blocks) != 1 or tuple(map(int, (blocks[0][0], blocks[0][2]))) != (frames, frames):
        raise ValueError('missing or truncated sequence dump')
    crcs = []
    pixels = []
    for line in blocks[0][1].splitlines():
        fields = line.split()
        if len(fields) < 4 or fields[0] != 'SEQ':
            raise ValueError('malformed sequence line')
        index, values = int(fields[2]), fields[3:]
        if fields[1] == 'CRC' and index == len(crcs) + 1 and not pixels:
            if len(values) > 16 or any(not re.fullmatch(r'[0-9A-F]{8}', v) for v in values):
                raise ValueError('invalid CRC')
            crcs.extend(int(v, 16) for v in values)
        elif fields[1] == 'PIXELS' and len(values) == 1 and len(crcs) == frames:
            expected = max(1, frames - 31) + len(pixels)
            if index != expected or index > frames or len(values[0]) != 1024:
                raise ValueError('unexpected pixel frame')
            if zlib.crc32(bytes.fromhex(values[0])) != crcs[index - 1]:
                raise ValueError('pixel payload does not match its CRC')
            pixels.append(index)
        else:
            raise ValueError('out-of-order sequence data')
    if len(crcs) != frames or len(pixels) != min(32, frames):
        raise ValueError('missing CRC or pixel records')
    return Capture(path, outcome, crcs, rd_history_summary(final))


def content_runs(crcs):
    """Collapse adjacent identical refreshes, retaining dwell in refreshes."""
    return [(crc, sum(1 for _ in group)) for crc, group in groupby(crcs)]


def report(captures, length):
    timelines = [content_runs(capture.crcs) for capture in captures]
    for capture, timeline in zip(captures, timelines):
        print(f'{capture.path}: {capture.outcome}, {len(capture.crcs)} refreshes, '
              f'{len(timeline)} content runs')
        for line in capture.row_summary:
            print(f'  {line}')
    for index, capture in enumerate(captures):
        if capture.outcome == 'PASS':
            continue
        runs = timelines[index]
        print(f'Before {capture.outcome} in {capture.path.name}')
        if len(runs) < length:
            print(f'  Fewer than {length} content runs available for comparison.')
            continue
        suffix = runs[-length:]
        print('  ' + ' -> '.join(f'{crc:08X} x{dwell}' for crc, dwell in suffix))
        print('  The last dwell may end early because capture stopped.')
        needle = [crc for crc, _ in suffix]
        for other, timeline in enumerate(timelines):
            signatures = [crc for crc, _ in timeline]
            matches = [start for start in range(len(signatures) - length + 1)
                       if signatures[start:start + length] == needle]
            continued = sum(start + length < len(signatures) for start in matches)
            other_terminal = int(other != index and len(signatures) - length in matches)
            label = 'same capture' if other == index else captures[other].path.name
            print(f'  {label}: {continued} match(es) followed by another content change, '
                  f'{other_terminal} other terminal match(es).')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('logs', nargs='+', type=Path)
    parser.add_argument('--length', type=int, default=3, help='Number of CRC runs to match')
    args = parser.parse_args()
    if args.length < 2:
        parser.error('--length must be at least 2')
    captures = []
    invalid = False
    for path in dict.fromkeys(path.resolve() for path in args.logs):
        try:
            captures.append(load_capture(path))
        except (OSError, ValueError) as error:
            print(f'Skipping {path}: {error}')
            invalid = True
    report(captures, args.length)
    return int(invalid)


if __name__ == '__main__':
    raise SystemExit(main())
