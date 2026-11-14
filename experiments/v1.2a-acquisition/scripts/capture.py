#!/usr/bin/env python3
"""Save raw serial bytes for a bounded window; reset only with --reset."""
import argparse
from datetime import datetime
import json
import math
from pathlib import Path
import time

ROOT = Path(__file__).resolve().parents[1]


def duration_value(value):
    seconds = float(value)
    if not math.isfinite(seconds) or seconds <= 0:
        raise argparse.ArgumentTypeError('duration must be a finite positive number')
    return seconds


def collect(port, output, duration, reset=False, clock=time.monotonic, sleep=time.sleep):
    """Port is configured but unopened. Always close, including on reset/read errors."""
    started = clock()
    deadline = started + duration
    try:
        port.open()
        if reset:
            # GPIO0 remains deasserted (DTR false); pulse EN using RTS.
            port.dtr = False
            port.rts = True
            try:
                sleep(min(0.1, max(0, deadline - clock())))
            finally:
                port.rts = False
        while clock() < deadline:
            port.timeout = min(0.2, max(0, deadline - clock()))
            data = port.read(min(65536, max(1, port.in_waiting)))
            if data:
                output.write(data)
                output.flush()
    finally:
        port.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/cu.usbserial-02073BE4')
    parser.add_argument('--duration', type=duration_value, default=45.0, metavar='SECONDS')
    parser.add_argument('--reset', action='store_true', help='Explicit RTS reset after opening')
    args = parser.parse_args()
    from serial import Serial
    directory = ROOT / 'diagnostics'
    directory.mkdir(exist_ok=True)
    stamp = datetime.now().astimezone().strftime('%Y%m%d_%H%M%S_%f%z')
    path = directory / f'ttgo_serial_{stamp}.txt'
    port = Serial(port=None, baudrate=115200, timeout=0.2, exclusive=True)
    port.port = args.port
    # Set desired states before opening. Some drivers may still glitch control lines.
    port.dtr = False
    port.rts = False
    metadata = dict(port=args.port, baud=115200, duration_seconds=args.duration,
                    reset_requested=args.reset, started=datetime.now().astimezone().isoformat())
    started = time.monotonic()
    status = 0
    print(f'Capture: {path}', flush=True)
    try:
        with path.open('xb') as output:
            collect(port, output, args.duration, args.reset)
    except KeyboardInterrupt:
        metadata['error'] = 'Interrupted by user'
        status = 130
    except Exception as error:
        metadata['error'] = f'{type(error).__name__}: {error}'
        status = 1
    finally:
        port.close()
        raw = path.read_bytes() if path.exists() else b''
        metadata.update(elapsed_seconds=round(time.monotonic() - started, 3),
                        bytes=len(raw), port_closed=not port.is_open,
                        startup_captured=b'PINBALL DMD V1.2g' in raw,
                        boot_banner_captured=b'boot:' in raw)
        path.with_suffix('.json').write_text(json.dumps(metadata, indent=2) + '\n')
        print(json.dumps(metadata, indent=2), flush=True)
    return status


if __name__ == '__main__':
    raise SystemExit(main())
