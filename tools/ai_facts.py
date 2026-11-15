#!/usr/bin/env python3
# ai_facts - extract machine-comparable "facts" from a decompiled nation AI
# DLL (dump produced by DumpDecomp.java) + the raw PE strings.
#
# Part of the nation-AI porting workflow (docs/AI.md). Produces JSON:
#   - registration name<->DAT_ maps (units + upgrades), the per-game "pick"
#     strings, InitAI constants (radii/workers), Assign* role names
#   - token sequences of the water handler + upgrade chains with every
#     DAT_ object reference normalized positionally, so two nations'
#     handlers align call-by-call to derive the name map.
#
# Usage: ai_facts.py <decomp_dir> <dll>   (JSON to stdout)
import json
import re
import subprocess
import sys

REG_APIS = {'RegisterUnitType', 'RegisterUpgrade', 'SafeRegisterUpgrade'}
# query-only APIs: legal inside helper functions (officers tail, chains)
# without disqualifying them from shape-based detection
READERS = {'GetUnits', 'GetUnitsByUsage', 'GetAIData', 'UpgIsDone', 'UpgIsRun'}
SYM = r's_[A-Za-z0-9_]+?_(10[0-9a-f]{6})'


def load_strings(dll):
    out = subprocess.run(
        [sys.executable, __file__.replace('ai_facts.py', 'pe_dump.py'), dll, 'strings'],
        capture_output=True, text=True, check=True, encoding='latin1').stdout
    m = {}
    for line in out.splitlines():
        va, s = line.split(' ', 1)
        m[int(va, 16)] = s[1:-1]
    return m


def split_functions(txt):
    return [(m.group(1), m.group(2), m.group(3)) for m in re.finditer(
        r'// ===== (\S+) @ ([0-9a-f]+) =====\n(.*?)(?=\n\n\n|// ===== |\Z)', txt, re.S)]


def tokenize(body, slotmap, strings):
    """One entry per API call: (api, normalized_args, [dat_refs]).
    Calls may sit inside assignment lines (iVar1 = (*DAT_x)(...);) or in
    compound conditions (cVar2 = (*DAT_x)(&DAT_y), cVar2 == '\\0'). The arg
    list is read up to the call's own balanced closing paren - NOT up to
    the next ';', which would glue a condition-embedded call to a later
    statement (the FUN_/LAB_ addresses in the glue differ per nation and
    break the positional alignment)."""
    toks = []
    for mo in re.finditer(r'\(\*(DAT_[0-9a-f]+)\)\(', body):
        slot = mo.group(1)
        args, i = [], mo.end()
        depth = 1
        while i < len(body):
            c = body[i]
            if c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
                if depth == 0:
                    break
            args.append(c)
            i += 1
        if depth != 0 or i >= len(body):
            continue  # unterminated - decompiler artifact, skip
        args = ''.join(args)
        api = slotmap.get(slot.lower(), slot)
        refs, norm = [], args
        for ref in re.findall(r'&?(_?DAT_[0-9a-f]+)', args):
            refs.append(ref)
            norm = norm.replace(ref, '$%d' % len(refs), 1)
        norm = re.sub(SYM, lambda mo2: repr(strings.get(int(mo2.group(1), 16), mo2.group(0))), norm)
        toks.append((api, norm, refs))
    return toks


