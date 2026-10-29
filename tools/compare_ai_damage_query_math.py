"""Static complete ordinary damage-query instruction graphs; no code execution."""
import argparse
import itertools
import json
import re
import struct
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from compare_ai_damage_query import owner_instructions
from compare_ai_guided import digest
from compare_ai_center import image_bytes
sys.path.insert(0,str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff


def trace(rows,constants,calls,bindings,section,image,source,event,bits,match,previous,next_,room_found=True,special=False,kind=2,sender='other',helpers=None,motion=False,unit_scale=False,fade_equal=False,smoke=False,finish=False,smoked=True,slots=(0,0),acquired=True,tag=False,callback_move=False,tick=False,stop_address=None,rng=False,rng_step=None,boundary_contract=None,ordinary=False,mode3=False,whole=False,due_negative=False,selection=None):
    def ptr(name,offset=0):return ('ptr',name,offset)
    def constant(v):return ('constant',float(v).hex())
    def f32(v):
        if isinstance(v,tuple):
            if v[0] in ('f32','input-float'):return v
            if v[0]=='constant':return constant(struct.unpack('<f',struct.pack('<f',float.fromhex(v[1])))[0])
        return ('f32',v)
    def operation(op,a,b):
        if op in ('add','mul') and repr(a)>repr(b):a,b=b,a
        return (op+'53',a,b)
    def plus(v,n):return ptr(v[1],v[2]+n) if isinstance(v,tuple) and v[0]=='ptr' else v+n
    def word(v):return v&65535 if isinstance(v,int) else v if v[0]=='word' else ('word',v)
    actor,packet,context,pool,sound=map(ptr,('actor','packet','context','pool','sound'))
    memory={4:actor,8:packet,12:context,16:('word','clock'),('context',0):pool,('context',4):ptr('route'),('route',12):sound,
            ('actor',0x1a4):ptr('world'),('world',0x128):actor,('actor',0xf8):ptr('previous') if previous else 0,
            ('actor',0xf4):ptr('next') if next_ else 0,('actor',0x104):('sound-handle',),
            ('actor',0x1b8):ptr('subject') if match&1 else ptr('other-target'),
            ('actor',0x1ac):ptr('subject') if match&2 else ptr('other-owner'),
            ('actor',0x1e4):('word','actor-tick'),('actor',0x1e6):('word','duration'),('packet',0):event}
    for offset in list(range(0x120,0x174,4))+[0x194,0x19c]:memory[('actor',offset)]=('input-float','actor.'+hex(offset))
    for i in range(1,10):memory[('packet',4*i)]=('input-float','packet.'+str(i))
    if event==0:memory[('packet',8)]=ptr('sender')
    if event==17:memory[('packet',4)]=ptr('subject')
    if special:
        memory.update({('context',0):ptr('damage'),('context',8):ptr('vehicles'),('context',16):ptr('player'),('context',24):0,
                       ('damage',4):ptr('route'),('vehicles',0):pool,('actor',0x118):int(previous),('actor',0xfc):kind})
        for offset in (0x180,0x184,0x188,0x198):memory[('actor',offset)]=('input-float','actor.'+hex(offset))
        if event==0:memory[('packet',8)]={'other':ptr('sender'),'player':ptr('player'),'self':actor,'null':0}[sender]
        memory.update({('route',0):ptr('record'),('route',4):1,('route',8):1024,('record',0):ptr('world'),('vehicles',8):actor,('vehicles',12):5,
                       ('actor',0xec):ptr('global-previous') if match&1 else 0,('actor',0xf0):ptr('global-next') if match&2 else 0,
                       ('previous',0xf4):actor,('next',0xf8):actor,('global-previous',0xf0):actor,('global-next',0xec):actor})
    registers=dict(ESP=0,ESI='saved.esi',EDI='saved.edi',EBX='saved.ebx',EBP='saved.ebp')
    if motion:
        memory[8]=context
        if unit_scale:memory[('actor',0x198)]=0x3f800000
        if not source:registers.update(ESI=actor,EDI=0x3f800000,EAX=memory[('actor',0x198)])
    if smoke:
        memory.update({12:ptr('death-position'),('context',4):ptr('attack'),('attack',0):ptr('pursuit'),('damage',8):ptr('particles'),('damage',28):15})
    if finish:
        memory.update({('damage',0):pool,('damage',12):ptr('fragments'),('damage',16):ptr('targets'),('damage',20):ptr('loot-rows'),('damage',24):4,('damage',32):42,('damage',40):ptr('angles'),('context',12):ptr('mission-context'),('context',20):777,
                       ('actor',0x100):1234,('actor',0x114):5678,('actor',0x11c):123 if tag else 0,('actor',0x1dc):slots[0],('actor',0x1de):slots[1],('record',12):ptr('moved-world'),('moved-world',0x128):0,('route',4):2})
        for i in range(3):memory[('death-position',4*i)]=('input-float','death-position.'+str(i));memory[('angles',4*i)]=('input-float','angles.'+str(i))
        memory[('actor',0x198)]=0x3f800000 if unit_scale else 0x40000000
        memory[('actor',0x19c)]=0x3f800000 if fade_equal else 0x3f000000
        if previous:memory[('world',0x128)]=ptr('previous')
        if source:memory.update({8:context,12:ptr('death-position'),16:int(smoked)})
        else:
            registers.update(ESP=-52 if smoked else 0,ESI=actor,EDI=ptr('actor',0x12c) if smoked else 0x3f800000,EBX=ptr('actor',0x150),EBP=ptr('actor',0x144))
            memory.update({0:'saved.edi',4:'saved.esi',8:'saved.ebp',12:'saved.ebx'})
    if tick:
        memory.update({('actor',0x330):context,('context',4):ptr('attack'),('context',12):ptr('mission-context'),('damage',8):ptr('particles'),('damage',12):ptr('fragments'),('damage',16):ptr('targets'),('damage',20):ptr('loot-rows')})
    if rng:
        memory[('actor',0x1e8)]=('rng32','initial')
        registers.update(ESP=-24 if source else -20,ESI=actor,EDI=context if source else 'saved.edi')
    if ordinary and smoke:
        memory.update({('context',4):ptr('route'),('context',8):ptr('particles'),('context',28):15})
        registers.update(ESP=-8 if source else 0,ESI=actor,EDI=context if source else 'saved.edi')
    if ordinary and finish:
        memory.update({('context',0):pool,('context',4):ptr('route'),('context',12):ptr('fragments'),('context',32):42,('context',36):3 if mode3 else 0,('context',40):ptr('angles'),('bound-clock',0):('word','clock')})
        registers.update(ESP=-56 if source else -52,ESI=actor,EDI=context if source else ptr('actor',0x12c),EBX=ptr('actor',0x12c) if source else ptr('actor',0x150),EBP=ptr('actor',0x150) if source else ptr('actor',0x144))
        memory[('actor',0x194)]=('input-float','after-particle.damage')
        for i in range(3):memory[('death-position',4*i)]=('input-float','death-position.'+str(i))
    if whole:
        assert ordinary and motion and smoke and finish and rng
        registers=dict(ESP=0,ESI='saved.esi',EDI='saved.edi',EBX='saved.ebx',EBP='saved.ebp')
        memory.update({4:actor,8:context,('context',16):ptr('targets'),('context',20):ptr('loot-rows'),('context',24):4,('context',28):15,('bound-clock',0):('word','clock')})
        memory[('actor',0x194)]=('input-float','actor.0x194')
        for i in range(3):memory[('spin',4*i)]=('input-float','spin.'+str(i))
    if selection is not None:
        excluded=ptr('candidate'+str(selection['excluded'])) if selection['excluded']>=0 else ptr('excluded')
        player=ptr('player') if selection['player'] else 0
        head=ptr('candidate0') if selection['classes'] else 0
        memory.update({4:player if source else excluded,8:head if source else ptr('origin'),12:excluded,16:ptr('origin'),20:100,24:ptr('selection-result'),('player',0x238):int(selection['player']==2),('player',0x234):('input-int','player-weight')})
        for i in range(3):memory[('origin',4*i)]=('input-float','origin.'+str(i));memory[('player',12+4*i)]=('input-float','player.'+str(i))
        for i,kind_ in enumerate(selection['classes']):
            name='candidate'+str(i)
            memory.update({(name,0xfc):kind_,(name,0xf0):ptr('candidate'+str(i+1)) if i+1<len(selection['classes']) else 0,(name,0xd0):('position-query',name),(name,0x100):('input-int',name+'.weight')})
        if selection.get('iteration'):
            memory.update({('candidate0',0xf0):ptr('successor'),('selection-result',0):ptr('incumbent'),('selection-result',4):('input-float','incumbent.score'),('selection-result',8):0,-76 if source else -56:('input-float','incumbent.score')})
            registers.update(ESP=-88 if source else -72,ESI=ptr('candidate0'),EBX=ptr('origin') if source else ptr('incumbent'),EDI=ptr('selection-result') if source else ptr('origin'),EBP=-4 if source else excluded)
    stack=[];events=[];comparisons=[];covered=set();cursor=0;zero=carry=None;negative=False;comparison=None;curve_count=0;random_count=0;frames=[];scope='core';stage_destination=None
    lookup={r['address']:i for i,r in enumerate(rows)}
    def canonical(v):
        if isinstance(v,int):
            offset=v+(0x90 if source else 0x94) if whole else v
            if ordinary and (smoke or finish):
                base=0x48 if source else 0x40
                if offset in (base,base+4,base+8):return ('death-position',offset-base)
            if whole and not source and offset in (0x88,0x8c,0x90):return ('angles',offset-0x88)
            if (smoke or finish) and not source and v in (0x40,0x44,0x48):return ('death-position',v-0x40)
            if finish and not source and v in (0xb0,0xb4,0xb8):return ('angles',v-0xb0)
            if rng:
                base=0x60 if source else 0x58
                if offset in (base,base+4,base+8):return ('spin',offset-base)
            return v
        if v[0] in ('absolute','clock'):return v
        assert v[0]=='ptr',v
        if v[1]=='stack':return canonical(v[2])
        if whole and v[1]=='room-record' and v[2]==4:return (memory[('actor',0x1a4)][1],0x128)
        if v[1]=='record' and v[2]==4:return ('world',0x128)
        if finish and v[1]=='record' and v[2]==16:return ('moved-world',0x128)
        return v[1:]
    def register(name):
        if name in ('AX','CX','DX','DI'):return word(registers['E'+name])
        if name in ('AL','BL'):return registers['E'+name[0]+'X']&255
        return registers[name]
    def address(operand):
        inner=re.search(r'\[(.*?)\]',operand)[1].strip();result=0
        for term in inner.split('+'):
            term=term.strip()
            if '*' in term:
                a,b=[s.strip() for s in term.split('*')];v=(register(a) if a in registers else int(a,16))*(register(b) if b in registers else int(b,16))
            else:v=register(term) if term in registers else int(term,16)
            if isinstance(v,tuple):assert isinstance(result,int);result=plus(v,result)
            else:result=plus(result,v)
        return result
    def st(operand):
        m=re.fullmatch(r'ST\(?([0-7])?\)?',operand);return int(m[1] or '0') if m else None
    def read(operand,row,floating=False):
        index=st(operand)
        if index is not None:return stack[index]
        if '[' in operand:
            if row['address'] in constants:return constants[row['address']]
            at=canonical(address(operand));binding=bindings.get(row['address'])
            if source and binding and binding['kind']=='local':
                assert isinstance(at,int);at+=binding['target'];width=1 if 'BYTE' in operand else 4
                if width==1:return section['data'][at]
                relocation=next(r for r in section['relocations'] if r['offset']==at)
                return relocation['symbol']['value']+struct.unpack_from('<I',section['data'],at)[0]
            if not source and isinstance(at,int) and at>=0x400000:
                if selection is not None and at in (0x4dfab8,0x4dfb28):return player if at==0x4dfab8 else head
                if at==0x4dfac4:return memory[('bound-clock',0)] if ordinary and finish else ('word','clock')
                if ordinary and at==0x4dfb08:return memory[('context',36)]
                if special and at==0x4dfab8:return ptr('player')
                if smoke and at==0x4f0028:return 15
                if finish and at in (0x4d1918,0x4f5540):return 777 if at==0x4d1918 else 42
                if special and at in (0x4dfb28,0x4dfb2c):return memory[('vehicles',8 if at==0x4dfb28 else 12)]
                width=1 if 'BYTE' in operand else 4;return int.from_bytes(image_bytes(image,at,width),'little')
            v=memory[at]
            if floating and ('QWORD' in operand or 'DOUBLE' in operand) and isinstance(v,tuple) and v[0]=='low64':
                assert memory[at+4]==('high64',v[1]);v=v[1]
            elif floating and ('QWORD' in operand or 'DOUBLE' in operand):
                assert isinstance(at,int) and isinstance(v,int) and isinstance(memory[at+4],int)
                return constant(struct.unpack('<d',struct.pack('<II',v,memory[at+4]))[0])
        elif operand in registers or operand in ('AX','CX','DX','DI'):v=register(operand)
        else:
            binding=bindings.get(row['address'])
            v=binding['value'] if source and binding and binding['kind']=='callback' else {0x435790:('callback','normal-tick'),0x4357d0:('callback','damage-tick'),0x435720:('callback','release-tick'),0x439910:('callback','response-tick'),0x4397e0:('callback','release-tick'),0x437a30:('callback','mission-tick'),0x4398c0:('callback','vehicle-tick'),0x47d680:0}.get(int(operand,16)&0xffffffff,int(operand,16)&0xffffffff)
        return constant(struct.unpack('<f',struct.pack('<I',v))[0]) if floating and isinstance(v,int) else v
    def write(operand,v,row,floating=False):
        index=st(operand)
        if index is not None:stack[index]=v;return
        if '[' not in operand:
            if operand in ('AX','CX','DX','DI'):registers['E'+operand]=word(v)
            elif operand in ('AL','BL'):assert isinstance(v,int);registers['E'+operand[0]+'X']=(registers['E'+operand[0]+'X']&~255)|(v&255)
            elif operand=='CL':assert isinstance(v,int) and registers['ECX']==0;registers['ECX']=v
            else:registers[operand]=v
            return
        at=canonical(address(operand));wide=floating and ('QWORD' in operand or 'DOUBLE' in operand)
        if special and at in (0x4dfb28,0x4dfb2c):at=('vehicles',8 if at==0x4dfb28 else 12)
        if floating and not wide:v=f32(v)
        if wide:assert isinstance(at,int);memory[at]=('low64',v);memory[at+4]=('high64',v)
        else:
            if isinstance(at,tuple) and at[0] in ('actor','cue') and at[1] in (0x168,0x16c,0x170,0x180,0x184,0x194,0x198,0x19c) and isinstance(v,int):v=constant(struct.unpack('<f',struct.pack('<I',v))[0])
            memory[at]=v
        if isinstance(at,tuple) and at[0] in ('actor','cue','packet','world','moved-world','previous','next','context','vehicles','global-previous','global-next','death-position','spin'):
            if not ((tick and at==('context',24) and v==0) or (whole and at==('context',44))):events.append(('store',at,v))
    def vector(p):
        if finish and not source and p==ptr('stack',0xb0):p=ptr('angles')
        return tuple(memory[canonical(plus(p,4*i))] for i in range(3))
    def float_arg(v):return constant(struct.unpack('<f',struct.pack('<I',v))[0]) if isinstance(v,int) else v
    while cursor<len(rows):
        if stop_address is not None and rows[cursor]['address'] in (stop_address if isinstance(stop_address,(set,tuple,list)) else (stop_address,)):
            assert not stack and not frames
            if boundary_contract:boundary_contract(registers,memory)
            return events,comparisons,covered
        row=rows[cursor];cursor+=1;covered.add(row['address'] if scope=='core' else (scope,row['address']));m,arg=row['mnemonic'],row['operands'].upper();parts=[s.strip() for s in arg.split(',')]
        if m=='MOV':write(parts[0],read(parts[1],row),row)
        elif m=='MOVSX':
            v=read(parts[1],row);write(parts[0],((v&65535)-65536 if v&32768 else v&65535) if isinstance(v,int) else ('sx16',v),row)
        elif m=='LEA':v=address(parts[1]);write(parts[0],ptr('stack',v) if isinstance(v,int) and 'ESP' in parts[1] else v,row)
        elif m=='PUSH':v=read(arg,row);registers['ESP']-=4;memory[registers['ESP']]=v
        elif m=='POP':registers[arg]=memory[registers['ESP']];registers['ESP']+=4
        elif m in ('ADD','SUB'):
            a,b=read(parts[0],row),read(parts[1],row)
            if parts[0] in ('AX','CX','DX','DI'):v=('word','subtract',a,b)
            elif whole and parts[0]=='EDI' and isinstance(b,tuple) and b[0]=='word':assert m=='ADD';v=('word','add',b,a)
            elif smoke and isinstance(a,tuple) and a[0]=='rand':assert m=='SUB';v=('isub',a,b)
            else:v=plus(a,b*(1 if m=='ADD' else -1))
            write(parts[0],v,row);zero=v==0
        elif m=='XOR':
            if rng and parts[0]!=parts[1]:v=rng_step('xor',read(parts[0],row),read(parts[1],row));write(parts[0],v,row);zero=v==0
            else:assert parts[0]==parts[1];write(parts[0],0,row);zero=True
        elif m=='IMUL':assert rng and len(parts)==3;write(parts[0],rng_step('mul',read(parts[1],row),read(parts[2],row)),row)
        elif m=='SHR':assert rng;write(parts[0],rng_step('shr',read(parts[0],row),read(parts[1],row)),row)
        elif m in ('INC','DEC'):
            a=read(arg,row);v=a+(1 if m=='INC' else -1) if isinstance(a,int) else ('iadd',a,1 if m=='INC' else -1);write(arg,v,row);zero=v==0
        elif m in ('OR','AND'):
            b=read(parts[1],row);a=0 if m=='OR' and b==0xffffffff else read(parts[0],row)
            v=rng_step(m.lower(),a,b) if rng else 0xffffffff if m=='OR' and b==0xffffffff else (a|b if m=='OR' else a&b) if isinstance(a,int) else ('iand',a,b)
            if selection is not None and parts[0]=='ESP':assert m=='AND';v=a & (b-0x100000000 if b&0x80000000 else b)
            write(parts[0],v,row);zero=v==0;carry=False
        elif m in ('SETE','SETZ','SETNE','SETNZ'):write(arg,int(not zero if m in ('SETNE','SETNZ') else zero),row)
        elif m=='CMP':
            a,b=read(parts[0],row),read(parts[1],row);zero=a==b;carry=a<b if isinstance(a,int) and isinstance(b,int) else None
            if motion and b==0x3f800000 and isinstance(a,tuple):
                if a[0]=='constant':zero=a==constant(1)
                elif a==('input-float','actor.0x198'):zero=False
                else:assert a[0]=='f32';zero=fade_equal
        elif m=='TEST':
            if parts[0]=='AH':
                assert comparison is not None;mask=int(parts[1],16)
                if tick:
                    assert (mask,comparison) in ((0x41,(('input-float','actor.0x184'),constant(0))),(0x40,(('input-float','actor.0x188'),constant(0))));zero=True
                elif special and scope in ('timing','phase-timing'):
                    assert mask==1 and comparison[0][0]=='abs' and comparison[1]==constant(9223372036854775808.0);zero=False
                elif special and source and mask==0x40:
                    assert comparison==(('input-float','actor.0x188'),constant(0));zero=True
                else:
                    assert mask in (1,0x41);decision=(mask,*comparison);reversed_smoke=False
                    if smoke and source and comparison[0][0]=='i2f' and comparison[0][1][0]=='rand':
                        assert mask==1 and comparison[1][0]=='mul53';decision=(0x41,comparison[1],comparison[0]);reversed_smoke=True
                    if not special or decision not in comparisons:comparisons.append(decision)
                    index=comparisons.index(decision) if special else len(comparisons)-1
                    zero=not bool(bits&(1<<index))
                    if reversed_smoke:zero=not zero
            else:
                assert parts[0]==parts[1];v=read(parts[0],row);zero=v==0;carry=False;negative=v<0 if isinstance(v,int) else False
                if whole and parts[0]=='DI':
                    assert v==('word','subtract',('word','subtract',('word','clock'),('word','duration')),('word','actor-tick'));negative=due_negative
        elif m in ('JA','JAE','JB','JBE','JE','JZ','JNE','JNZ','JL','JMP'):
            take=m=='JMP' or (negative if m=='JL' else not carry if m=='JAE' else not carry and not zero if m=='JA' else carry if m=='JB' else carry or zero if m=='JBE' else zero if m in ('JE','JZ') else not zero)
            if take:
                target=read(arg,row) if '[' in arg else int(arg,16)
                if '[' in arg and not source:target-=0x43a1d0 if special else 0x435e80
                cursor=lookup[target]
        elif m in ('FLD','FILD'):
            v=read(arg,row,m=='FLD')
            if rng and m=='FILD' and 'QWORD' in arg:
                assert memory[canonical(plus(address(arg),4))]==0
                assert (isinstance(v,int) and 0<=v<=32767) or (v[0]=='and32' and 32767 in v[1:])
            stack.insert(0,('i2f',v) if m=='FILD' else v)
        elif m=='FXCH':index=st(arg) if arg else 1;stack[0],stack[index]=stack[index],stack[0]
        elif m in ('FST','FSTP'):
            write(arg,stack[0],row,True)
            if m=='FSTP':stack.pop(0)
        elif m in ('FADD','FMUL','FSUB','FSUBR','FDIV','FDIVR','FIDIV'):
            index=st(parts[0]) if len(parts)==2 else 0;left,right=stack[index],read(parts[-1],row,m!='FIDIV')
            if m=='FIDIV':right=('i2f',right)
            if m in ('FSUBR','FDIVR'):left,right=right,left
            stack[index]=operation({'FADD':'add','FMUL':'mul','FSUB':'sub','FSUBR':'sub','FDIV':'div','FDIVR':'div','FIDIV':'div'}[m],left,right)
        elif m in ('FADDP','FMULP','FSUBP','FSUBRP','FDIVP','FDIVRP'):
            index=st(parts[0]) if arg else 1;left,right=stack[index],stack[0]
            if m in ('FSUBRP','FDIVRP'):left,right=right,left
            stack[index]=operation({'FADDP':'add','FMULP':'mul','FSUBP':'sub','FSUBRP':'sub','FDIVP':'div','FDIVRP':'div'}[m],left,right);stack.pop(0)
        elif m=='FSQRT':stack[0]=('sqrt53',stack[0])
        elif m in ('FLDZ','FLD1'):stack.insert(0,constant(0 if m=='FLDZ' else 1))
        elif m=='FABS':stack[0]=('abs',stack[0])
        elif m in ('FCOM','FCOMP','FCOMPP'):
            comparison=(stack[0],stack[1] if m=='FCOMPP' else read(arg,row,True))
            if m!='FCOM':stack.pop(0)
            if m=='FCOMPP':stack.pop(0)
        elif m=='FNSTSW':assert arg=='AX';registers['EAX']=('statusword',comparison)
        elif m=='CALL':
            role='position-query' if selection is not None and '[' in arg else calls[row['address']];result=('call-result',role)
            assert not stack or (special and role=='ftol'),('Live x87 call stack',row,stack)
            def args(n):return [memory[registers['ESP']+4*i] for i in range(n)]
            if role=='position-query':
                candidate,packet=args(2);name=candidate[1];assert read(arg,row)==('position-query',name)
                assert memory[canonical(packet)]==3;events.append(('position-query',candidate,3))
                for i in range(3):memory[canonical(plus(packet,4+4*i))]=('input-float',name+'.position.'+str(i))
                memory[(name,0x100)]=('input-int',name+'.weight-after-query')
                if selection.get('iteration'):
                    for i in range(3):memory[('origin',4*i)]=('input-float','origin.after-query.'+str(i))
                if selection['cut']:memory[(name,0xf0)]=0
            elif role=='curve':
                t,dest,*controls=args(6);assert controls==[ptr('actor',o) for o in (0x120,0x144,0x150,0x12c)]
                events.append(('curve',float_arg(t),dest[1],tuple(vector(p) for p in controls)))
                for i in range(3):memory[canonical(plus(dest,4*i))]=f32(('curve-output',curve_count,i,float_arg(t)))
                curve_count+=1
            elif role=='room':assert args(2)==[context,actor];result=(ptr('room-record') if whole else ptr('record')) if room_found else 0
            elif role=='query':
                a=args(3 if source else 2);assert a==([pool,actor,0] if source else [actor,0]);events.append(('query',pool,actor,0))
                if ordinary and finish:memory[('actor',0x104)]=('sound-handle','after-query')
            elif role=='sound':
                a=args(2 if source else 1);assert a==([sound,memory[('actor',0x104)]] if source else [memory[('actor',0x104)]]);events.append(('sound-stop',sound,a[-1]))
                if ordinary and finish:
                    memory[('bound-clock',0)]=('word','after-sound-clock')
                    memory[('actor',0x1e4)]=('word','after-sound-stamp');memory[('actor',0x1e6)]=('word','after-sound-duration')
                    for o in range(0x120,0x15c,4):memory[('actor',o)]=('input-float','after-sound.'+hex(o))
            elif role=='select' and not source:
                a=args(2);assert a[0]==actor;events.append(('select',actor,vector(a[-1])));result=ptr('selected')
            elif special and role=='selector':
                a=args(6);assert a[:3]==[ptr('player'),actor,actor] and a[4]==1024
                events.append(('select',actor,vector(a[3])));memory[canonical(a[5])]=ptr('selected');result=1
            elif special and role=='finite':
                a=args(2);assert a[0][0]=='low64' and a[1]==('high64',a[0][1]);result=1
            elif tick and role=='live':assert args(2)==[context,actor];result=1
            elif tick and role=='environment':result=1
            elif tick and role=='attack':assert args(2)==[ptr('attack'),actor];events.append(('attack',actor));result=1
            elif tick and role=='motion-stage':assert args(2)==[actor,context];events.append(('stage','motion'));result=1
            elif tick and role=='smoke-stage':
                a=args(3);assert a[:2]==[actor,context];stage_destination=a[2];events.append(('stage','smoke'));result=int(smoked)
            elif tick and role=='finish-stage':assert args(4)==[actor,context,stage_destination,int(smoked)];events.append(('stage','finish',int(smoked)))
            elif rng and role=='rng-step':result=rng_step('step',args(1)[0],None)
            elif whole and role in ('advance','due'):
                assert args(2 if source else 1)==([ptr('route'),actor] if source else [actor])
                if role=='advance' or not due_negative:
                    events.append(('advance',actor));memory[('bound-clock',0)]=('word','post-advance-clock')
                    memory[('actor',0x1e4)]=('word','advance-stamp');memory[('actor',0x1e6)]=('word','advance-duration')
                    memory[('actor',0x1e8)]=('rng32','after-advance');memory[('actor',0x194)]=('input-float','after-advance.damage')
                    for o in range(0x120,0x15c,4):memory[('actor',o)]=('input-float','after-advance.'+hex(o))
                    if source:
                        value=('word','add',('word','post-advance-clock'),('word','subtract',('word','subtract',('word','clock'),('word','duration')),('word','actor-tick')))
                        memory[('actor',0x1e4)]=value;events.append(('store',('actor',0x1e4),value))
                result=1
            elif (smoke or finish) and role=='rand':events.append(('rand',random_count));result=('rand',random_count);random_count+=1
            elif finish and role=='velocity':
                after,at,dest=args(3);a,b=vector(after),vector(at)
                for i in range(3):
                    diff=operation('sub',a[i],b[i]);memory[canonical(plus(dest,4*i))]=f32(operation('mul',diff if i==0 else f32(diff),constant(15)))
            elif finish and role=='debris':
                a=args(6 if source else 5)
                if source:assert a.pop(0)==ptr('fragments')
                events.append(('debris',a[0],vector(a[1]),tuple(float_arg(v) for v in a[2:])))
            elif finish and role=='sound-request':
                a=args(6 if source else 5)
                if source:assert a.pop(0)==sound
                events.append(('sound-request',a[0],float_arg(a[1]),float_arg(a[2]),vector(a[3]),vector(a[4])))
            elif finish and role=='loot':
                a=args(6 if source else 3)
                if source:assert a[:2]==[ptr('loot-rows'),4];a=a[2:]
                events.append(('loot',*a[:3]));result=1 if source else ('loot-result',)
                if source:memory[canonical(a[3])]=('loot-result',)
                if whole:
                    memory[('actor',0x1e8)]=('rng32','after-loot')
                    memory[('actor',0x100)]=('token','after-loot');memory[('actor',0x114)]=('material','after-loot');memory[('actor',0x1ac)]=ptr('owner-after-loot')
            elif finish and role=='acquire':
                if source:assert args(1)==[pool]
                events.append(('acquire',pool));result=ptr('cue') if acquired else 0
            elif finish and role=='mission-bind':assert args(2)==[ptr('mission-context'),ptr('cue')];result=1
            elif finish and role=='construct':
                a=args(12 if source else 11)
                if source:assert a.pop(0)==ptr('targets')
                events.append(('construct',tuple(vector(p) for p in a[:4]),a[4],float_arg(a[5]),float_arg(a[6]),*a[7:]))
                if whole and callback_move:
                    memory[('actor',0x1a4)]=ptr('moved-world');memory[('actor',0xf8)]=0 if previous else ptr('previous');memory[('actor',0xf4)]=0 if next_ else ptr('next')
                    memory[('moved-world',0x128)]=actor if previous else ptr('previous')
            elif finish and role=='broadcast':
                a=args(6 if source else 5)
                if source:assert a.pop(0)==pool
                assert a[:4]==[0,0,0,0];p=canonical(a[4]);event_id=memory[p] if isinstance(p,int) else memory[p]
                packet_actor=memory[canonical(plus(a[4],4))];assert packet_actor==actor
                events.append(('broadcast',event_id,actor,memory[canonical(plus(a[4],8))] if event_id==14 else 0))
                if event_id==17 and callback_move:
                    events.append(('callback-room-move',actor))
                    if not previous:memory[('world',0x128)]=memory[('actor',0xf4)]
                    if previous:memory[('previous',0xf4)]=memory[('actor',0xf4)]
                    if next_:memory[('next',0xf8)]=memory[('actor',0xf8)]
                    memory[('actor',0xf8)]=memory[('actor',0xf4)]=0;memory[('actor',0x1a4)]=ptr('moved-world');memory[('moved-world',0x128)]=actor
            elif smoke and role=='pursuit':
                assert args(2 if source else 1)==([ptr('pursuit'),actor] if source else [actor]);events.append(('pursuit',actor));result=1
                for o in list(range(0x120,0x15c,4))+[0x180,0x184,0x194]:memory[('actor',o)]=('input-float','after-pursuit.'+hex(o))
            elif smoke and role=='particle':
                a=args(14 if source else 13)
                if source:assert a.pop(0)==ptr('particles')
                assert a[:2]==[15,3]
                events.append(('particle',15,3,tuple(tuple(float_arg(v) for v in vector(p)) for p in a[2:5]),tuple(float_arg(v) for v in a[5:])))
                if whole:memory[('actor',0x194)]=('input-float','after-particle.damage')
            elif special and role=='ftol':
                value=stack.pop(0);events.append(('ftol',value));result=('ftol-low32',value)
            elif special and role=='clock':assert args(1)==[ptr('route')];result=memory[('bound-clock',0)] if ordinary and finish else ('word','clock')
            elif special and role=='timing' and not helpers:
                a=args(5);assert a[0]==context
                assert a[1][0]=='low64' and a[2]==('high64',a[1][1]) and a[3][0]=='low64' and a[4]==('high64',a[3][1]);result=1
            elif special and role=='state':
                a=args(2);assert a[0]==actor;memory[canonical(a[1])]=memory[('actor',0x1a4)]
            elif special and role in ('unlink','timing','phase-timing','select','phase-value'):
                assert helpers and role in helpers
                if role=='phase-value':assert args(2)==[context,actor]
                if role=='unlink':assert args(2)==[context,actor]
                if role=='select':assert args(3)[:2]==[context,actor]
                frames.append((rows,constants,calls,bindings,section,lookup,cursor,scope))
                registers['ESP']-=4;memory[registers['ESP']]=('helper-return',)
                rows,constants,calls,bindings,section=helpers[role];lookup={r['address']:i for i,r in enumerate(rows)};cursor=0;scope=role
                continue
            else:assert role=='pool' and args(2 if source else 1)==([pool,actor] if source else [actor]);events.append(('pool-release',pool,actor))
            registers.update(EAX=result,ECX=('clobbered',),EDX=('clobbered',))
        elif m=='RET':
            if frames:
                assert memory[registers['ESP']]==('helper-return',) and (len(stack)==1 if scope=='phase-value' else not stack);registers['ESP']+=4
                rows,constants,calls,bindings,section,lookup,cursor,scope=frames.pop();continue
            assert registers['ESP']==(188 if finish and not source and not whole else 0) and not stack
            if selection is not None:
                assert all(registers[r]=='saved.'+r.lower() for r in ('ESI','EDI','EBX','EBP'))
                if source:assert registers['EAX']==1;target=memory[('selection-result',0)];score=memory[('selection-result',4)]
                else:target=registers['EAX'];score=memory[-56]
                events.append(('selection-result',target,float_arg(score)))
            if not motion or source or finish:assert all(registers[r]=='saved.'+r.lower() for r in ('ESI','EDI','EBX','EBP'))
            return events,comparisons,covered
        else:raise AssertionError(('Unreviewed query instruction',row))
    raise AssertionError('No return')


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_damage.obj');parser.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-damage-query-match/math-audit');a=parser.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    obj=a.object.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_damage_query_core');section=sections[owner['section']]
    source,listing,end,_=owner_instructions(a.object,symbols,sections,owner['name'])
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=[]
    for row in json.loads((ROOT/'META/m4/target600/b3/original/00435e80.disasm.json').read_text()):
        va=int(row['address'],16);data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J') and '[' not in arg:arg=hex(int(arg,16)-0x435e80)
        original.append(dict(address=va-0x435e80,data=data,mnemonic=row['mnemonic'],operands=arg))
    def constant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    old_constants={};new_constants={};old_calls={};new_calls={};bindings={}
    for row in original:
        if row['mnemonic']=='CALL':old_calls[row['address']]={0x4341a0:'curve',0x476e90:'query',0x47c140:'sound',0x477070:'pool'}[0x435e80+row['address']+5+struct.unpack_from('<i',row['data'],1)[0]]
        elif row['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',row['operands'])
            if m:old_constants[row['address']]=constant(image_bytes(image,int(m[1],16),8 if 'double' in row['operands'] else 4))
    for r in section['relocations']:
        if r['offset']>=end:continue
        row=next(v for v in source if v['address']<=r['offset']<v['address']+len(v['data']));s=r['symbol'];at=row['address']
        if r['kind']==20:new_calls[at]={'_bh_camera_curve':'curve','_damage_query_room':'room','_bh_entity_set_query':'query','_bh_sound_stop':'sound','_bh_entity_release':'pool'}[s['name']]
        elif s['section']==owner['section']:bindings[at]=dict(kind='local',target=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0])
        elif s['name'] in ('_release_tick','_bh_actor_damage_tick'):bindings[at]=dict(kind='callback',value=('callback','release-tick' if s['name']=='_release_tick' else 'damage-tick'))
        else:
            assert r['kind']==6 and row['mnemonic'].startswith('F')
            size=8 if 'QWORD' in row['operands'].upper() else 4;data_at=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0];new_constants[at]=constant(sections[s['section']]['data'][data_at:data_at+size])
    # Reviewed source registry adapter: fresh +1a4, stable count/base,12B stride,
    # first equal world, no calls/stores, NULL on absence. Linked body audited
    # by compare_ai_damage_query.py; no guessed original-global pointer.
    helper=next(s for s in symbols.values() if s['name']=='_damage_query_room');h=sections[helper['section']]
    assert h['data'].hex()=='8b4c24048b442408568b49048bb0a401000033c0578b510485d2761e8b398bcf3931740d4083c10c3bc272f433c05f5ec38d14408d04975f5ec35f5e33c0c390' and not h['relocations']
    proofs=[];ocover=set();ncover=set()
    for event,bits,match,previous,next_ in itertools.product(list(range(22))+[0xffffffff],range(4),range(4),(False,True),(False,True)):
        old,oc,ov=trace(original,old_constants,old_calls,{},None,image,False,event,bits,match,previous,next_)
        new,nc,nv=trace(source,new_constants,new_calls,bindings,section,image,True,event,bits,match,previous,next_)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(event=event,bits=bits,match=match,previous=previous,next=next_,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n');raise AssertionError('Damage query graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(event=event,bits=bits,match=match,previous=previous,next=next_,graph_sha256=digest(json.dumps((old,oc)).encode())))
    _,_,failure=trace(source,new_constants,new_calls,bindings,section,image,True,4,0,0,False,False,False);ncover|=failure
    assert ocover=={r['address'] for r in original};assert ncover=={r['address'] for r in source}
    result=dict(original_va='00435e80',original_body_bytes=1312,source_body_bytes=end,byte_match=False,valid_path_graphs_equal=True,all_original_and_source_instructions_covered=True,cases=len(proofs),object_sha256=digest(obj),registry_adapter_audited=True,proofs=proofs,
                limits=['PC53 nearest/masked; same-value sum/product commutation and bounded identity double spills only.','Real curve/pool/query/sound definitions are compared by caller arguments, not internal original implementation.','Source typed context/clock/registry guards and no-op callback binding are explicit adapters.','PC24/64, NaN payloads, exception flags and arbitrary packet/entity aliasing are unproved.'])
    (out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='proofs'}))


if __name__=='__main__':main()
