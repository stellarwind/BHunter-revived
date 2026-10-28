"""Create isolated diagnostic box assets and copy the source game build."""
import argparse, hashlib, json, shutil, struct, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'build/box-test'
def pack(fmt,*v): return struct.pack('<'+fmt,*v)
def mesh(path, lo, hi, portal=None, inside=True):
    materials=[bytes(9)+b'win.pcx'.ljust(23,b'\0'),bytes(9)+b'PORTAL.GIF'.ljust(23,b'\0')]
    vertices=[];edges=[];faces=[];nodes=[];leaves=[]
    for axis in range(3):
        for side in range(2):
            is_portal = portal==(axis,side) or isinstance(portal,set) and (axis,side) in portal
            index=len(faces);normal=[0.,0.,0.];normal[axis]=(-1 if side else 1)*(1 if inside else -1)
            other=[j for j in range(3) if j!=axis];corners=[]
            for a,b in [(0,0),(1,0),(1,1),(0,1)]:
                point=[0.,0.,0.];point[axis]=(lo,hi)[side][axis];point[other[0]]=(lo,hi)[a][other[0]];point[other[1]]=(lo,hi)[b][other[1]];corners.append(point)
            # Orient the polygon consistently with its plane normal.
            u=[corners[1][j]-corners[0][j] for j in range(3)];v=[corners[2][j]-corners[1][j] for j in range(3)]
            cross=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
            if sum(cross[j]*normal[j] for j in range(3))<0:corners.reverse()
            base=len(vertices)
            for k,point in enumerate(corners):
                vertices.append(pack('3f',*point)+bytes(48));edges.append(pack('iiff',base+k,base+k+1 if k<3 else -1,float(k in (1,2))*4,float(k>=2)*4))
            faces.append(pack('iii',int(is_portal),base,0));leaves.append(pack('ii',index,-1))
            # Continue through the half-space containing the box interior.
            # Outward obstacle normals reverse this side. Portal normals
            # point inward, as required by original directional crossing.
            child=index+1 if index<5 else -1
            front=inside
            nodes.append(pack('10f4i',*lo,*hi,*normal,-sum(normal[j]*corners[0][j] for j in range(3)),index,index,child if front else -1,-1 if front else child))
    path.write_bytes(b''.join(pack('I',len(rows))+b''.join(rows) for rows in [materials,vertices,edges,faces,nodes,leaves]))
