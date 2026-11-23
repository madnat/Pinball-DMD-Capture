#!/usr/bin/env python3
"""Audit recovered output, discarded IDs, timing arithmetic and continued capture."""
import argparse
import json
from pathlib import Path
import re
from analyse_filter_log import fields
from compare_sequences import load_capture

MASK=0xffffffff

def audit_publications(ids, events, timings):
    if len(ids)!=4096 or any(b<=a for a,b in zip(ids,ids[1:])):
        raise ValueError('publication IDs are incomplete, duplicated or reordered')
    discarded={e['candidate'] for e in events}
    if any(i in discarded for i in ids):
        raise ValueError('suspect refresh was published')
    if set(range(1,ids[-1]+1))-set(ids)!=discarded:
        raise ValueError('unexplained output gap')
    for e,t in zip(events,timings):
        if not e['reclaimed'] or not e['aligned']:
            raise ValueError('ownership/alignment was not recovered')
        seq=t['firstGoodSequence']
        if not 1<seq<len(ids) or ids[seq-1]!=t['firstGoodCandidate'] or ids[seq-1]<=e['candidate']:
            raise ValueError('first post-recovery publication invalid or capture did not continue')
        for measured,later,earlier in [('detectionToGoodUs','firstGoodEndUs','detectedUs'),
                                      ('goodGapUs','firstGoodEndUs','lastGoodEndUs'),
                                      ('detectionToPublishUs','firstGoodPublishedUs','detectedUs'),
                                      ('publicationGapUs','firstGoodPublishedUs','lastGoodPublishedUs')]:
            if ((t[later]-t[earlier])&MASK)!=t[measured]:
                raise ValueError('recovery timing arithmetic mismatch')
        if not 0<t['detectionToGoodUs']<=t['detectionToPublishUs']<1000000:
            raise ValueError('invalid recovery time ordering')
        if not 0<((t['reclaimedUs']-t['detectedUs'])&MASK)<100000:
            raise ValueError('descriptor reclaim timing invalid')
        if not 0<((t['firstGoodEndUs']-t['syncUs'])&MASK)<100000:
            raise ValueError('no complete post-sync refresh')
    return True


def analyse(path):
    capture=load_capture(path,allow_recovery=True)
    text=path.read_text(errors='replace')
    if capture.outcome!='RECOVERED_PASS':raise ValueError('recovery did not finish successfully')
    if text.count('01 setup entered; Serial ready')!=1 or text.count('AUTO RECOVERY enabled:')!=1:
        raise ValueError('missing/repeated setup; reset-free recovery not verified')
    if 'boot:' in text[text.index('01 setup entered; Serial ready'):]:raise ValueError('boot during recovery')
    if text.count('RECOVERY END;')!=1 or text.count('RD FILTER END')!=1:
        raise ValueError('truncated recovery/filter evidence')
    config=fields(re.search(r'^RD FILTER COMPARE enabled: .+$',text,re.M)[0])
    if any(config[k]!=v for k,v in dict(readback=80,enabled=1,rawFilterEnabled=0,APB_Hz=80000000).items()):
        raise ValueError('filter configuration invalid')
    blocks=re.findall(r'^PUB BEGIN frames=(\d+)\n(.*?)^PUB END frames=(\d+)$',text,re.M|re.S)
    if len(blocks)!=1 or blocks[0][0]!='4096' or blocks[0][2]!='4096':raise ValueError('publication dump incomplete')
    ids=[]
    for line in blocks[0][1].splitlines():
        f=line.split()
        if f[:2]!=['PUB','IDS'] or int(f[2])!=len(ids)+1:raise ValueError('publication order invalid')
        ids.extend(map(int,f[3:]))
    events=[fields(line) for line in re.findall(r'^RECOVERY EVENT .+$',text,re.M)]
    times=[fields(line) for line in re.findall(r'^RECOVERY TIMING .+$',text,re.M)]
    if len(events)!=len(times) or sum(e['injected'] for e in events)!=1:raise ValueError('expected one deliberate fault')
    for i,(e,t) in enumerate(zip(events,times)):
        if e['id']!=i or t['id']!=i:raise ValueError('event IDs invalid')
        if e['injected'] and (not 0<e['droppedBits']<4096 or e['frame']<257):raise ValueError('fault not stable and mid-refresh')
    audit_publications(ids,events,times)
    summary=fields(re.search(r'^RD FILTER SUMMARY .+$',text,re.M)[0])
    if summary['filteredMarkerOK']!=4096:raise ValueError('filtered marker validation failed')
    result=dict(outcome=capture.outcome,frames=len(capture.crcs),events=events,timings=times,
                maxReopen=int(re.search(r'Handover: .*?maxReopen=(\d+) us',text)[1]),
                maxAlignmentOpen=fields(re.search(r'^RECOVERY TOTALS .+$',text,re.M)[0])['maxAlignmentOpen'],
                summary=summary,suspect_published=False,continued_without_reset=True,
                publication_ids_count=len(ids),last_candidate=ids[-1])
    if max(result['maxReopen'],result['maxAlignmentOpen'])>=100:raise ValueError('100 us deadline exceeded')
    return result

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path)
    print(json.dumps(analyse(p.parse_args().log),indent=2))
