#!/usr/bin/env python3
"""Audit a continuous raw capture: no reboot, monotonic CRCs and one outage event."""
import argparse,json,re
from pathlib import Path

def values(line):
    result={}
    for k,v in re.findall(r'(\w+)=([^\s]+)',line):
        if k in {'crc','LCDcrc'}:result[k]=int(v,16)
        elif re.fullmatch(r'\d+',v):result[k]=int(v)
        else:result[k]=v
    return result

def analyse(path):
    raw=path.read_bytes();text=raw.decode('utf-8',errors='replace')
    if text.count('01 setup entered; Serial ready')!=1 or text.count('CONTINUOUS REPLICA enabled:')!=1:
        raise ValueError('missing or repeated ESP32 startup')
    after=text[text.index('01 setup entered; Serial ready'):]
    if re.search(r'(?m)^(?:ets Jul|rst:|.*boot:|Guru Meditation|Backtrace:|CONT FATAL)',after):
        raise ValueError('reset, crash or fatal capture stop in test')
    statuses=[values(x) for x in re.findall(r'^CONT STATUS .+$',text,re.M)]
    images=[values(x) for x in re.findall(r'^GRAY n=.+$',text,re.M)]
    events=[values(x) for x in re.findall(r'^CONT EVENT .+$',text,re.M)]
    if not statuses or not images:raise ValueError('no status or resumed grayscale stream')
    if [x['n'] for x in images]!=list(range(1,len(images)+1)):
        raise ValueError('lost, duplicated or reset grayscale sequence number')
    for a,b in zip(images,images[1:]):
        if not 0<((b['us']-a['us'])&0xffffffff)<1000000000:raise ValueError('nonmonotonic image clock')
        if b['epoch']<a['epoch'] or b['last']-b['first']!=2:raise ValueError('invalid group boundary')
        if b['epoch']==a['epoch'] and b['first']!=a['last']+1:raise ValueError('unlogged group discontinuity')
    for line in re.findall(r'^GRAY n=.+$',text,re.M):
        hist=re.search(r'hist=([\d,]+)',line)
        if not hist or sum(map(int,hist[1].split(',')))!=4096:raise ValueError('invalid grayscale histogram')
    if any(s['crcLost'] or s['binaryDrops'] or s['eventLost'] for s in statuses):raise ValueError('CRC/input/event queue loss reported')
    if max(s['maxReopen'] for s in statuses)>=100 or max(s['maxAlignment'] for s in statuses)>=100:
        raise ValueError('handover/alignment deadline exceeded')
    if any(b['frames']<a['frames'] for a,b in zip(statuses,statuses[1:])):raise ValueError('binary frame count restarted')
    lookup={x['n']:x for x in images};recoveries=[]
    for wait in [e for e in events if e['kind']=='WAITING_OUTAGE']:
        episode=wait['episode']
        resumed=next((e for e in events if e['kind']=='RESUMED_GRAY' and e['episode']==episode),None)
        if not resumed:raise ValueError('outage has no valid grayscale recovery')
        returns=[e for e in events if e['kind']=='SIGNAL_RETURN' and e['episode']==episode and wait['us']<e['us']<resumed['us']]
        if not returns:raise ValueError('recovery without detected clocks')
        returned=returns[-1]
        image=lookup[resumed['a']]
        if image['epoch']!=resumed['epoch'] or resumed['a']!=wait['b']+1:
            raise ValueError('CRC stream did not resume at the next complete group')
        pauses=[s for s in statuses if s['state']=='WAITING' and wait['us']<s['us']<returned['us']]
        if not pauses:raise ValueError('outage waiting interval not captured')
        if any(s['Errors']!=episode or s['Losses']!=episode or s['Recoveries']!=episode-1 for s in pauses):
            raise ValueError('outage was counted repeatedly or recovery count wrong')
        if any(s['LCDn']!=wait['b'] or s['LCDcrc']!=wait['c'] or s['partial']!=0 for s in pauses):
            raise ValueError('image not held or incomplete group not cleared')
        lcd=next(e for e in events if e['kind']=='LCD_RESUMED' and e['episode']==episode)
        drops=[e for e in events if e['kind']=='DISCARDED_DMA' and e['episode']==episode]
        recoveries.append(dict(episode=episode,lossAtFrames=wait['a'],lastGrayBefore=wait['b'],firstGrayAfter=resumed['a'],
                               detectedReturnEventUs=returned['us'],returnToGrayUs=resumed['c'],returnToLcdUs=lcd['d'],
                               discardedBits=[d['a'] for d in drops],waitingStatusSamples=len(pauses),
                               returnAttempts=len(returns),imageHeld=True,partialCleared=True))
    initial=next((e for e in events if e['kind']=='STARTED_GRAY'),None)
    startup=None
    if initial:
        returned=next(e for e in events if e['kind']=='SIGNAL_RETURN' and e['episode']==0)
        waiting=[s for s in statuses if s['state']=='WAITING' and s['us']<returned['us']]
        if any(s['frames'] or s['gray'] or s['Errors'] or s['Losses'] or s['Recoveries'] for s in waiting):
            raise ValueError('startup silence incorrectly counted as a fault')
        lcd=next(e for e in events if e['kind']=='LCD_RESUMED' and e['episode']==0)
        startup=dict(waitingSamples=len(waiting),firstReturnEventUs=returned['us'],returnToGrayUs=initial['c'],returnToLcdUs=lcd['d'])
    return dict(startupCount=1,noRebootDuringTest=True,grayscaleRecords=len(images),crcNumbersContinuous=True,
                crcLost=0,binaryDrops=0,eventLost=0,maxFrames=max(s['frames'] for s in statuses),
                worstHandoverUs=max(s['maxReopen'] for s in statuses),worstAlignmentUs=max(s['maxAlignment'] for s in statuses),
                finalStatus=statuses[-1],outages=recoveries,startup=startup,completeTrailingLine=raw.endswith(b'\n'))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path)
    print(json.dumps(analyse(p.parse_args().log),indent=2))
