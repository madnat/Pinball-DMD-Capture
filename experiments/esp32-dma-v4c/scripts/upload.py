#!/usr/bin/env python3
"""Verify the TTGO USB identity before an explicitly requested upload."""
import argparse
import os
from pathlib import Path
import subprocess

PIO = os.environ.get('PIO', 'pio')
ROOT = Path(__file__).resolve().parents[1]


def verify_device(port, devices):
    matches = [d for d in devices if d.device == port]
    if len(matches) != 1:
        raise ValueError(f'Expected exactly one device at {port}; found {len(matches)}')
    device = matches[0]
    if (device.vid, device.pid, device.serial_number) != (0x10C4, 0xEA60, '02073BE4'):
        raise ValueError(f'USB identity mismatch at {port}; upload refused')
    return device


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True, help='Explicit enumerated serial path')
    parser.add_argument('--environment', choices=('ttgo_tdisplay', 'ttgo_tdisplay_rd_trace',
                                 'ttgo_tdisplay_attract_sequence', 'ttgo_tdisplay_rd_filter_compare', 'ttgo_tdisplay_filtered_validate',
                                 'ttgo_tdisplay_filtered_recovery', 'ttgo_tdisplay_filtered_recovery_slot20',
                                 'ttgo_tdisplay_dmd_replica', 'ttgo_tdisplay_dmd_replica_fault_test', 'ttgo_tdisplay_dmd_replica_continuous',
                                 'ttgo_tdisplay_dmd_replica_freezy', 'ttgo_tdisplay_dmd_replica_ws'),
                        default='ttgo_tdisplay', help='PlatformIO firmware environment')
    parser.add_argument('--confirm-upload', action='store_true',
                        help='Required: confirm this invocation is an authorized flash')
    args = parser.parse_args()
    if not args.confirm_upload:
        parser.error('--confirm-upload is required; obtain explicit approval first')
    from serial.tools import list_ports
    try:
        device = verify_device(args.port, list_ports.comports())
    except ValueError as error:
        parser.exit(2, f'{error}\n')
    print(f'Verified {device.device}: {device.description}; '
          f'USB 10C4:EA60 serial={device.serial_number}', flush=True)
    return subprocess.call([PIO, 'run', '-e', args.environment, '-t', 'upload',
                            '--upload-port', args.port], cwd=ROOT)


if __name__ == '__main__':
    raise SystemExit(main())
