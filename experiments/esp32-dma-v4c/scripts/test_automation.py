"""Hardware-free checks of capture bounds, cleanup, identity and log summaries."""
import argparse
import io
from types import SimpleNamespace
import unittest

from capture import collect, duration_value
from upload import verify_device
from analyse_log import summarise


class FakePort:
    def __init__(self, fail=None):
        self.now = 0.0
        self.is_open = False
        self.fail = fail
        self.in_waiting = 0
        self.reads = 0
        self.payload = b'V4c STARTUP [527 ms] 01 setup entered; Serial ready\n'
        self.dtr = False
        self.rts = False

    def open(self):
        self.is_open = True
        if self.fail == 'open':
            raise OSError('simulated open failure')

    def read(self, size):
        self.reads += 1
        self.now += self.timeout
        if self.reads == 2 and self.fail:
            if self.fail == 'interrupt':
                raise KeyboardInterrupt()
            raise OSError('simulated disconnect')
        return self.payload if self.reads == 1 else b''

    def close(self):
        self.is_open = False

    def sleep(self, seconds):
        self.now += seconds
        if self.fail == 'reset':
            raise OSError('simulated reset failure')


class AutomationTests(unittest.TestCase):
    def test_bounded_capture_and_raw_startup(self):
        for duration in (0.01, 0.45, 45):
            with self.subTest(duration=duration):
                port, output = FakePort(), io.BytesIO()
                collect(port, output, duration, clock=lambda: port.now, sleep=port.sleep)
                self.assertAlmostEqual(port.now, duration)
                self.assertEqual(output.getvalue(), port.payload)
                self.assertFalse(port.is_open)

    def test_reset_and_cleanup(self):
        port = FakePort()
        collect(port, io.BytesIO(), 0.45, True, lambda: port.now, port.sleep)
        self.assertFalse(port.rts)
        self.assertFalse(port.dtr)
        self.assertFalse(port.is_open)
        self.assertAlmostEqual(port.now, 0.45)

    def test_errors_preserve_output_and_close(self):
        for failure in ('read', 'interrupt', 'open', 'reset'):
            with self.subTest(failure=failure):
                port, output = FakePort(failure), io.BytesIO()
                with self.assertRaises((OSError, KeyboardInterrupt)):
                    collect(port, output, 1, failure == 'reset',
                            lambda: port.now, port.sleep)
                self.assertFalse(port.is_open)
                self.assertFalse(port.rts)
                if failure in ('read', 'interrupt'):
                    self.assertEqual(output.getvalue(), port.payload)

    def test_duration_validation(self):
        for value in ('0', '-1', 'nan', 'inf'):
            with self.assertRaises(argparse.ArgumentTypeError):
                duration_value(value)

    def test_usb_identity(self):
        device = SimpleNamespace(device='/dev/test', vid=0x10C4,
                                 pid=0xEA60, serial_number='02073BE4')
        self.assertIs(verify_device('/dev/test', [device]), device)
        with self.assertRaises(ValueError):
            verify_device('/dev/missing', [device])
        device.serial_number = 'OTHER'
        with self.assertRaises(ValueError):
            verify_device('/dev/test', [device])

    def test_failure_summary_and_pass_requirement(self):
        raw = (b'V4c FINAL frames=1914/4096 processed=1914\n'
               b'STOP: RD phase mismatch\n')
        summary = summarise(raw)
        self.assertIn('passing test: no', summary)
        self.assertIn('STOP: RD phase mismatch', summary)
        final = b'V4c FINAL frames=4096/4096 processed=4096\n'
        self.assertIn('passing test: no', summarise(final))
        self.assertIn('passing test: yes', summarise(final + b'RESULT: STATIC STREAM PASS\n'))
        self.assertIn('passing test: no', summarise(
            final + b'RESULT: STATIC STREAM PASS\n' + final + b'STOP: validation failed\n'))


if __name__ == '__main__':
    unittest.main()
