"""Check the exact ESP32 Freezy serializer against independent wire fixtures."""
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class DmdWebSocketProtocolTests(unittest.TestCase):
    def test_dimensions_timestamp_and_oriented_planes(self):
        source = r'''
#include "dmd_ws_protocol.h"
#include <stdio.h>

int main() {
    uint8_t dimensions[dmd_ws::dimensionsBytes];
    uint8_t frame[dmd_ws::frameBytes], pixels[4096];
    dmd_ws::dimensions(dimensions);
    if (dmd_ws::acquisitionTime(0x100000020ull, 0xfffffff0u) != 0xfffffff0ull ||
        dmd_ws::acquisitionTime(0x200000020ull, 0x10u) != 0x200000010ull) return 1;
    if (fread(pixels, 1, sizeof(pixels), stdin) != sizeof(pixels)) return 2;
    dmd_ws::gray2(frame, pixels, 0x12345678);
    fwrite(dimensions, 1, sizeof(dimensions), stdout);
    fwrite(frame, 1, sizeof(frame), stdout);
}
'''
        src = Path(__file__).resolve().parents[1] / "src"
        pixels = bytearray(shade for shade in range(4) for _ in range(1024))
        pixels[:8] = bytes([1, 2, 0, 0, 0, 0, 0, 3])
        pixels[128] = 2
        pixels[-1] = 0
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "test.cpp").write_text(source)
            subprocess.run(["clang++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                            "-I", str(src), str(root / "test.cpp"), "-o", str(root / "test")],
                           check=True)
            payload = subprocess.run([str(root / "test")], input=pixels,
                                     capture_output=True, check=True).stdout

        self.assertEqual(payload[:19], b"dimensions\0" + struct.pack("<II", 128, 32))
        self.assertEqual(payload[19:35], b"gray2Planes\0" + struct.pack("<I", 0x12345678))
        self.assertEqual(len(payload), 19 + 1040)
        low, high = payload[35:547], payload[547:]
        self.assertEqual((low[0], high[0]), (0x81, 0x82))
        self.assertEqual((low[16], high[16]), (0, 1))
        self.assertEqual((low[-1], high[-1]), (0x7F, 0x7F))
        decoded = bytes(((low[i // 8] >> (i % 8)) & 1) |
                        (((high[i // 8] >> (i % 8)) & 1) << 1) for i in range(4096))
        self.assertEqual(decoded, pixels)


if __name__ == "__main__":
    unittest.main()
