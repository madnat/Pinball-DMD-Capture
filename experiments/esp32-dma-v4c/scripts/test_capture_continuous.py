import io,unittest
from capture_continuous import collect_live
from test_automation import FakePort

class LiveCaptureTests(unittest.TestCase):
    def test_clean_stop_preserves_bytes_and_releases_port(self):
        p=FakePort();out=io.BytesIO()
        collect_live(p,out,900,True,lambda:p.reads>=2,clock=lambda:p.now,sleep=p.sleep)
        self.assertEqual(out.getvalue(),p.payload);self.assertFalse(p.is_open)
        self.assertFalse(p.rts);self.assertFalse(p.dtr);self.assertLess(p.now,1)
    def test_disconnect_closes_without_discarding_bytes(self):
        p=FakePort('read');out=io.BytesIO()
        with self.assertRaises(OSError):collect_live(p,out,900,False,lambda:False,clock=lambda:p.now,sleep=p.sleep)
        self.assertFalse(p.is_open);self.assertEqual(out.getvalue(),p.payload)
if __name__=='__main__':unittest.main()
