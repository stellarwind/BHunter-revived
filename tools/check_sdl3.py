"""Run SDL menu/GPU diagnostics without desktop automation or extra Python packages."""
import argparse, csv, hashlib, json, math, os, re, shutil, struct, subprocess, zlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def png(path):
    data=path.read_bytes();offset=8;compressed=bytearray();width=height=0
    assert data[:8]==b'\x89PNG\r\n\x1a\n'
    while offset<len(data):
        size=struct.unpack_from('>I',data,offset)[0];kind=data[offset+4:offset+8];body=data[offset+8:offset+8+size]
        if kind==b'IHDR':
            width,height,depth,color,*_=struct.unpack('>IIBBBBB',body)
            assert (depth,color)==(8,6), 'Expected SDL RGBA8 PNG'
        if kind==b'IDAT':compressed.extend(body)
        offset+=size+12
    raw=zlib.decompress(compressed);stride=width*4+1
    # SDL_SavePNG emits unfiltered RGBA scanlines.
    assert all(raw[y*stride]==0 for y in range(height))
    return width,height,[raw[y*stride+1:(y+1)*stride] for y in range(height)]

def write_png(path,w,h,rows):
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    path.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,6,0,0,0))+
                     chunk(b'IDAT',zlib.compress(b''.join(b'\0'+row for row in rows)))+chunk(b'IEND',b''))

def normalize(path):
    w,h,rows=png(path);scale=min(w/640,h/480);left=(w-640*scale)/2;top=(h-480*scale)/2
    return [b''.join(rows[min(h-1,int(top+(y+.5)*scale))][min(w-1,int(left+(x+.5)*scale))*4:][:4]
                     for x in range(640)) for y in range(480)]

def bounds(rows):
    points=[(x,y) for y,row in enumerate(rows) for x in range(640) if max(row[x*4:x*4+3])>8]
    assert points,'Empty render fixture'
    return [min(p[0] for p in points),min(p[1] for p in points),max(p[0] for p in points),max(p[1] for p in points)]

def compare(root):
    results={}
    for size in ('1920x1080','3840x2160'):
        for name in ('menu-00495e40-row-0','menu-00496220-row-0','menu-00496220-row-2','effect-5'):
            a=normalize(root/'640x480'/f'{name}.png');b=normalize(root/size/f'{name}.png');ba,bb=bounds(a),bounds(b)
            rmse=math.sqrt(sum((x-y)**2 for ar,br in zip(a,b) for i,(x,y) in enumerate(zip(ar,br)) if i%4!=3)/(640*480*3))
            results[f'{size}/{name}']={'rmse_8bit':round(rmse,4),'reference_bounds':ba,'normalized_bounds':bb}
            assert max(abs(x-y) for x,y in zip(ba,bb))<=2, f'UI dimensions changed: {size}/{name}'
            assert rmse<20, f'Unexpected visual change: {size}/{name}: {rmse}'
            if size=='3840x2160' and name=='menu-00496220-row-0':
                write_png(root/'menu-4k-normalized.png',640,480,b)
                diff=[bytes(min(255,abs(x-y)*4) if i%4!=3 else 255 for i,(x,y) in enumerate(zip(ar,br))) for ar,br in zip(a,b)]
                write_png(root/'menu-4k-diff.png',640,480,diff)
    return results

