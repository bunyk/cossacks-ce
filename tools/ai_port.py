#!/usr/bin/env python3
# ai_port - generate a nation AI module from the Bavaria template, using
# positional alignment of the decompiled handlers (see docs/AI.md).
#
# The European 94208-byte DLLs share one code skeleton; only the registered
# names and a few constants differ. This script:
#   1. runs tools/ai_facts.py on the nation's decomp dump (and Bavaria's),
#   2. aligns the water handler + upgrade chains + officers tail token by
#      token - identical API/constant sequences give a positional DAT_
#      equivalence, hence a name map Bavaria-name -> nation-name,
#   3. completes the map for names not referenced by the handlers by
#      base-name matching (strip the "(xx)" suffix), and pairs the one
#      leftover on each side as the per-game 18th c. shooter pick,
#   4. verifies InitAI constants + tables, reports every difference,
#   5. copies src/ai/Bavaria to src/ai/<Nation> and applies everything,
#      then validates (strings vs DLL, names vs NDS, reverse coverage).
#
# Usage: ai_port.py <Nation> <decomp_dir> <dll> [--apply]
import json
import os
import re
import shutil
import subprocess
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))
BAV_DIR = '/tmp/bavaria_decomp'
BAV_DLL = '/home/tbunyk/.wine/drive_c/GOG Games/Cossacks - Back To War/AI/Bavaria.dll'
REPO = os.path.dirname(TOOLS)

NATION_INFO = {
    'England':     ('England.dll',     'ENGLAND.NDS'),
    'France':      ('France.dll',      'FRANCE.NDS'),
    'Prussia':     ('German.dll',      'GERMAN.NDS'),
    'Holland':     ('Holland.dll',     'HOLLAND.NDS'),
    'Piemont':     ('Piemont.dll',     'PIEMONT.NDS'),
    'Poland':      ('Poland.dll',      'POLAND.NDS'),
    'Portugal':    ('Portugalia.dll',  'PORTUGALIA.NDS'),
    'Saxony':      ('Saksinia.dll',    'SAKSINIA.NDS'),
    'Spain':       ('Spain.dll',       'SPAIN.NDS'),
    'Sweden':      ('Sveden.dll',      'SVEDEN.NDS'),
    'Venice':      ('Venecia.dll',     'VENECIA.NDS'),
    'Hungary':     ('Vengria.dll',     'VENGRIA.NDS'),
    'Switzerland': ('Switzerland.dll', 'SWITZERLAND.NDS'),
    # 49152-byte DLL, but the AI code is the same European skeleton
    # (all handlers token-identical to Bavaria; smaller CRT)
    'Denmark':     ('Denmark.dll',     'DENMARK.NDS'),
}


def facts(decomp_dir, dll):
    out = subprocess.run([sys.executable, os.path.join(TOOLS, 'ai_facts.py'), decomp_dir, dll],
                         capture_output=True, text=True, check=True, encoding='latin1').stdout
    return json.loads(out)


def align_tokens(bav, nat):
    dat_map, diffs = {}, []
    if len(bav) != len(nat):
        return False, dat_map, ['token count %d vs %d' % (len(bav), len(nat))]
    for i, (bt, nt) in enumerate(zip(bav, nat)):
        if bt[0] != nt[0] or bt[1] != nt[1]:
            diffs.append('token %d: %s %s vs %s %s' % (i, bt[0], bt[1], nt[0], nt[1]))
            continue
        for b, n in zip(bt[2], nt[2]):
            if b in dat_map and dat_map[b] != n:
                diffs.append('token %d: %s maps to %s and %s' % (i, b, dat_map[b], n))
            dat_map[b] = n
    return not diffs, dat_map, diffs


def base(name):
    # strip every "(xx)" nation-suffix group, anywhere in the name
    # (Pikiner_evro(BA), Mushketer_ev(BA)ATTACK, shahta(BA)INSIDE7),
    # then the paren-less upgrade-id suffix (AKA29BA -> AKA29)
    m = re.sub(r'\([a-zA-Z]{2}\)', '', name)
    return re.sub(r'^(AKA\d+|KUZ\d+|MAIN)[a-zA-Z]{2}$', r'\1', m)


