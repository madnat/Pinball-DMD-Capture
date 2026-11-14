#!/usr/bin/env python3
"""Read-only V4c log summary; CRC changes alone do not imply corruption."""
import argparse
from pathlib import Path
import re


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