def compare_reference(root, reference):
    results={}
    for name in ('menu-00495e40-row-0','effect-0','effect-1','effect-2','effect-3','effect-4','effect-5','effect-6','effect-7','effect-8'):
        _,_,a=png(reference/f'{name}.png');_,_,b=png(root/'640x480'/f'{name}.png')
        error=math.sqrt(sum((x-y)**2 for ar,br in zip(a,b) for i,(x,y) in enumerate(zip(ar,br)) if i%4!=3)/(640*480*3))
        assert error<4, f'Original renderer parity regression: {name}: {error}'
        results[name]={'rmse_8bit':round(error,4)}
        if name=='menu-00495e40-row-0':
            mask=[(x,y) for y in range(210,265) for x in range(230,405) if min(a[y][x*4:x*4+3])>180]
            assert len(mask)>100, 'Reference lacks readable menu lettering'
            brightness=sum(sum(b[y][x*4:x*4+3]) for x,y in mask)/sum(sum(a[y][x*4:x*4+3]) for x,y in mask)
            assert brightness>.9, 'Menu font became dark relative to original'
            results[name]['selected_text_brightness_ratio']=brightness
    return results

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--reuse-captures',action='store_true');parser.add_argument('--reference-dir',type=Path,help='Original renderer fixture PNG directory');parser.add_argument('--runtime-dir',type=Path,default=Path('build'));args=parser.parse_args()
    runtime=(ROOT/args.runtime_dir).resolve()
    cache=ROOT/'build-cache'
    if runtime!=(ROOT/'build').resolve():cache=cache/'targets'/hashlib.sha256(str(runtime).encode()).hexdigest()[:12]
    out=cache/'sdl-captures';out.mkdir(parents=True,exist_ok=True)
    record=json.loads((cache/'build.json').read_text());assert record.get('passed') and record.get('debugging'),'Build --debugging first'
    gcc=shutil.which('gcc');assert gcc,'32-bit MinGW gcc required for timing checks'
    timing=out/'timing-tests.exe'
    subprocess.run([gcc,'-O2','-I'+str(ROOT/'src'),str(ROOT/'tools/tests/sdl_timing.c'),*[str(ROOT/name) for name in ('src/bh_render_interpolation.c','src/bh_rotation.c','src/bh_math.c','src/bh_round.c')],'-o',str(timing)],check=True)
    subprocess.run([str(timing)],check=True)
    portal_test=out/'portal-clip-tests.exe'
    portal_sources=['tools/tests/portal_clip.c','src/bh_portal_clip.c','src/bh_clipper.c','src/bh_matrix_inverse.c','src/bh_math.c','src/bh_round.c']
    subprocess.run([gcc,'-O2','-I'+str(ROOT/'src'),*[str(ROOT/name) for name in portal_sources],'-o',str(portal_test)],check=True)
    subprocess.run([str(portal_test)],check=True)
    if not args.reuse_captures:
        for size in ('640x480','1920x1080','3840x2160'):
            captures=out/size;captures.mkdir(exist_ok=True)
            env=dict(os.environ,BH_RENDER_CAPTURE_DIR=str(captures),BH_GPU_TEST_SIZE=size,BH_PLAYER_INTERPOLATION_CHECK='1')
            subprocess.run([str(runtime/'BHunter-source.exe'),'--startup-check'],env=env,timeout=60,check=True)
            log=(runtime/'startup-check.log').read_text();assert 'SDL menu navigation:' in log;assert 'SDL player interpolation:' in log
            shutil.copy2(runtime/'startup-check.log',captures/'startup-check.log')
            print(size, 'menu navigation and GPU fixtures passed',flush=True)
        subprocess.run([str(runtime/'BHunter-source.exe')],env=dict(os.environ,BH_MENU_SMOKE_SECONDS='8',BH_RENDER_CAPTURE_DIR=str(out/'640x480')),timeout=30,check=True)
    seams={}
    flare_replay={}
    target_tracking={}
    for size in ('640x480','1920x1080','3840x2160'):
        log=(out/size/'startup-check.log').read_text()
        match=re.search(r'SDL portal seam pixels: stale camera=(\d+) blue gap pixels, render camera=(\d+)',log)
        assert match, f'Missing portal seam positive/negative controls: {size}'
        before,after=map(int,match.groups());assert before>=10 and after==0
        seams[size]={'stale_camera_gap_pixels':before,'render_camera_gap_pixels':after}
        images={n:png(out/size/f'effect-{n}.png')[2] for n in (6,7,9,10,11)}
        assert images[9]==images[7], f'Fresh interpolated flare changed: {size}'
        assert images[10]==images[6], f'Consumed flare resurrected on second render: {size}'
        assert images[11]==images[7], f'Next tick failed to publish fresh flare: {size}'
        assert images[6]!=images[7], f'Flare positive control is invisible: {size}'
        flare_replay[size]={'first_render_matches_fresh':True,'second_render_matches_trail_only':True,'new_tick_restores_fresh':True}
        for fraction in range(3):
            actual=png(out/size/f'target-tracking-{fraction}-interpolated.png')[2]
            expected=png(out/size/f'target-tracking-{fraction}-reference.png')[2]
            assert actual==expected, f'Target brackets drift from render-pose reference: {size}/{fraction}'
            bounds(normalize(out/size/f'target-tracking-{fraction}-interpolated.png')) # Visible positive control.
        target_tracking[size]={'render_fractions':3,'pixel_differences':0}
    report={'scaling':compare(out),'player_pose_checks_per_resolution':48,'portal_seams':seams,'flare_replay':flare_replay,'target_tracking':target_tracking}
    if args.reference_dir:report['original_renderer']=compare_reference(out,args.reference_dir)
    smoke=json.loads((runtime/'menu-smoke-summary.json').read_text());report['menu_smoke']=smoke
    assert smoke['error']==0 and smoke['music_failures']==0
    assert smoke['simulation_ticks']==smoke['sound_ticks']
    assert abs(smoke['simulation_ticks']/smoke['seconds']-30)<1
    if smoke['local_music']:assert smoke['music_position']>500 and smoke['music_plays']>=1
    newest=max(runtime.glob('frame-times-*.csv'),key=lambda p:p.stat().st_mtime)
    rows=[r for r in csv.DictReader(newest.open()) if float(r['time_ms'])>1000]
    mean=sum(float(r['interval_ms']) for r in rows)/len(rows);report['steady_render_interval_ms']=mean
    assert 15<mean<19, f'Menu pacing differs from 60 Hz: {mean} ms'
    objdump=shutil.which('objdump');assert objdump
    for name in ('BHunter-source.exe','BHunterGPU.dll','SDL3.dll'):
        imports=subprocess.check_output([objdump,'-p',str(runtime/name)],text=True)
        assert 'dll name: ddraw.dll' not in imports.lower()
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    print('SDL menu checks passed. Report:',out/'results.json')
if __name__=='__main__':main()