def build_map(bav, nat):
    """name map Bavaria-string -> nation-string + report lines."""
    report = []
    dat_map = {}
    ok, dm, diffs = align_tokens(bav['water_handler']['tokens'],
                                 nat['water_handler']['tokens'])
    report.append('water handler: %s' % ('IDENTICAL' if ok else 'DIFFERS'))
    report += ['  ' + d for d in diffs]
    dat_map.update(dm)

    def sig(tokens):
        return [t[1] for t in tokens]
    for bc in bav['chains']:
        match = [c for c in nat['chains'] if sig(c['tokens']) == sig(bc['tokens'])]
        if not match:
            report.append('chain %s: NO signature match' % bc['name'])
            continue
        ok2, m2, d2 = align_tokens(bc['tokens'], match[0]['tokens'])
        report.append('chain %s -> %s: %s' % (bc['name'], match[0]['name'],
                                               'IDENTICAL' if ok2 else 'DIFFERS'))
        report += ['  ' + x for x in d2]
        dat_map.update(m2)
    ok3, m3, d3 = align_tokens(bav['officers']['tokens'], nat['officers']['tokens'])
    report.append('officers: %s' % ('IDENTICAL' if ok3 else 'DIFFERS'))
    report += ['  ' + x for x in d3]
    dat_map.update(m3)

    bav_by_dat = {d: n for d, n in bav['units']}
    # upgrades share the DAT namespace with units; needed when the nation
    # repoints an upgrade slot (Hungary: its MAIN-slot DAT is registered
    # as Melnica(VG)GETRES - the original never researches the 18th c.)
    bav_by_dat.update({d: n for d, n in bav.get('upgrades_by_dat', [])})
    nat_by_dat = {d: n for d, n in nat['units']}
    nat_by_dat.update({d: n for d, n in nat.get('upgrades_by_dat', [])})
    name_map = {}
    for bd, nd in dat_map.items():
        bn, nn = bav_by_dat.get(bd), nat_by_dat.get(nd)
        if bn and nn:
            name_map[bn] = nn

    # base-name matching for everything registered but not handler-aligned
    bav_all = {n for _, n in bav['units']} | set(bav['upgrades'])
    nat_all = {n for _, n in nat['units']} | set(nat['upgrades'])
    for k, v in bav['assigns'].items():
        nv = nat['assigns'].get(k)
        if nv and nv != v:
            name_map[v] = nv

    bav_by_base, nat_by_base = {}, {}
    for n in bav_all:
        bav_by_base.setdefault(base(n), []).append(n)
    for n in nat_all:
        nat_by_base.setdefault(base(n), []).append(n)
    for b, bs in bav_by_base.items():
        ns = nat_by_base.get(b, [])
        if len(bs) == 1 and len(ns) == 1:
            name_map[bs[0]] = ns[0]
        elif bs and ns:
            report.append('AMBIGUOUS base %r: %s vs %s' % (b, bs, ns))

    # the per-game picks: pair by VALUE, not by Ghidra buffer name (the
    # stack offsets are not stable across nations). The mounted pick
    # (Konni_Ricar) usually pairs by base already; the other one is the
    # 18th c. shooter slot (Russia: Mushketer-or-Grenader). A pick whose
    # role the nation genuinely lacks stays unmapped here and is caught
    # by the dropped-roles pass below (Hungary: its shooter is only ever
    # fed to dead SafeRegisterUpgrade slots, never a unit registration).
    for i, bp in enumerate(bav.get('pick_order', [])):
        if bp in name_map:
            continue
        cands = nat.get('pick_order', [])
        if i < len(cands) and cands[i] not in name_map.values():
            name_map[bp] = cands[i]
            report.append('pick pair (order %d): %s -> %s' % (i, bp, cands[i]))
        else:
            report.append('AMBIGUOUS pick %s, candidates %s' % (bp, cands))

    # unit renames propagate to their upgrade chains (e.g. Bavaria
    # Mushketer_ev(BA)ATTACK -> Mushketer_niderland(HO)ATTACK)
    propagated = set()
    for old_unit, new_unit in list(name_map.items()):
        if old_unit not in bav_all:
            continue
        for n2 in bav_all:
            if n2 != old_unit and n2.startswith(old_unit):
                name_map[n2] = new_unit + n2[len(old_unit):]
                propagated.add(n2)
    # propagation can invent names the nation never registers (England
    # has no Bashnia(en)PAUSE5 even though it has the other PAUSE steps);
    # those are dropped roles, not renames. Only propagated entries are
    # checked - direct mappings (assigns, handler alignment, picks) may
    # reference names that are only Assigned, never registered.
    for k in sorted(k for k in propagated if name_map[k] not in nat_all):
        del name_map[k]
        report.append('unregistered chain tail dropped: %s' % k)

    # cluster pairing for renamed init-only units: group the remaining
    # unmapped names by their chain-suffix set and pair equal shapes
    CHAIN = re.compile(r'(ATTACK[3-7]?|SHIELD[3-7]?|PAUSE[3-7]?|BUILD[3-6]?|GETRES2?|INSIDE[3-7]?)$')
    def cluster(name):
        m = CHAIN.search(name)
        return (name[:m.start()] if m else name, m.group(0) if m else '')
    rem_bav = [n for n in bav_all if n not in name_map]
    rem_nat = [n for n in nat_all if n not in name_map.values()]
    bav_cl, nat_cl = {}, {}
    for n in rem_bav:
        bav_cl.setdefault(cluster(n)[0], {})[cluster(n)[1]] = n
    for n in rem_nat:
        nat_cl.setdefault(cluster(n)[0], {})[cluster(n)[1]] = n
    for cb, members in list(bav_cl.items()):
        # exact-base cluster first
        if cb in nat_cl and set(nat_cl[cb]) == set(members):
            for suf, bn in members.items():
                name_map[bn] = nat_cl[cb][suf]
            report.append('cluster (same base) %s' % cb)
            continue
        cands = [cn for cn, mem in nat_cl.items()
                 if set(mem) == set(members) and cn not in bav_cl]
        if len(cands) == 1:
            for suf, bn in members.items():
                name_map[bn] = nat_cl[cands[0]][suf]
            report.append('cluster (renamed) %s -> %s' % (cb, cands[0]))
        elif cands:
            report.append('AMBIGUOUS cluster %s: candidates %s' % (cb, cands))

    # Bavaria roles whose base has NO nation counterpart = dropped roles
    # (after every mapping stage; drop_roles deletes their registrations
    # and neutralizes their uses)
    dropped = sorted(n for n in bav_all
                     if n not in name_map and base(n) not in nat_by_base)
    if dropped:
        report.append('DROPPED roles (nation registers no counterpart): %s' % dropped)

    # pair the one leftover on each side = the per-game 18th c. shooter pick
    lm_bav = sorted(n for n in bav_all if n not in name_map and n not in dropped)
    lm_nat = sorted(n for n in nat_all if n not in name_map.values())
    if len(lm_bav) == 1 and len(lm_nat) == 1:
        name_map[lm_bav[0]] = lm_nat[0]
        report.append('pick2 pairing: %s -> %s' % (lm_bav[0], lm_nat[0]))
    else:
        report.append('LEFTOVERS bav=%s nat=%s (need manual pairing)' % (lm_bav, lm_nat))

    for k in sorted(set(bav['init']) | set(nat['init'])):
        if bav['init'].get(k) != nat['init'].get(k):
            report.append('INIT %s: Bavaria %s vs %s' % (k, bav['init'].get(k), nat['init'].get(k)))
    for k in sorted(set(bav['assigns']) | set(nat['assigns'])):
        if bav['assigns'].get(k) != nat['assigns'].get(k):
            report.append('ASSIGN %s: %s -> %s' % (k, bav['assigns'].get(k), nat['assigns'].get(k)))
    extras = sorted(n for n in nat_all if n not in name_map.values())
    if extras:
        report.append('nation-only names (register as extras): %s' % extras)
    return name_map, report, dropped


