#!/usr/bin/env python3
"""Validate and summarise the opt-in RD filter diagnostic without serial access."""
import argparse
import json
from pathlib import Path
import re
from compare_sequences import load_capture
from analyse_log import RD_ROW

LIMIT = 30000
MASK = 0xffffffff


def fields(line):
    return {k: int(v) for k, v in re.findall(r'(\w+)=(-?\d+)', line)}


def delta(current, previous):
    return (current - previous) % LIMIT


def moved(sample):
    return delta(sample['rawAfter'], sample['raw']) != 0 or delta(
        sample['filteredAfter'], sample['filtered']) != 0


def validate_pair(previous, current):
    if delta(current['raw'], previous['raw']) != current['rawDelta']:
        raise ValueError('raw delta does not match its own counter')
    if delta(current['filtered'], previous['filtered']) != current['filteredDelta']:
        raise ValueError('filtered delta does not match its own counter')
    if (moved(previous) or moved(current)) and not current['ambiguous']:
        raise ValueError('unflagged read-window movement')


def analyse(path):
    capture = load_capture(path)
    text = path.read_text(errors='replace')
    startup = re.findall(r'^RD FILTER COMPARE enabled: (.+)$', text, re.M)
    if len(startup) != 1:
        raise ValueError('missing or repeated RD filter startup')
    config = fields(startup[0])
    for key, value in dict(rawUnit=2, filteredUnit=3, pin=26, limit=LIMIT,
                           cycles=80, readback=80, enabled=1, rawFilterEnabled=0).items():
        if config.get(key) != value:
            raise ValueError(f'bad filter readback/configuration: {key}')
    if config.get('APB_Hz') != 80000000 or config.get('settleCpuCycles') != 240:
        raise ValueError('unexpected APB/CPU clock; review timing before continuing')
    summaries = re.findall(r'^RD FILTER SUMMARY (.+)$', text, re.M)
    if len(summaries) != 1 or text.count('RD FILTER END') != 1:
        raise ValueError('missing or truncated filter summary')
    summary = fields(summaries[0])
    if summary['eventsSaved'] != min(16, summary['eventsTotal']) or summary['eventsTotal'] != summary['disagree']:
        raise ValueError('invalid disagreement retention totals')
    rows = [fields(line) for line in re.findall(r'^RD FILTER ROW .+$', text, re.M)]
    rawrows = list(RD_ROW.finditer(text))
    if not rows or len(rows) != len(rawrows) or len(rows) != min(64, summary['rows']):
        raise ValueError('incomplete filtered row history')
    anchor = re.findall(r'^RD FILTER ANCHOR .+$', text, re.M)
    if len(anchor) != 1:
        raise ValueError('missing alignment anchor')
    samples = [fields(anchor[0])] + rows
    samples += [fields(line) for line in re.findall(r'^RD FILTER EVENT(?:_PREV)? .+$', text, re.M)]
    for c in samples:
        order = ['rawStart', 'rawEnd', 'levelEnd', 'filteredStart', 'filteredEnd',
                 'settledStart', 'settledEnd', 'rawAfterEnd']
        offsets = [(c[k] - c['rawStart']) & MASK for k in order]
        if offsets != sorted(offsets) or offsets[-1] > 100000:
            raise ValueError('invalid snapshot ordering or excessive read window')
        if ((c['settledStart'] - c['rawEnd']) & MASK) < config['settleCpuCycles']:
            raise ValueError('filtered confirmation occurred before settling interval')
    for i, (c, raw) in enumerate(zip(rows, rawrows)):
        index, slot, expected, rclk, dclk, rd, level, notify, isr, sample, failed = map(int, raw.groups())
        if (c['index'], c['expected'], c['raw'], c['level'], c['isrCycles']) != (index, expected, rd, level, isr):
            raise ValueError('raw and supplemental history do not correspond')
        if ((c['levelEnd'] - isr) & MASK) != sample:
            raise ValueError('original GPIO timing not preserved in trace')
        if i:
            validate_pair(rows[i-1], c)
    prev_events = re.findall(r'^RD FILTER EVENT_PREV .+$', text, re.M)
    events = re.findall(r'^RD FILTER EVENT .+$', text, re.M)
    if len(events) != summary['eventsSaved'] or len(prev_events) != len(events):
        raise ValueError('missing disagreement event entries')
    for before, after in zip(prev_events, events):
        validate_pair(fields(before), fields(after))
    final = re.search(r'V4c FINAL frames=(\d+)/4096 processed=(\d+) rows=(\d+)', text)
    frames, processed, accepted = map(int, final.groups())
    failing = bool(int(rawrows[-1].groups()[-1]))
    if summary['rows'] != accepted + int(failing):
        raise ValueError('summary does not cover every sampled row')
    if summary['markers'] != summary['rows'] // 32:
        raise ValueError('expected marker total does not match sampled rows')
    if capture.outcome == 'PASS' and (summary['rows'] != 131072 or summary['markers'] != 4096):
        raise ValueError('pass has incomplete comparison totals')
    result = dict(outcome=capture.outcome, frames=frames,
                  maxReopen=int(re.search(r'Handover: .*?maxReopen=(\d+) us', text).group(1)),
                  config=config, summary=summary,
                  comparable_rd_stop=capture.outcome == 'RD phase mismatch' and failing
                      and not rows[-1]['ambiguous'] and summary['bothMarkerOK'] == summary['markers'],
                  repeated_marker_disagreement=(summary['markers'] - summary['bothMarkerOK']) >= 2)
    if failing:
        result['failure'] = dict(previous_raw=rawrows[-2].group(0) if len(rawrows)>1 else None,
                                 failing_raw=rawrows[-1].group(0),
                                 previous_filter=rows[-2] if len(rows)>1 else None,
                                 failing_filter=rows[-1],
                                 row=rows[-1]['index'], slot=rows[-1]['index'] % 32)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    print(json.dumps(analyse(args.log), indent=2))


if __name__ == '__main__':
    main()
