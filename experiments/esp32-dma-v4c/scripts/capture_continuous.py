#!/usr/bin/env python3
"""Preserve raw bytes during a coordinated test; stop-file ends cleanly, no reset on stop."""
import argparse,json,time
from datetime import datetime
from pathlib import Path
from capture import duration_value
from upload import verify_device
ROOT=Path(__file__).resolve().parents[1]

def collect_live(port, output, duration, reset, stop, clock=time.monotonic, sleep=time.sleep):
    deadline=clock()+duration
    try:
        port.open()
        if reset:
            port.dtr=False;port.rts=True
            try:sleep(min(0.1,max(0,deadline-clock())))
            finally:port.rts=False
        line_boundary=True
        while clock()<deadline:
            if stop() and line_boundary:break
            port.timeout=min(0.2,max(0,deadline-clock()))
            data=port.read(min(65536,max(1,port.in_waiting)))
            if data:output.write(data);output.flush();line_boundary=data.endswith(b'\n')
    finally:port.close()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',required=True);parser.add_argument('--duration',type=duration_value,default=900)
    parser.add_argument('--reset',action='store_true');parser.add_argument('--stop-file',required=True,type=Path)
    args=parser.parse_args()
    if args.stop_file.exists():parser.error('stop file already exists; choose a new path')
    from serial import Serial
    from serial.tools import list_ports
    verify_device(args.port,list_ports.comports())
    directory=ROOT/'diagnostics';directory.mkdir(exist_ok=True)
    stamp=datetime.now().astimezone().strftime('%Y%m%d_%H%M%S_%f%z')
    path=directory/f'ttgo_continuous_{stamp}.txt'
    port=Serial(port=None,baudrate=115200,timeout=0.2,exclusive=True);port.port=args.port;port.dtr=False;port.rts=False
    metadata=dict(port=args.port,baud=115200,duration_seconds=args.duration,reset_requested=args.reset,
                  started=datetime.now().astimezone().isoformat(),stop_file=str(args.stop_file))
    print('Capture:',path,flush=True);started=time.monotonic();status=0
    try:
        with path.open('xb') as out:collect_live(port,out,args.duration,args.reset,args.stop_file.exists)
    except BaseException as error:
        metadata['error']=f'{type(error).__name__}: {error}';status=1
    finally:
        port.close();raw=path.read_bytes()
        metadata.update(elapsed_seconds=round(time.monotonic()-started,3),bytes=len(raw),port_closed=not port.is_open,
                        stopped_by_request=args.stop_file.exists(),startup_captured=b'01 setup entered; Serial ready' in raw,
                        continuous_startup_captured=b'CONTINUOUS REPLICA enabled:' in raw,
                        boot_banner_captured=b'boot:' in raw)
        path.with_suffix('.json').write_text(json.dumps(metadata,indent=2)+'\n');print(json.dumps(metadata,indent=2),flush=True)
    return status
if __name__=='__main__':raise SystemExit(main())