def extract_nds(nation):
    dll, nds = NATION_INFO[nation]
    out = subprocess.run([sys.executable, os.path.join(TOOLS, 'gsc_extract.py'),
                          'gamedata/all.gsc', nds], capture_output=True, text=True, encoding='latin1')
    if out.returncode != 0:
        return ''
    return out.stdout


def drop_roles(txt, dropped, report):
    """Remove registration and use lines for roles the nation lacks.
    TryUnit/TryUpgrade lines are deleted; reader/gate calls that would
    pop the engine's "invalid unit ID" dialog on the unbound GAMEOBJ
    (GetUnits/UpgIsDone/UpgIsRun - see activescenary.cpp) are replaced
    with the constant a dead slot yields: 0 / false."""
    for name in dropped:
        mo = re.search(r'Register\w+\(&(\w+), "%s"\);\n' % re.escape(name), txt)
        if not mo:
            report.append('MANUAL: cannot find registration of %s' % name)
            continue
        var = mo.group(1)
        txt = txt.replace(mo.group(0), '')
        n_uses = 0
        for line in txt.split('\n'):
            if ('&%s' % var) in line and ('TryUnit(&%s' % var in line or
                                         'TryUpgrade(&%s' % var in line):
                txt = txt.replace(line + '\n', '')
                n_uses += 1
        # neutralize reader/gate references: dead slot == 0 units / not done
        for call, repl in (('GetUnits(&%s)' % var, '0'),
                           ('UpgIsDone(&%s)' % var, 'false'),
                           ('UpgIsRun(&%s)' % var, 'false')):
            if call in txt:
                txt = txt.replace(call, repl)
        # any remaining references need manual care
        gates = re.findall(r'&%s\b' % var, txt)
        if gates:
            report.append('MANUAL: %s (%s) still referenced (%d spots)' % (var, name, len(gates)))
    return txt


