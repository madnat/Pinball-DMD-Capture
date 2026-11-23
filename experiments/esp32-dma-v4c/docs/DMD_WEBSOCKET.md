# Live Freezy WebSocket replica

The opt-in PlatformIO environment `ttgo_tdisplay_dmd_replica_ws` extends
`ttgo_tdisplay_dmd_replica_continuous`. It uses espressif32 6.3.2, board
esp32dev, Arduino, TFT_eSPI 2.5.43 and 115200 baud for serial diagnostics.
The existing serial pixel exporter remains in its own environment.

Set local Wi-Fi configuration in the Git-ignored `src/dmd_ws_local.h`.

```cpp
#define DMD_WS_WIFI_SSID "Loki"
#define DMD_WS_WIFI_PASSWORD "replace with the Wi-Fi password"
#define DMD_WS_URL "ws://192.168.0.125:8765/dmd"
```

SSID and URL default to the values above. Set an explicitly empty password
for an open network. With no password definition the sender reports offline
and the TTGO replica can run without Wi-Fi. Local password values are embedded
in the corresponding firmware binary.

```sh
sh scripts/build.sh ttgo_tdisplay_dmd_replica_ws
python3 -m unittest discover -s scripts -p 'test_*.py'
```

Use Freezy v2.5.1-SNAPSHOT-r21 for the
[Windows loopback check](FREEZY_EXPORT.md#websocket-loopback-with-freezy-v251-snapshot-r21).
For an approved TTGO test, bind the receiver to its LAN address and allow
inbound TCP on port 8765. Use the matching address and path in the TTGO URL.

```sh
python3 scripts/upload.py --environment ttgo_tdisplay_dmd_replica_ws --port /dev/cu.usbserial-02073BE4 --confirm-upload
python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --baud 115200 --duration 55 --reset
```

The same completed row-major image feeds the TFT and the WebSocket encoder.
Each connection starts with a 19-byte binary `dimensions\0` message
containing width and height as little-endian 32-bit integers. Images are
1040-byte binary messages with `gray2Planes\0`, a little-endian 32-bit
timestamp in milliseconds, then 512 bytes for each shade bit, low bit
plane first. The leftmost pixel in each byte occupies bit zero. The timestamp
is the third validated refresh's acquisition time since ESP32 boot, extended
past the 32-bit microsecond wrap and truncated to milliseconds. See
[Freezy v2.5.0's serializer](https://github.com/freezy/dmd-extensions/blob/v2.5.0/LibDmd/Output/Network/WebsocketSerializer.cs)
and [plane splitter](https://github.com/freezy/dmd-extensions/blob/v2.5.0/LibDmd/Common/FrameUtil.cs).

The decoder offers frames without waiting for its two-slot queue. The lower
priority sender task handles Wi-Fi and WebSocket calls. It drops new images
when the queue is full and discards queued images older than 150 ms or from a
previous connection. `WS STATUS` reports successful connections, reconnects,
sends, and offline, queue, stale, and send drops. Connection attempts, client startup failures and dimensions send failures
have separate counters. Compare with
`CONT STATUS` for DMA/decoder input, recovery, and renderer evidence.

## Live TTGO status and follow-up

On 23 November 2026, the TTGO connected to Wi-Fi and completed the WebSocket
handshake before Fish Tales was powered on. In three repeated runs it
stopped at transaction 1 after alignment on the 100 µs SPI handover
deadline. Callback entry was already at 103.004–103.992 µs after the
deadline start. No image frames were sent. Three WebSocket-disabled runs
completed their approximately ten-second active windows, with maximum
reopen times of 76–78 µs. See the
[engineering journal](ENGINEERING_JOURNAL.md#13481356-handover-timestamp-comparison-gmt).

The remaining bench comparisons are Wi-Fi associated with the sender and
WebSocket disabled, and SPI queue timing before the setup callback to
distinguish descriptor readiness, interrupt delivery and driver work.
Keep the watchdog and existing handover limit enabled during those tests.
