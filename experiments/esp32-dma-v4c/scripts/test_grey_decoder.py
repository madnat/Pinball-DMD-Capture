"""Compile/test the exact portable decoder used on the ESP32, not a Python clone."""
from pathlib import Path
import subprocess
import tempfile
import unittest
import zlib

class GreyDecoderTests(unittest.TestCase):
    def test_shades_crc_and_gap(self):
        source=r'''
#include "grey_decoder.h"
#include <assert.h>
#include <stdio.h>
int main() {
 GreyDecoder d;uint8_t a[512]={},b[512]={},c[512]={};
 a[0]=0x70;b[0]=0x30;c[0]=0x10;
 assert(!d.push(a,1,0));assert(!d.push(b,2,0));assert(d.push(c,3,0));
 for(int i=0;i<4;++i)assert(d.image[i]==i);
 for(int i=0;i<4;++i)for(int j=i+1;j<4;++j)assert(GREY_PALETTE[i]!=GREY_PALETTE[j]);
 auto original=greyCRC32(d.image);printf("%08X\n",original);
 b[0]|=0x40;assert(!d.push(a,4,0));assert(!d.push(b,5,0));assert(d.push(c,6,0));
 assert(d.image[1]==2);assert(greyCRC32(d.image)!=original);
 const auto held=greyCRC32(d.image);
 assert(!d.push(a,7,0));assert(!d.push(b,8,0));
 assert(!d.push(c,10,1));assert(d.partialDiscards==1);assert(d.partial==1);
 assert(greyCRC32(d.image)==held); // no incomplete image published at gap
 assert(!d.push(c,11,1));assert(d.push(c,12,1));
 assert(d.firstCandidate==10);assert(d.image[3]==3);assert(d.image[1]==0);
 assert(d.groups==3);return 0;
}'''
        with tempfile.TemporaryDirectory() as temp:
            p=Path(temp);(p/'test.cpp').write_text(source)
            subprocess.run(['/usr/bin/clang++','-std=c++11','-I',str(Path(__file__).resolve().parents[1]/'src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True,capture_output=True)
            result=subprocess.run([str(p/'test')],check=True,capture_output=True,text=True)
            expected=bytes([0,1,2,3])+bytes(4092)
            self.assertEqual(result.stdout.strip(),f'{zlib.crc32(expected):08X}')

if __name__=='__main__':unittest.main()
