import binascii
import io
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest

from export_freezy import Frame, read_capture, verify_dump, write_dump


class FreezyTests(unittest.TestCase):
    def test_four_bands_with_msb_first_transport(self):
        pixels = bytes((0, 1, 2, 3)) * 32 + b"".join(
            bytes([shade]) * 128 * rows for shade, rows in enumerate((7, 8, 8, 8))
        )
        packed = bytes(sum(value << shift for value, shift in zip(pixels[i:i+4], (6, 4, 2, 0)))
                       for i in range(0, len(pixels), 4))
        with TemporaryDirectory() as directory:
            path = Path(directory) / "serial.txt"
            with path.open("wb") as log:
                log.write(b"FREEZY2 START\r\n")
                for n, us in enumerate((12100, 36720, 61190), 1):
                    log.write(f"FREEZY2 {n} {us} 0 {3*n-2} {3*n} {binascii.crc32(pixels):08X} ".encode()
                              + packed + b"\n")
                log.write(b"FREEZY2 END built=3 sent=3 dropped=0 txErrors=0 inputDrops=0\n")
            frames = read_capture(path)
        self.assertEqual(frames[0].pixels, pixels)

        bands = b"0123" * 32 + b"\r\n"
        bands += b"".join((bytes([48 + shade]) * 128 + b"\r\n") * rows
                          for shade, rows in enumerate((7, 8, 8, 8)))
        output = io.BytesIO()
        write_dump(output, frames)
        expected = b"".join(stamp + bands + b"\r\n" for stamp in
                            (b"0x000003E8\r\n", b"0x00000401\r\n", b"0x00000419\r\n"))
        self.assertEqual(output.getvalue(), expected)
        self.assertEqual(verify_dump(io.BytesIO(expected)), (3, 49))

    def test_verifier_rejects_invalid_files(self):
        output = io.BytesIO()
        write_dump(output, [Frame(1, 50, 0, 1, 3, bytes(4096)),
                            Frame(2, 24650, 0, 4, 6, bytes(4096))])
        valid = output.getvalue()
        for bad in (valid[:-2], valid.replace(b"\r\n", b"\n"), b"\xef\xbb\xbf" + valid,
                    valid[:12] + b"4" + valid[13:], valid[:12] + valid[13:],
                    valid.replace(b"0x00000401", b"0x000003E8")):
            with self.subTest(prefix=bad[:20], length=len(bad)), self.assertRaises(ValueError):
                verify_dump(io.BytesIO(bad))


if __name__ == "__main__":
    unittest.main()
