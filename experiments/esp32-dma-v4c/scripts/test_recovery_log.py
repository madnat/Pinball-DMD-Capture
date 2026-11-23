import unittest
from analyse_recovery_log import audit_publications

class RecoveryAuditTests(unittest.TestCase):
    def evidence(self):
        ids=list(range(1,257))+list(range(258,4098))
        e=dict(candidate=257,reclaimed=1,aligned=1)
        t=dict(firstGoodSequence=257,firstGoodCandidate=258,
               detectedUs=10000,reclaimedUs=10030,syncUs=15000,
               firstGoodEndUs=23000,lastGoodEndUs=7000,
               firstGoodPublishedUs=24000,lastGoodPublishedUs=8000,
               detectionToGoodUs=13000,goodGapUs=16000,
               detectionToPublishUs=14000,publicationGapUs=16000)
        return ids,[e],[t]
    def test_valid_skip_and_timings_over_ten_ms(self):
        self.assertTrue(audit_publications(*self.evidence()))
    def test_reject_suspect_publication(self):
        ids,e,t=self.evidence();ids[256]=257
        with self.assertRaisesRegex(ValueError,'suspect'):
            audit_publications(ids,e,t)
    def test_reject_unexplained_gaps_and_false_timing(self):
        ids,e,t=self.evidence();e[0]['candidate']=258
        with self.assertRaises(ValueError):audit_publications(ids,e,t)
        ids,e,t=self.evidence();t[0]['goodGapUs']=9000
        with self.assertRaisesRegex(ValueError,'timing arithmetic'):audit_publications(ids,e,t)
    def test_reject_unreclaimed_buffer(self):
        ids,e,t=self.evidence();e[0]['reclaimed']=0
        with self.assertRaisesRegex(ValueError,'ownership'):audit_publications(ids,e,t)

if __name__=='__main__':unittest.main()
