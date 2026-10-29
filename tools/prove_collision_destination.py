"""SMT refinement of actual production destination work-stack scheduling.

The contact/plane predicate is an explicit dependency cut, not a whole-owner
proof. Reads immutable original instructions and real production COFF only.
"""
import argparse
import json
from pathlib import Path
import prove_collision_iteration as engine
from prove_collision_iteration import z, B, State, walk, read_owner, prove, feasible
from compare_ai_damage_query import digest, image_bytes

ROOT = Path(__file__).resolve().parents[1]
Q, G, A, NODES, STACK, SP = engine.Q, engine.G, engine.A, engine.NODES, engine.STACK, engine.SP


def no_call(*args):
    raise AssertionError(('Unexpected call', args))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object', type=Path, default=ROOT/'build/game/bh_collision_query.obj')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    engine.DUMP_SMT = args.output/'smt'
    engine.DUMP_SMT.mkdir(exist_ok=True)
    rows, calls, listing, size = read_owner(args.object, '_destination_tree')
    image = (ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image) == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    raw = json.loads((ROOT/'META/m4/collision-traversal/original/0044c200.disassembly.json').read_text())
    assert b''.join(bytes.fromhex(r['bytes']) for r in raw) == image_bytes(image, 0x44c200, 2591)
    original = [dict(address=int(r['address'], 16)-0x44c200, data=bytes.fromhex(r['bytes']),
                     mnemonic=r['mnemonic'], operands=', '.join(r['operands'])) for r in raw]
    for row in original:
        if row['mnemonic'].startswith('J'):
            row['operands'] = hex(int(row['operands'], 0)-0x44c200)
    ocalls = {r['address']: 'self' for r in original if r['mnemonic'] == 'CALL' and r['operands'] == '0x0044c200'}
    an, bn, front, back, depth, capacity = z.BitVecs('an bn front back depth capacity', 32)
    count = z.If(back >= 0, B(1), B(0)) + z.If(front >= 0, B(1), B(0))
    base = [an >= 0, an < 65535, bn >= 0, bn < 65535,
            z.Or(front == -1, z.And(front >= 0, front < 65535)),
            z.Or(back == -1, z.And(back >= 0, back < 65535)),
            z.UGE(depth, 1), z.ULE(depth, 65535), z.ULE(capacity, 65536),
            z.UGE(capacity, depth+1),
            z.Implies(z.And(front >= 0, back >= 0), z.UGE(capacity, depth+2))]
    mat, world, out = B(0x7000000), B(0x7100000), B(0x7200000)
    transitions = []
    for gate, contact in ((0, None), (1, 2), (1, 1), (1, 0)):
        name = 'destination.'+str(gate)+'.'+str(contact)
        s = State(name, 0x2f)
        s.path = list(base)
        s.reg.update(ESP=B(SP), ESI=B(Q), EBX=4*depth, EBP=depth+2)
        for at, value in ((SP+20, Q), (SP+24, an), (SP+28, bn),
                          (SP+32, mat), (SP+36, world), (SP+40, out),
                          (Q+28, STACK), (Q+40, capacity), (Q+4, G), (G, A), (A+16, NODES),
                          (B(STACK)+4*(depth-1), bn),
                          (B(NODES)+56*bn+48, front), (B(NODES)+56*bn+52, back)):
            s.store(at, value)
        before = s.mem

        def dependency(st, callee):
            ptr = st.reg['ESP']
            if callee == '_step':
                expected, result = [B(Q)], 1
            elif callee == '_spheres':
                expected, result = [B(Q), an, bn, mat, B(0)], gate
            else:
                assert callee == '_node_contacts' and contact is not None
                expected, result = [B(Q), an, bn, mat, world, out], contact
            prove(name+'.arguments.'+callee, st.path,
                  z.And(*[st.load(ptr+4*i) == v for i, v in enumerate(expected)]))
            st.events.append(callee)
            st.reg.update(EAX=B(result), ECX=z.BitVec(name+callee+'.ecx', 32),
                          EDX=z.BitVec(name+callee+'.edx', 32))

        states = walk(rows, calls, s, {0x2f}, dependency)
        for i, st in enumerate(states):
            suffix = name+'.path.'+str(i)
            expected_events = ['_step', '_spheres'] + (['_node_contacts'] if gate else [])
            assert st.events == expected_events
            if contact == 1:
                assert st.returned
                prove(suffix+'.first-hit-return', st.path,
                      z.And(st.reg['EAX'] == 1, st.reg['ESP'] == SP+16))
                continue
            next_depth = depth-1 + (count if contact == 0 else B(0))
            if st.returned:
                prove(suffix+'.empty-miss', st.path,
                      z.And(next_depth == 0, st.reg['EAX'] == 0, st.reg['ESP'] == SP+16))
            else:
                prove(suffix+'.stack-register-invariant', st.path,
                      z.And(next_depth > 0, st.reg['EBX'] == 4*next_depth,
                            st.reg['EBP'] == next_depth+2, st.reg['ESP'] == SP,
                            st.reg['ESI'] == Q, z.ULE(next_depth+1, capacity)))
                if contact == 0:
                    if feasible(st.path+[count > 0]):
                        prove(suffix+'.front-first-top', st.path+[count > 0],
                              st.load(B(STACK)+4*(next_depth-1)) == z.If(front >= 0, front, back))
                    both = st.path+[front >= 0, back >= 0]
                    if feasible(both):
                        prove(suffix+'.back-waits-under-front', both,
                              st.load(B(STACK)+4*(next_depth-2)) == back)
            p = z.BitVec(suffix+'.prefix-index', 32)
            prove(suffix+'.pending-prefix-preserved', st.path+[z.ULT(p, 65535)],
                  z.Implies(z.ULT(p, depth-1),
                            st.load(B(STACK)+4*p) == z.Select(before, B(STACK)+4*p)))
            transitions.append(dict(case=name, returned=st.returned, covered=sorted(st.covered)))

    # Actual production entry; valid root initializes exactly one pending node.
    # Negative roots do not call predicates or touch the pending-node array.
    for negative in (False, True):
        s = State('entry.'+str(negative))
        s.reg['ESP'] = B(SP)
        s.path = [z.Or(an < 0, bn < 0)] if negative else [an >= 0, bn >= 0]
        for at, value in ((SP+4, Q), (SP+8, an), (SP+12, bn), (Q+28, STACK)):
            s.store(at, value)
        saved = dict(s.reg)
        for st in walk(rows, calls, s, {0x2f}, no_call):
            if negative:
                assert st.returned
                prove('entry.negative-zero', st.path, z.And(st.reg['EAX'] == 0, st.reg['ESP'] == SP,
                      *[st.reg[r] == saved[r] for r in ('EBX', 'EBP', 'ESI', 'EDI')]))
            else:
                assert not st.returned
                prove('entry.initial-pending-stack', st.path,
                      z.And(st.load(STACK) == bn, st.reg['EBX'] == 4, st.reg['EBP'] == 3,
                            st.reg['ESI'] == Q, st.reg['ESP'] == SP-16))

    s = State('original.negative-entry')
    s.reg['ESP'] = B(SP)
    s.path = [z.Or(an < 0, bn < 0)]
    s.store(SP+4, an)
    s.store(SP+8, bn)
    saved = dict(s.reg)
    for st in walk(original, ocalls, s, {0x25}, no_call):
        assert st.returned
        prove('original.negative-is-zero', st.path, z.And(st.reg['EAX'] == 0, st.reg['ESP'] == SP,
              *[st.reg[r] == saved[r] for r in ('EBX', 'EBP', 'ESI', 'EDI')]))

    # Original post-contact recursion. The front result suppresses the back
    # call on hit. Miss/negative front advances to back; actual NEG/SBB/NEG
    # normalization is checked separately as a bitvector identity below.
    fr, br = z.BitVecs('original.front-result original.back-result', 32)
    s = State('original.children', 0x858)
    s.path = [z.Or(fr == 0, fr == 1), z.Or(br == 0, br == 1)]
    s.reg.update(ESP=B(SP), EBP=B(SP+0x1000), EBX=B(NODES))
    s.store(SP+0x1008, an)
    s.store(NODES+48, front)
    s.store(NODES+52, back)

    def recurse(st, callee):
        assert callee == 'self'
        which = 'front' if not st.events else 'back'
        prove('original.child-arguments.'+which, st.path,
              z.And(st.load(st.reg['ESP']) == an,
                    st.load(st.reg['ESP']+4) == (front if which == 'front' else back)))
        st.events.append(which)
        st.reg.update(EAX=fr if which == 'front' else br,
                      ECX=z.BitVec(which+'.ecx', 32), EDX=z.BitVec(which+'.edx', 32))

    for st in walk(original, ocalls, s, {0x871, 0x885}, recurse):
        if st.pc == 0x871:
            assert st.events == ['front']
            prove('original.front-short-circuit', st.path, z.And(fr == 1, st.reg['EAX'] == 1))
        else:
            assert st.events == ['front', 'back']
            prove('original.back-after-front-miss', st.path, z.And(fr == 0, st.reg['EAX'] == br))
    assert image_bytes(image, 0x44ca85, 6).hex() == 'f7d81bc0f7d8'
    result = z.BitVec('normalization.input', 32)
    carry_after_neg = result != 0
    sbb_eax_eax = z.If(carry_after_neg, B(-1), B(0))
    prove('original.NEG-SBB-NEG.boolean-normalization', [],
          -sbb_eax_eax == z.If(result == 0, B(0), B(1)))

    (args.output/'production-destination.asm.txt').write_text(listing)
    record = dict(passed=True, static_only=True, candidate_read=False,
                  classification='Close: scoped destination scheduling refinement; contact and geometry remain dependencies',
                  object=str(args.object), object_sha256=digest(args.object.read_bytes()),
                  original_sha256=digest(image), original_owner='0044c200', original_bytes=2591,
                  production_driver_bytes=size, obligations=engine.PROOFS, transitions=transitions,
                  relation='Pending node array bottom-to-top denotes original remaining recursive activations in reverse visitation order. Pop processes the next original activation; back then front pushes preserve original front-first recursion. Negative children are omitted original zero-result calls. First contact hit terminates all pending work.',
                  induction='Root lemma initializes relation. Universal arbitrary-depth transition lemmas preserve pending prefix and selected child order; each pop either stutters an original rejection, returns the first hit, or extends ordered pending work. Capacity: each paused ancestor contributes at most one pending back child. Pending depth is at most original activation depth; a node with two children has a proper descendant, so depth+2<=N+1. Finite acyclic-tree/path induction yields equal contact call order and hit/miss result under corresponding dependency responses. This induction is reviewed, not an automated whole-program termination theorem.',
                  domain=['Valid nonaliasing normalized logical views; immutable acyclic finite topology with node indices below65535',
                          'Capacity N+1 and pending-depth invariant depth+1<=capacity; original and production geometry predicates correspond and do not modify traversal state',
                          'Bounded work<=1048576; owner/format/finite/error guards are explicit source adapters'],
                  limits=['Full node_contacts nested face loops and numerical composition remain Functional/proof-open; this driver proof does not promote whole0044c200',
                          'Original plane arithmetic and source graph are separate evidence; matrix transforms and sphere/contact dependency implementations require independent equivalence',
                          'No candidate read, original byte matching, arbitrary invalid-domain, FP-exception, or PC24/64 claim'])
    (args.output/'destination.json').write_text(json.dumps(record, indent=2)+'\n')
    print(str(len(engine.PROOFS))+' production/original destination scheduling obligations proved UNSAT')


if __name__ == '__main__':
    main()
