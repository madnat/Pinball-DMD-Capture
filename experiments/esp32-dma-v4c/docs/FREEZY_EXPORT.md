# Fish Tales Freezy text export

The existing attract sequence reports CRCs for every validated 1-bit refresh
and stores pixels only for its last 32 refreshes. The replica reports one CRC
and histogram per completed four-shade image and dumps one tail image. Its
internal binary queue carries 512-byte refreshes with acquisition timestamps.
The existing serial logs do not contain a replayable four-shade clip.

The opt-in finite replica environment uses the same 128×32 row-major image
and intensity values as the TTGO renderer. The decoder packs each completed
image into 1024 bytes and tries a nonblocking eight-packet queue. A separate
task writes fixed-length binary pixels and an ASCII metadata header at 460800
baud. Each header includes the pixel CRC in place of a separate GRAY line. The DMA loop has no new
serial calls. The summary counts queue drops, TX errors and decoder input
drops. The host checks the pixel CRC and accounts for records missing or
damaged in transit.

Build and use the verified-device helpers from this experiment directory.
The upload and hardware capture require explicit approval.

```sh
sh scripts/build.sh ttgo_tdisplay_dmd_replica_freezy
python3 scripts/upload.py --environment ttgo_tdisplay_dmd_replica_freezy --port /dev/cu.usbserial-02073BE4 --confirm-upload
python3 scripts/capture.py --port /dev/cu.usbserial-02073BE4 --baud 460800 --duration 55 --reset
python3 scripts/export_freezy.py diagnostics/ttgo_serial_<capture>.txt --output diagnostics/fish-tales-attract.txt
```

Leave Fish Tales running in attract mode and close other serial monitors
before capture. The ROM boot banner is transmitted at a different baud
rate. The firmware startup trace and FREEZY2 markers identify this run.
Keep the unfiltered raw log and its metadata. The high-baud reader leaves
its 0.2-second serial timeout fixed to avoid reapplying macOS custom baud
rates during acquisition.

The timestamp comes from the measured DMA closing edge of the third
validated refresh in each grayscale group. The exporter subtracts the first
frame's acquisition time, rounds the relative time to the nearest
millisecond, then adds 1000 ms. Export preserves capture order and repeated
images. The default clips the first 18 seconds. Duration is measured from
first to last frame timestamp. A completed-frame sequence jump, skipped
binary refresh IDs, a new recovery epoch, or an interval over 40 ms
(each normal interval is about 24.6 ms) marks a gap. The exporter leaves
gaps in the timeline and reports their locations. The binary writer uses
explicit ASCII CRLF with a blank line after every image, then rereads
the entire file to verify the format. The output path must be new.

Hardware-free test:

```sh
python3 -m unittest discover -s scripts -p 'test_*.py'
```

## Windows setup

Use separate directories for the two Freezy versions and run each executable
by its full path or from its own directory. Keep downloaded executables, the
PAC, the capture and local INIs outside Git.

### Offline colourised playback with Freezy v2.5.0

Make a copy of `fish-tales-attract.txt` named `ft_l5.txt` for playback.
Freezy uses the dump's filename stem as the game name. Put `FT_2.0.pac` in
the `ft_l5` folder under Freezy's active `altcolor` directory. Use the
`ft_l5` name in both places so Freezy can find the package.

Copy the v2.5.0 distribution's INI to `DmdDevice.fishtales-test.ini`.
Enable `colorize = true` in `[global]` and `enabled = true` in
`[virtualdmd]`. Disable other outputs you do not intend to use for this
test, including `[networkstream]`. From the v2.5.0 directory in PowerShell:

```powershell
.\dmdext.exe play --file=".\ft_l5.txt" --use-ini=".\DmdDevice.fishtales-test.ini"
```

Check the Freezy log for the selected game and PAC, and use the virtual DMD
to verify colourised playback. The tagged
[Freezy dump source](https://github.com/freezy/dmd-extensions/blob/v2.5.0/LibDmd/Input/FileSystem/DumpSource.cs)
shows the filename lookup.

### WebSocket loopback with Freezy v2.5.1-SNAPSHOT-r21

Run the r21 server on Windows loopback with the virtual destination:

```powershell
.\dmdext.exe server -d virtual --ip=127.0.0.1 --port=8765 --path=/dmd
```

In a second r21 process, replay the dump with a separate test INI. Set
`[networkstream]` to `enabled = true` and
`url = ws://127.0.0.1:8765/dmd`. Set `colorize = false` under `[global]`
and disable the sender's local display outputs so the receiver display is
the one being checked.

```powershell
.\dmdext.exe play --file=".\ft_l5.txt" --use-ini=".\DmdDevice.fishtales-loopback.ini"
```

Verify that the receiver displays the replay. This Windows loopback uses the
dump as its frame source. See [TTGO live capture status](DMD_WEBSOCKET.md)
for the separate hardware result.
