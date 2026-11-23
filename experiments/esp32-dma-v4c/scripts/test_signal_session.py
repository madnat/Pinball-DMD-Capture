from pathlib import Path
import subprocess,tempfile,unittest

class SignalSessionTests(unittest.TestCase):
    def test_startup_outages_retries_and_more_than_4096(self):
        code=r'''
#include "signal_session.h"
#include "grey_decoder.h"
#include <assert.h>
int main(){
 SignalSession s;
 for(int i=0;i<100;++i){assert(!s.enterWait(true));s.markSilent();}
 assert(s.waiting && s.errors==0 && s.losses==0 && s.recoveries==0);
 s.returned(100);assert(!s.goodGroup());assert(!s.waiting);
 assert(s.enterWait(true));
 for(int i=0;i<100;++i){assert(!s.enterWait(true));s.markSilent();s.returned(200+i);}
 assert(s.errors==1 && s.losses==1 && s.recoveries==0);
 assert(s.goodGroup());assert(s.recoveries==1);
 assert(s.enterWait(false));assert(s.errors==2 && s.losses==1);
 assert(s.markSilent());assert(!s.markSilent());s.returned(1000);assert(s.goodGroup());
 assert(s.errors==2 && s.losses==2 && s.recoveries==2);
 GreyDecoder d;uint8_t p[512]={};
 for(unsigned i=1;i<=10000;++i)d.push(p,i,0);
 assert(d.groups==3333);const unsigned before=d.groups;
 d.gap();assert(d.partial==0);d.push(p,10002,1);d.push(p,10003,1);
 assert(d.groups==before);assert(d.push(p,10004,1));assert(d.groups==before+1);
 return 0;
}'''
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp);(p/'test.cpp').write_text(code)
            subprocess.run(['/usr/bin/clang++','-std=c++11','-I',str(Path(__file__).resolve().parents[1]/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True,capture_output=True)
            subprocess.run([str(p/'test')],check=True,capture_output=True)

if __name__=='__main__':unittest.main()
