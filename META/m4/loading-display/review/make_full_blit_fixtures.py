"""Full-resolution golden CRCs from sequential PC53 raw endpoint contracts.

Moderate IEEE binary64 operations round like PC53 x87 arithmetic. Independent
Python state emits static output/store controls; no original code executes.
"""
import json,struct,zlib
from fractions import Fraction as F
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4]
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def intervals(kind,width,height,from_,to):
 w,h=width//2,height//2;offset=[0,.125,.375,.625,.875][kind];scale=8 if kind in (0,4) else 4
 a=f32((from_-offset)*scale);b=f32((to-offset)*scale);out=[]
 if kind in (0,2,4):
  for y in range(h):
   p=float((h-y)*w)/h;q=float(y*(w if kind==4 else w*2))/h
   if kind==0:lo,hi=int(p*a+w),int(p*b+w);yy=y
   elif kind==2:lo,hi=int(q*(1.-b)+p),int((q*(1.-a)+p)+1.);yy=y+h
   else:lo,hi=int(p*a+q),int(p*b+q);yy=y
   out.append((yy,max(0,lo),min(width,hi)))
 else:
  for x in range(w):
   p=float((w-x)*h)/w;q=float(x*h*2)/w
   if kind==1:lo,hi=int(q*a+p),int((q*b+p)+1.);xx=x+w
   else:lo,hi=int(q*(1.-b)+p),int((q*(1.-a)+p)+1.);xx=w-x-1
   out.append((xx,max(0,lo),min(height,hi)))
 return out
def exact_intervals(kind,width,height,from_,to):
 w,h=width//2,height//2;offset=[0,.125,.375,.625,.875][kind];scale=8 if kind in (0,4) else 4
 a=F.from_float(f32((from_-offset)*scale));b=F.from_float(f32((to-offset)*scale));out=[]
 if kind in (0,2,4):
  for y in range(h):
   if kind==0:lo,hi=int(w+F((h-y)*w,h)*a),int(w+F((h-y)*w,h)*b);yy=y
   elif kind==2:lo,hi=int(w+F(y*w,h)*(1-2*b)),int(w+F(y*w,h)*(1-2*a)+1);yy=y+h
   else:lo,hi=int(w-F((h-y)*w,h)*(1-a)),int(w-F((h-y)*w,h)*(1-b));yy=y
   out.append((yy,max(0,lo),min(width,hi)))
 else:
  for x in range(w):
   if kind==1:lo,hi=int(h+F(x*h,w)*(2*a-1)),int(h+F(x*h,w)*(2*b-1)+1);xx=x+w
   else:lo,hi=int(h+F(x*h,w)*(1-2*b)),int(h+F(x*h,w)*(1-2*a)+1);xx=w-x-1
   out.append((xx,max(0,lo),min(height,hi)))
 return out
out=['/* Sequential PC53 raw-contract golden CRC/store controls, not original execution. */'];report=[]
for width,height in [(320,240),(640,480)]:
 for kind,start,end in [(0,0,.125),(1,.125,.375),(2,.375,.625),(3,.625,.875),(4,.875,1.)]:
  for aa,bb in [(start,end),(start+(end-start)*.137,end-(end-start)*.113),(start+(end-start)*.3333333,start+(end-start)*.6666667),(end,start)]:
   a,b=f32(aa),f32(bb);ranges=intervals(kind,width,height,a,b);writes=sum(max(0,hi-lo) for _,lo,hi in ranges);crcs=[]
   for n in [1,2,4]:
    pitch=width*n+3;target=bytearray(b'\xcd'*(64+pitch*height));step=(pitch//n)*n
    for fixed,lo,hi in ranges:
     for moving in range(lo,hi):
      x,y=(fixed,moving) if kind in (1,3) else (moving,fixed)
      index=(x+11*y+3)&255;value=((index*5+7)&255) if n==1 else (0xaa000000+index*0x010203)&0xffffffff
      at=32+lo*pitch+x*n+(y-lo)*step if kind in (1,3) else 32+y*pitch+x*n
      target[at:at+n]=value.to_bytes(4,'little')[:n]
    crcs.append(zlib.crc32(target)&0xffffffff)
   out.append('{%d,%d,%d,0x%08xUL,0x%08xUL,%dUL,{%s}},'%(kind,width,height,bits(a),bits(b),writes,','.join('0x%08xUL'%c for c in crcs)))
   exact=exact_intervals(kind,width,height,a,b)
   differences=[dict(position=p[0],sequential=p[1:],rational=q[1:]) for p,q in zip(ranges,exact) if p!=q]
   report.append(dict(kind=kind,width=width,height=height,from_bits=f'{bits(a):08x}',to_bits=f'{bits(b):08x}',writes=writes,crcs=crcs,rational_endpoint_differences=differences))
(ROOT/'tests/loading_blit_full_cases.h').write_text('\n'.join(out)+'\n',newline='\n')
(Path(__file__).resolve().parent/'full-fixtures.json').write_text(json.dumps(dict(oracle='sequential binary64 PC53 raw endpoint contract, static goldens not original execution',rational_endpoint_differences=sum(len(c['rational_endpoint_differences']) for c in report),cases=report),indent=2)+'\n')
print('Created40 full-resolution sequential PC53 CRC/store fixtures')