def apply_module(nation, dll, name_map, nat, report, dropped=()):
    dll_name, nds_name = NATION_INFO[nation]
    dst = os.path.join(REPO, 'src/ai', nation)
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(os.path.join(REPO, 'src/ai/Bavaria'), dst)

    src_path = os.path.join(dst, 'dllmain.cpp')
    txt = open(src_path).read()

    # whole-quoted-string replacements (registrations, assigns, comments)
    for old, new in sorted(name_map.items(), key=lambda kv: -len(kv[0])):
        txt = txt.replace('"%s"' % old, '"%s"' % new)

    # roles the nation does not register: drop lines + uses
    txt = drop_roles(txt, dropped, report)

    # unquoted occurrences of the old names inside comments: plain replace
    # is safe (every entry swaps the same Bavaria suffix for the nation's)
    for old, new in sorted(name_map.items(), key=lambda kv: -len(kv[0])):
        if old in txt:
            txt = txt.replace(old, new)

    # header comment block: regenerate with the nation's data
    i = txt.index('// Registers Bavaria\'s units')
    j = txt.index('void RegisterBavaria()')
    suffix = base(next(iter(name_map.values())))  # not used directly
    picks = nat.get('picks', {})
    pick_txt = ', '.join(sorted(picks.values())) if picks else 'none'
    header = """// Registers %(nation)s's units/upgrades with the game. Names must match the
// per-nation script %(nds)s (listed in Nations.lst, packed in all.gsc).
// All strings below are byte-exact from the original %(dll)s string
// table (Ghidra + raw-string dump; see tools/ai_port.py which generated
// this module from the Bavaria template by positional handler alignment).
// Role notes for this nation:
//   - 17th c. musketeer: %(musket)s; 18th c. shooter pick: %(pick)s
//   - town hall: %(center)s; house: %(house)s; peasant: %(peasant)s
//   - 18th c. barracks %(kazarma)s is MAIN-gated (ENABLE UNITS GRP 4)
// The water-map AI and routing are 1:1 with the decompilation; the land
// pipeline and the Mediterranean handler keep the Turkey-lineage
// fallbacks (see docs/AI.md "Known gaps").
""" % {'nation': nation, 'nds': nds_name, 'dll': dll_name,
       'musket': name_map.get('Mushketer_ev(BA)', '?'),
       'pick': pick_txt,
       'center': name_map.get('Europ(BA)', '?'),
       'house': nat['assigns'].get('AssignHouse', '?'),
       'peasant': nat['assigns'].get('AssignPeasant', '?'),
       'kazarma': name_map.get('Kazarma(BA)', '?')}
    txt = txt[:i] + header + txt[j:]

    # function + comment renames
    txt = txt.replace('void RegisterBavaria()', 'void Register%s()' % nation)
    txt = txt.replace('\tRegisterBavaria();', '\tRegister%s();' % nation)
    txt = txt.replace('Bavaria.dll', dll_name)
    txt = txt.replace('BAVARIA.NDS', nds_name)
    txt = re.sub(r'Original Bavaria\.dll FUN_[0-9a-f]+', 'Original %s' % dll_name, txt)
    txt = txt.replace('Bavaria', nation)  # remaining comment mentions
    txt = txt.replace('bavaria', nation.lower())

    # InitAI constants per nation facts
    init = nat['init']
    txt = txt.replace('\tSET_MINE_CAPTURE_RADIUS(290);',
                      '\tSET_MINE_CAPTURE_RADIUS(%d);' % init.get('SET_MINE_CAPTURE_RADIUS', 290))
    txt = txt.replace('\tSET_MINE_UPGRADE1_RADIUS(90);',
                      '\tSET_MINE_UPGRADE1_RADIUS(%d);' % init.get('SET_MINE_UPGRADE1_RADIUS', 90))
    txt = txt.replace('\tSET_MINE_UPGRADE2_RADIUS(90);',
                      '\tSET_MINE_UPGRADE2_RADIUS(%d);' % init.get('SET_MINE_UPGRADE2_RADIUS', 90))
    txt = txt.replace('\tSET_DEFAULT_MAX_WORKERS(300);',
                      '\tSET_DEFAULT_MAX_WORKERS(%d);' % init.get('SET_DEFAULT_MAX_WORKERS', 300))
    txt = txt.replace('\tSET_MIN_PEASANT_BRIGADE(5);',
                      '\tSET_MIN_PEASANT_BRIGADE(%d);' % init.get('SET_MIN_PEASANT_BRIGADE', 5))
    open(src_path, 'w').write(txt)

    # nation-only names (units AND upgrades): append registrations +
    # objects.h vars (Poland: the whole Grenader(PL)ATTACK/SHIELD chain
    # exists only on its side)
    extras = sorted(n for n in set([n for _, n in nat['units']] +
                                    [n for _, n in nat.get('upgrades_by_dat', [])])
                    if n not in name_map.values() and n not in name_map)
    def is_upgrade(n):
        return bool(re.search(r'(ATTACK[3-7]?|SHIELD[3-7]?|PAUSE[3-7]?|BUILD[3-6]?|GETRES2?|INSIDE[3-7]?)$', n)) \
            or re.match(r'^(AKA\d+|KUZ\d+|MAIN)[a-zA-Z]{2}$', n)
    if extras:
        objs_path = os.path.join(dst, 'objects.h')
        objs = open(objs_path).read()
        reg_lines, obj_lines = [], []
        for u in extras:
            var = re.sub(r'[^A-Za-z0-9_]', '_', re.sub(r'\([a-zA-Z]{2}\)', '', u))
            obj_lines.append('GAMEOBJ %s;' % var)
            if is_upgrade(u):
                reg_lines.append('\tRegisterUpgrade(&%s, "%s");' % (var, u))
            else:
                reg_lines.append('\tRegisterUnitType(&%s, "%s");' % (var, u))
        objs += ('\n// Nation-only units registered by the original %s (no handler use)\n'
                 % dll_name) + '\n'.join(obj_lines) + '\n'
        open(objs_path, 'w').write(objs)
        txt = open(src_path).read()
        txt = txt.replace('\tRegisterUpgrade(&KUZ04,', '\n'.join(reg_lines) +
                          '\n\tRegisterUpgrade(&KUZ04,')
        open(src_path, 'w').write(txt)
        report.append('extras registered: %s' % extras)

    report.append('module generated: src/ai/%s' % nation)
    return dst