def main():
    decomp_dir, dll = sys.argv[1], sys.argv[2]
    txt = open(decomp_dir + '/decomp.txt').read()
    strings = load_strings(dll)

    slotmap = {}
    for slot, sym in re.findall(r'(_?DAT_[0-9a-f]+) = FUN_[0-9a-f]+\(s_([A-Za-z0-9_]+?)_(?:10[0-9a-f]{6})',
                                txt):
        slotmap[slot.lower()] = sym

    funcs = split_functions(txt)
    reg_units, reg_upgrades, buf_units = [], [], {}
    UPG_RE = re.compile(r'^(AKA\d+|KUZ\d+|MAIN)[a-zA-Z]{2}$|'
                        r'(ATTACK[3-7]?|SHIELD[3-7]?|PAUSE[3-7]?|BUILD[3-6]?|GETRES2?|INSIDE[3-7]?)$')
    def reg_kind(api, name):
        if api in REG_APIS:
            return api
        # local code pointer: classify by the name shape
        return 'RegisterUpgrade' if UPG_RE.search(name) else 'RegisterUnitType'
    for mo in re.finditer(r'\(\*(DAT_[0-9a-f]+|pcVar\w+)\)\(&(DAT_[0-9a-f]+),\s*(s_[A-Za-z0-9_]+?_(10[0-9a-f]{6}))\)', txt):
        api = slotmap.get(mo.group(1).lower())
        name = strings.get(int(mo.group(4), 16), mo.group(3))
        kind = reg_kind(api, name) if api in REG_APIS or 'pcVar' in mo.group(1) else api
        if kind not in REG_APIS:
            continue
        name = strings.get(int(mo.group(4), 16), mo.group(3))
        (reg_units if kind == 'RegisterUnitType' else reg_upgrades).append((mo.group(2), name))
    # thunk-form registrations: FUN_xxx(&DAT_obj, s_name) - the thunk wraps
    # RegisterUpgrade/SafeRegisterUpgrade; classify by the name shape
    for mo in re.finditer(r'FUN_[0-9a-f]+\(&(DAT_[0-9a-f]+),\s*(s_[A-Za-z0-9_]+?_(10[0-9a-f]{6}))\)', txt):
        name = strings.get(int(mo.group(3), 16), mo.group(2))
        kind = 'RegisterUpgrade' if UPG_RE.search(name) else 'RegisterUnitType'
        (reg_upgrades if kind == 'RegisterUpgrade' else reg_units).append((mo.group(1), name))
    for mo in re.finditer(r'\(\*(DAT_[0-9a-f]+|pcVar\w+)\)\(&(DAT_[0-9a-f]+),\s*&?(acStack_\w+|DAT_10[0-9a-f]{6})\)', txt):
        callee = mo.group(1)
        slot = slotmap.get(callee.lower())
        if slot == 'RegisterUnitType' or ('pcVar' in callee and slot is None):
            buf_units[mo.group(2)] = mo.group(3)

    # pick copies: the source may be an s_ symbol or a raw &DAT_ address
    # (Ghidra leaves unnamed); the destination may be an acStack_ buffer or
    # a global DAT buffer. Track sources in order of appearance.
    picks = {}
    pick_order = []
    init_fnames = {'InitAI'} | {n for n, a, b in funcs
                                if len(re.findall(r'\(\*DAT_[0-9a-f]+\)\(&', b)) >= 40}
    for fname, addr, body in funcs:
        if fname not in init_fnames:
            continue
        last_src = None
        for line in body.split('\n'):
            mo = re.search(r'= (?:s_[A-Za-z0-9_]+?_(10[0-9a-f]{6})|(&DAT_10[0-9a-f]{6}))', line)
            if mo:
                va = mo.group(1) or mo.group(2).replace('&', '').replace('DAT_', '')
                if int(va, 16) in strings:
                    last_src = int(va, 16)
            mo2 = re.search(r'=\(char\*\)&(DAT_10[0-9a-f]{6})', line.replace(' ', ''))
            mo3 = re.search(r'acStack_(\w+);', line)
            dest = None
            if mo2:
                dest = mo2.group(1)
            elif mo3 and '=acStack_' + mo3.group(1) in line.replace(' ', ''):
                dest = 'acStack_' + mo3.group(1)
            if dest and last_src is not None:
                picks[dest] = strings.get(last_src, str(last_src))
                pick_order.append(picks[dest])
                last_src = None
    units = [(o, n) for o, n in reg_units if o not in buf_units]
    for obj, buf in buf_units.items():
        if buf in picks:
            units.append((obj, picks[buf]))
        elif buf.startswith('DAT_10'):
            # a raw string address used directly as the name argument
            s = strings.get(int(buf[4:], 16))
            if s:
                units.append((obj, s))

    # ordered registration sequence from InitAI (+ heavy callee for
    # Austria-style wrappers) - basis for positional order alignment
    init_bodies_ = [b for n, a, b in funcs if n == 'InitAI']
    for n, a, b in funcs:
        if len(re.findall(r'\(\*DAT_[0-9a-f]+\)\(&', b)) >= 40 and b not in init_bodies_:
            init_bodies_.append(b)
    reg_order = []
    for body in init_bodies_:
        for mo in re.finditer(r'\(\*(DAT_[0-9a-f]+)\)\(&(DAT_[0-9a-f]+),\s*(s_[A-Za-z0-9_]+?_(10[0-9a-f]{6}))\)', body):
            api = slotmap.get(mo.group(1).lower())
            name = strings.get(int(mo.group(4), 16), mo.group(3))
            reg_order.append((api, mo.group(2), name))
        for mo in re.finditer(r'\(\*(DAT_[0-9a-f]+)\)\(&(DAT_[0-9a-f]+),\s*(acStack_\w+)\)', body):
            if slotmap.get(mo.group(1).lower()) == 'RegisterUnitType':
                reg_order.append(('RegisterUnitType', mo.group(2), picks.get(mo.group(3), mo.group(3))))

    # pick copy order (mounted pick first, shooter second - matches
    # Russia's Tiagoli/Mushketer pattern and is stable across nations,
    # unlike the Ghidra stack-buffer names)
    if not pick_order:
        pick_order = [picks[b] for b in picks]
    # only copies whose buffer is later REGISTERED are the per-game picks;
    # other copies are AssignFormUnit-style name buffers
    if buf_units:
        pick_order = [picks[b] for b in buf_units.values() if b in picks]
    facts = {'dll': dll.split('/')[-1],
             'upgrades_by_dat': sorted(set(reg_upgrades)),
             'pick_order': pick_order,
             'units': sorted(set(units)),
             'upgrades': sorted(set(n for _, n in reg_upgrades)),
             'picks': picks}

    inits, assigns = {}, {}
    init_bodies = [b for n, a, b in funcs if n == 'InitAI']
    # Austria-style: InitAI is a thin wrapper; the registration function is
    # the callee with many Register calls - include it in the scan
    for n, a, b in funcs:
        if len(re.findall(r'\(\*DAT_[0-9a-f]+\)\(&', b)) >= 40 and b not in init_bodies:
            init_bodies.append(b)
    for body in init_bodies:
        for slot, args in re.findall(r'\(\*(DAT_[0-9a-f]+)\)\(([^()]*)\);', body):
            api = slotmap.get(slot.lower())
            if api and api.startswith('SET_'):
                try:
                    inits[api] = int(args, 0)
                except ValueError:
                    pass
            elif api and api.startswith('Assign'):
                mo = re.search(SYM, args)
                if mo:
                    assigns[api] = strings.get(int(mo.group(1), 16), args)
    facts['reg_order'] = reg_order
    facts['init'] = inits
    facts['assigns'] = assigns

    def tokens_of(fname):
        for n, a, b in funcs:
            if n == fname:
                return tokenize(b, slotmap, strings)
        return None

    wrap_calls = []
    for n, a, b in funcs:
        if n == 'ProcessAI':
            for line in b.split('\n'):
                for mo in re.finditer(r'\(\*(DAT_[0-9a-f]+)\)\(', line):
                    wrap_calls.append(slotmap.get(mo.group(1).lower(), mo.group(1)))
            wrap_calls += re.findall(r'^(FUN_[0-9a-f]+)\(\);', b, re.M)
    facts['processai_calls'] = wrap_calls or None

    for n, a, b in funcs:
        toks = tokenize(b, slotmap, strings)
        if toks and toks[0][0] == 'SetDefSettings':
            facts['water_handler'] = {'name': n, 'tokens': toks}
            break
    for n, a, b in funcs:
        if '/ 0x24' not in b:
            continue
        toks = tokenize(b, slotmap, strings)
        if not toks:
            continue
        # the officers tail counts a formation unit (reader call, e.g.
        # GetUnits(&pikiner)/36) and then TryUnits officers + drummer
        tu = [t for t in toks if t[0] == 'TryUnit']
        rest = [t[0] for t in toks if t[0] != 'TryUnit']
        if len(tu) >= 2 and all(r in READERS for r in rest):
            facts['officers'] = {'name': n, 'tokens': toks}
            break
    chains = []
    for n, a, b in funcs:
        toks = tokenize(b, slotmap, strings)
        if toks and all(t[0] == 'TryUpgrade' for t in toks) and len(toks) <= 15:
            chains.append({'name': n, 'tokens': toks})
    facts['chains'] = chains

    print(json.dumps(facts, indent=1))


if __name__ == '__main__':
    main()