def main():
    global OUT
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ai-junctions',action='store_true')
    parser.add_argument('--tunnel-ring',action='store_true')
    args=parser.parse_args()
    if args.tunnel_ring:
        OUT=ROOT/'build/tunnel-ring'
    OUT.mkdir(parents=True,exist_ok=True);(OUT/'bsp').mkdir(exist_ok=True)
    mesh(OUT/'bsp/boxa.bsp',(-40,-20,-80),(40,20,0),(2,1))
    mesh(OUT/'bsp/boxb.bsp',(-40,-20,0),(40,20,80),(2,0))
    mesh(OUT/'bsp/boxpost.bsp',(-4,-20,-4),(4,8,4),inside=False)
    mesh(OUT/'bsp/boxmark.bsp',(-.2,-.2,-.2),(.2,.2,.2),inside=False)
    (OUT/'bsp/blocks.txt').write_text('boxa.bsp\nboxb.bsp\nboxpost.bsp\nboxmark.bsp\nend\n')
    (OUT/'bsp/boxmark.txt').write_text('grid 1\ncollision off\nrender off\n')
    for n in ['boxa','boxb','boxpost']:(OUT/('bsp/'+n+'.txt')).write_text('grid 1\ncollision on\nrender on\n')
    placements=[('boxa.bsp',0,0,0,0),('boxpost.bsp',20,0,-30,0),('boxb.bsp',0,0,0,0),('boxmark.bsp',0,0,5,0)]
    if args.ai_junctions or args.tunnel_ring:
        rooms=[('aicenter',(-40,-20,-40),(40,20,40),{(0,0),(0,1),(2,0),(2,1)}),
               ('aiwest',(-120,-20,-40),(-40,20,40),{(0,1),(2,0),(2,1)}),
               ('aieast',(40,-20,-40),(120,20,40),{(0,0),(2,0),(2,1)}),
               ('ainorth',(-40,-20,40),(40,20,120),{(2,0),(0,0),(0,1)}),
               ('aisouth',(-40,-20,-120),(40,20,-40),{(2,1),(0,0),(0,1)}),
               ('ainw',(-120,-20,40),(-40,20,120),{(0,1),(2,0)}),
               ('aine',(40,-20,40),(120,20,120),{(0,0),(2,0)}),
               ('aisw',(-120,-20,-120),(-40,20,-40),{(0,1),(2,1)}),
               ('aise',(40,-20,-120),(120,20,-40),{(0,0),(2,1)})]
        if args.tunnel_ring:
            # Eight degree-two tunnel sections around a solid central void.
            rooms=rooms[1:]
            rooms=[(name,lo,hi,portals-{(0,1)} if name=='aiwest' else
                   portals-{(0,0)} if name=='aieast' else
                   portals-{(2,0)} if name=='ainorth' else
                   portals-{(2,1)} if name=='aisouth' else portals)
                   for name,lo,hi,portals in rooms]
        for name,lo,hi,portals in rooms:
            mesh(OUT/('bsp/'+name+'.bsp'),lo,hi,portals)
            (OUT/('bsp/'+name+'.txt')).write_text('grid 1\ncollision on\nrender on\n')
        names=[r[0]+'.bsp' for r in rooms]+['boxpost.bsp','boxmark.bsp']
        (OUT/'bsp/blocks.txt').write_text('\n'.join(names)+'\nend\n')
        # Original ownership uses the preceding room in placement order.
        placements=[(names[0],0,0,0,0),('boxpost.bsp',20,0,-20,0)]+[(n,0,0,0,0) for n in names[1:len(rooms)]]+[('boxmark.bsp',0,0,5,0)]
        if args.tunnel_ring:
            placements=[(n+'.bsp',0,0,0,0) for n,_,_,_ in rooms]+[('boxmark.bsp',0,0,-75,0)]
    (OUT/'test.lev').write_bytes(pack('i',len(placements))+b''.join(n.encode().ljust(16,b'\0')+pack('4i',x,y,z,f) for n,x,y,z,f in placements))
    # A unique invalid cache is unnecessary: the original archive cache is rejected
    # on dimensions and the actual navigation builder computes this graph.
    build=json.loads((ROOT/'build/game/build.json').read_text());assert build['passed']
    shutil.copy2(ROOT/build['target'],OUT/'BHunter-box.exe')
    (OUT/'Play box.cmd').write_text('@echo off\ncd /d "%~dp0"\nBHunter-box.exe --box-test --hardware\n')
    (OUT/'Run checks.cmd').write_text('@echo off\ncd /d "%~dp0..\\.."\npython tools\\test_box_level.py\npause\n')
    (OUT/'level.json').write_text(json.dumps(dict(kind='isolated source diagnostic',bounds=[[-120,-20,-120],[120,20,120]] if args.ai_junctions else [[-40,-20,-80],[40,20,80]],rooms=9 if args.ai_junctions else 2,junctions=5 if args.ai_junctions else 0,ai_junctions=args.ai_junctions,obstacles=1,original_assets_modified=False,executable_sha256=hashlib.sha256((OUT/'BHunter-box.exe').read_bytes()).hexdigest()),indent=2))
    if args.ai_junctions or args.tunnel_ring:
        (OUT/'Play box.cmd').write_text('@echo off\ncd /d "%~dp0"\nBHunter-box.exe --box-aggression\n')
        (OUT/'Run checks.cmd').write_text('@echo off\ncd /d "%~dp0"\nBHunter-box.exe --box-aggression\n')
    if args.tunnel_ring:
        info=json.loads((OUT/'level.json').read_text())
        info.update(kind='isolated tunnel ring',rooms=8,junctions=0,obstacles=0,
                    bounds=[[-120,-20,-120],[120,20,120]],tunnel_ring=True,
                    actors=dict(neutral=3,moving_targets=2,hunters=2))
        (OUT/'level.json').write_text(json.dumps(info,indent=2))
        (OUT/'Play tunnel ring.cmd').write_text('@echo off\ncd /d "%~dp0"\nBHunter-box.exe --box-aggression\n')
    print(OUT/'BHunter-box.exe')
if __name__=='__main__':main()
