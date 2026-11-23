#!/usr/bin/env python3
"""Read-only replica audit: CRC coverage, group boundaries, raw tail and LCD holds."""
import argparse,json,re,zlib
from pathlib import Path
from collections import Counter
from compare_sequences import load_capture
from analyse_filter_log import fields
from analyse_recovery_log import audit_publications

def pairs(line):
    result={}
    for k,v in re.findall(r'(\w+)=([^\s]+)',line):
        if k in {'crc','heldCRC','imageCRC','latestCRC'}:result[k]=int(v,16)
        elif re.fullmatch(r'\d+',v):result[k]=int(v)
    return result

def analyse(path):
    c=load_capture(path,allow_replica=True)
    if c.outcome!='REPLICA_PASS':raise ValueError('replica did not complete validated capture')
    t=path.read_text(errors='replace')
    if t.count('REPLICA END')!=1 or t.count('01 setup entered; Serial ready')!=1:raise ValueError('incomplete/repeated replica startup/end')
    if 'TFT SPI_PORT=3 VSPI/SPI3' not in t or 'captureHost=1 SPI2' not in t:raise ValueError('SPI hosts not verified')
    summary=pairs(re.search(r'^REPLICA SUMMARY .+$',t,re.M)[0])
    records=[]
    for line in re.findall(r'^GRAY n=.+$',t,re.M):
        r=pairs(line);r['histogram']=list(map(int,re.search(r'hist=([\d,]+)',line)[1].split(',')))
        if sum(r['histogram'])!=4096:raise ValueError('bad grayscale histogram')
        records.append(r)
    if len(records)!=summary['crcEmitted'] or summary['crcEmitted']+summary['crcLost']!=summary['grayBuilt']:
        raise ValueError('CRC record accounting mismatch')
    coverage=summary['crcLost']==0 and summary['binaryDrops']==0
    if coverage and [r['n'] for r in records]!=list(range(1,summary['grayBuilt']+1)):raise ValueError('missing numbered grayscale record')
    ids=[]
    for line in re.findall(r'^PUB IDS .+$',t,re.M):
        f=line.split();assert int(f[2])==len(ids)+1;ids.extend(map(int,f[3:]))
    events=[fields(x) for x in re.findall(r'^RECOVERY EVENT .+$',t,re.M)]
    times=[fields(x) for x in re.findall(r'^RECOVERY TIMING .+$',t,re.M)]
    audit_publications(ids,events,times)
    expected=[];partial=[];epoch=0;discardedPartial=0
    for i,candidate in enumerate(ids):
        if i and candidate!=ids[i-1]+1:
            if partial:discardedPartial+=1
            partial=[];epoch+=1
        partial.append(candidate)
        if len(partial)==3:expected.append((partial[0],partial[-1],epoch));partial=[]
    if coverage and [(x['first'],x['last'],x['epoch']) for x in records]!=expected:raise ValueError('gray group crossed a gap or phase mismatch')
    if coverage and (summary['partialDiscarded']!=discardedPartial or summary['partialRemaining']!=len(partial)):
        raise ValueError('partial-group accounting mismatch')
    raw={ids[int(n)-1]:bytes.fromhex(p) for n,p in re.findall(r'^SEQ PIXELS (\d+) ([0-9A-F]+)$',t,re.M)}
    verified=0
    for image in records:
        group=list(range(image['first'],image['last']+1))
        if not all(x in raw for x in group):continue
        values=bytes(sum(bool(raw[x][i//8]&(0x80>>(i%8))) for x in group) for i in range(4096))
        h=Counter(values)
        if zlib.crc32(values)!=image['crc'] or [h[x] for x in range(4)]!=image['histogram']:
            raise ValueError('grayscale CRC/intensity histogram does not match raw refreshes')
        verified+=1
    tail=re.search(r'^GRAY TAIL n=(\d+) .*?crc=([0-9A-F]{8}) values=([0-3]+)$',t,re.M)
    if not tail or len(tail[3])!=4096:raise ValueError('missing grayscale tail evidence')
    values=bytes(int(x) for x in tail[3]);assert zlib.crc32(values)==int(tail[2],16)==records[-1]['crc']
    status=pairs(re.search(r'^LCD STATUS .+$',t,re.M)[0])
    for k in ['Errors','Recoveries','FilteredRD']:
        if status[k]!=summary[k]:raise ValueError('LCD status did not reach final counters')
    if summary['Errors']!=len(events) or summary['Recoveries']!=sum(e['aligned'] for e in events):raise ValueError('fault counters not counted exactly once')
    if summary['injected']!=sum(e['injected'] for e in events):raise ValueError('injected counter mismatch')
    holds=[pairs(x) for x in re.findall(r'^GRAY HOLD .+$',t,re.M)]
    if len(holds)!=len(events):raise ValueError('missing LCD hold record')
    for h in holds:
        before=records[h['heldN']-1];after=records[h['firstNewN']-1]
        if before['crc']!=h['heldCRC'] or after['epoch']<=before['epoch']:raise ValueError('held image or resumed image invalid')
        for measured,start,end in [('holdToStartUs','lastCompleteUs','drawStartUs'),('holdToCompleteUs','lastCompleteUs','drawEndUs'),('detectionToDrawUs','detectedUs','drawStartUs')]:
            if (h[end]-h[start])&0xffffffff!=h[measured]:raise ValueError('hold duration arithmetic invalid')
    if summary['rendered']+summary['rendererDrops']!=summary['grayBuilt']:raise ValueError('renderer accounting mismatch')
    maximum=int(re.search(r'Handover: .*?maxReopen=(\d+) us',t)[1])
    if maximum>=100:raise ValueError('handover deadline exceeded')
    return dict(outcome=c.outcome,frames=len(c.crcs),maxReopen=maximum,summary=summary,
                crcCoverageComplete=coverage,rawTailGrayImagesVerified=verified,lcdStatus=status,
                holds=holds,recoveryEvents=events,recoveryTimings=times,
                periodicity=[pairs(x) for x in re.findall(r'^GRAY PERIOD .+$',t,re.M)],
                grouping='three consecutive equal refresh sum; physical phase/brightness needs visual confirmation')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path)
    print(json.dumps(analyse(p.parse_args().log),indent=2))
