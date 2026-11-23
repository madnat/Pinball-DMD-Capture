import contextlib
import io
from pathlib import Path
import tempfile
import unittest

from compare_sequences import Capture, load_capture, report


class SequenceTests(unittest.TestCase):
    def test_dump_validation(self):
        text = (
            'ATTRACT SEQUENCE enabled: test\nmode=MOVING\n'
            'V4c FINAL frames=1/4096 processed=1 rows=33\nSTOP: RD phase mismatch\n'
            'SEQ BEGIN frames=1\nSEQ CRC 1 B2AA7578\n'
            f'SEQ PIXELS 1 {"00" * 512}\nSEQ END frames=1\n'
        )
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'capture.txt'
            path.write_text(text)
            self.assertEqual(load_capture(path).crcs, [0xB2AA7578])
            path.write_text(text.replace('B2AA7578', '00000000'))
            with self.assertRaisesRegex(ValueError, 'pixel payload'):
                load_capture(path)
            path.write_text(text.split('SEQ END')[0])
            with self.assertRaisesRegex(ValueError, 'truncated'):
                load_capture(path)

    def test_repeated_content_with_different_dwell(self):
        captures = [
            Capture(Path('failed.txt'), 'RD phase mismatch', [1, 1, 2, 3], []),
            Capture(Path('failed2.txt'), 'Row continuity mismatch', [1, 2, 2, 3], []),
            Capture(Path('passed.txt'), 'PASS', [1, 2, 3, 3, 4], []),
        ]
        with contextlib.redirect_stdout(io.StringIO()) as output:
            report(captures, 3)
        self.assertIn('passed.txt: 1 match(es) followed by another content change, 0',
                      output.getvalue())
        self.assertIn('failed2.txt: 0 match(es) followed by another content change, 1',
                      output.getvalue())


if __name__ == '__main__':
    unittest.main()