def validate(nation, dll, dst, name_map, nat):
    report = []
    src_path = os.path.join(dst, 'dllmain.cpp')
    txt = open(src_path).read()
    dll_strings = set(subprocess.run(
        [sys.executable, os.path.join(TOOLS, 'pe_dump.py'), dll, 'strings'],
        capture_output=True, text=True, check=True, encoding='latin1').stdout.splitlines())
    dll_names = {s.split(' ', 1)[1][1:-1] for s in dll_strings}
    quoted = set(re.findall(r'"([A-Za-z0-9_.()]+)"', txt))
    miss = sorted(q for q in quoted if q not in dll_names and not q.startswith('tp'))
    report.append('strings not in DLL: %s' % ([m for m in miss if not re.match(r'^(objects|typedefs|default|C|-)$', m)],))

    nds = extract_nds(nation)
    names = re.findall(r'RegisterUnitType\(&\w+,\s*"([^"]+)"\)|RegisterUpgrade\(&\w+,\s*"([^"]+)"\)|Assign(?:Peasant|Mine|House|Wall|FormUnit|MineUpgrade)\((?:\d+,\s*)?"([^"]+)"', txt)
    names = sorted(set(a or b or c for a, b, c in names))
    nds_missing = [n for n in names if n not in nds]
    report.append('registered names NOT in %s: %s' % (NATION_INFO[nation][1], nds_missing))

    reverse = [n for _, n in nat['units'] if n not in set(names)]
    report.append('DLL units not registered by module: %s' % reverse)
    return report


def main():
    nation, decomp_dir, dll = sys.argv[1], sys.argv[2], sys.argv[3]
    apply = '--apply' in sys.argv
    bav = facts(BAV_DIR, BAV_DLL)
    nat = facts(decomp_dir, dll)
    name_map, report, dropped = build_map(bav, nat)
    n_report = len(report)
    print('\n'.join(['=== %s ===' % nation] + report))
    if apply:
        dst = apply_module(nation, dll, name_map, nat, report, dropped)
        # apply_module/drop_roles append late findings (MANUAL fixes,
        # extras) after the report above was printed - show them too
        if len(report) > n_report:
            print('\n'.join(report[n_report:]))
        print('\n'.join(validate(nation, dll, dst, name_map, nat)))
    json.dump({'name_map': name_map, 'report': report, 'dropped': dropped},
              open('/tmp/%s_map.json' % nation.lower(), 'w'), indent=1)


if __name__ == '__main__':
    main()
