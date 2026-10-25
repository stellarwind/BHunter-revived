from pathlib import Path
import struct,json
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
truncdiv=lambda x,y:abs(x)//y*(-1 if x<0 else 1)
def sar(v,n):return (v if v<0x80000000 else v-0x100000000)>>n
def tint(color,shift,a):return int(((sar((color+((1<<shift)-1 if color&0x80000000 else 0))&0xffffffff,shift))&255)*a)
rows=[]
for depth,amount,color,indices in [(1,.37,0x314159,[0,1,41,42,127,210,255]),(1,.625,0x8091a2b3,[0,41,42,127,255]),(2,.37,0x314159,[0,1,0x7e0,0xf800,0xffff,0x1234]),(2,.625,0x8091a2b3,[0,0xffff,0x1234])]:
 a=f32(amount);keep=1-a;r=tint(color,16,a);g=tint(color,8,a);b=int((color&255)*a)
 for i in indices:
  if depth==1:
   rr=int(r-((i//42*255)*keep)*-.2);gg=int(g-(((i%42)//6*255)*keep)*(-1/6));bb=int(b-(((i%42)%6*255)*keep)*-.2)
   output=((truncdiv(rr*5,255)*7+truncdiv(gg*6,255))*6+truncdiv(bb*5,255))&255
  else:
   output=0
   for mask,extract,expand,add in [(0xf800,11,3,r),(0x7e0,5,2,g),(31,0,3,b)]:output+=(int((((i&mask)>>extract)<<expand)*keep+add)>>expand)<<extract
   output&=65535
  rows.append(dict(depth=depth,amount_bits=struct.unpack('<I',struct.pack('<f',a))[0],color=color,index=i,expected=output))
p=Path('META/m4/hud-attachment/fade');p.joinpath('static-cases.json').write_text(json.dumps(dict(method='Independent scalar oracle: signed RGB extraction, truncation, raw constant operation grouping, palette quantization and RGB565 repacking. Not original execution or general IEEE/x87 proof.',cases=rows),indent=2)+'\n')
h='typedef struct FadeCase {unsigned long depth,amount,color,index,expected;} FadeCase;\nstatic const FadeCase fade_cases[]={\n'+''.join('{%d,0x%08x,0x%08x,0x%04x,0x%04x},\n'%(r['depth'],r['amount_bits'],r['color'],r['index'],r['expected']) for r in rows)+'};\n'
Path('tests/fade_cases.h').write_text(h)
