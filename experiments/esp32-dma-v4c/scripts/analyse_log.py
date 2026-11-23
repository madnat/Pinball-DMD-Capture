#!/usr/bin/env python3
"""Read-only V4c log summary; CRC changes alone do not imply corruption."""
import argparse
from pathlib import Path
import re


RD_ROW = re.compile(
    r'RD ROW index=(\d+) slot=(\d+) expected=([01]) RCLK=(\d+) DCLK=(\d+) '
    r'RD=(\d+) level=([01]) notify=(\d+) isrCycles=(\d+) '
    r'sampleEndAfterISR=(\d+) failed=([01])'
)


def rd_history_summary(final_text):
    rows = [tuple(map(int, row.groups())) for row in RD_ROW.finditer(final_text)]
    if not rows or not rows[-1][-1]:
        return []
    index, slot, expected, rclk, dclk, rd, level, notify, _, sample, _ = rows[-1]
    result = [f'RD trace failure: row index={index}, slot={slot} (zero-based), '
              f'expected RD level/edge={expected}, sampled level={level}, '
              f'notifications={notify}, sample end={sample} cycles after RCLK ISR timestamp.']
    if len(rows) > 1 and rows[-2][0] + 1 == index:
        previous = rows[-2]
        # These are the same modulo-30000 hardware counters used by the firmware.
        deltas = tuple((current - old) % 30000
                       for current, old in zip((rclk, dclk, rd), previous[3:6]))
        result.append(f'RD trace deltas RCLK/DCLK/RD={deltas[0]}/{deltas[1]}/{deltas[2]}; '
                      f'sampled RD level {previous[6]}->{level}.')
    return result


def summarise(raw):
    text = raw.decode('utf-8', errors='replace')
    lines = text.splitlines()
    frames = list(re.finditer(r'V4c FINAL frames=(\d+)/(\d+) processed=(\d+)', text))
    final_text = text[frames[-1].start():] if frames else ''
    passed = bool(re.search(r'^RESULT: .*STREAM PASS', final_text, re.MULTILINE))
    failed = bool(re.search(r'^(?:V4c )?STOP:', final_text, re.MULTILINE))
    complete = bool(frames and frames[-1].groups() == ('4096', '4096', '4096')
                    and passed and not failed)
    result = [f'Startup captured: {"yes" if "01 setup entered; Serial ready" in text else "no"}',
              f'Complete 4096-frame passing test: {"yes" if complete else "no"}']
    if len(frames) > 1:
        result.append('Multiple FINAL records; pass status describes the last FINAL only.')
    prefixes = ('V4c FINAL ', 'Handover:', 'Timing:', 'Last deltas:', 'STOP:',
                'V4c STOP:', 'RESULT:', 'Guru Meditation', 'Brownout', 'Backtrace:',
                'Saved final:', 'Saved handover:', 'Saved timing:', 'Saved deltas:')
    seen = set()
    for line in lines:
        if line.startswith(prefixes) and line not in seen:
            result.append(line)
            seen.add(line)
    result.extend(rd_history_summary(final_text))
    crc = [line for line in lines if 'firstCRC=' in line and 'lastCRC=' in line]
    if crc:
        result += ['First CRC report: ' + crc[0], 'Last CRC report: ' + crc[-1],
                   'Last CRC report may precede the final frame; MOVING CRC changes are expected.']
    if not frames:
        result.append('No full FINAL record: capture may be late, incomplete, or startup failed.')
    return '\n'.join(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    print(summarise(args.log.read_bytes()))


if __name__ == '__main__':
    main()
