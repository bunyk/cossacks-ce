#!/usr/bin/env python3
# check_mission_args - extract every mission-DLL call's ARGUMENTS from the
# raw objdump disassembly, so ported modules can be verified against the
# bytes, not against Ghidra's (occasionally lying) decompilation.
#
# WHY THIS EXISTS (Single1, 2026-11-01): Ghidra misattributes hoisted
# `push` instructions to the wrong call when they sit around a branch -
# it printed zone Z4A for a galley spawn whose actual bytes push Z6, and
# Z6 where the bytes push Z7. The call NAME sequence stayed correct, so
# tools/check_mission.py could not catch it; only the raw argument values
# differ. The AI handbook warned about hoisted pushes for TryUnit args;
# the same applies to mission spawns.
#
# Usage: check_mission_args.py <mission.dll> [addr-lo addr-hi]
#   Dumps each call to the known thunks/import slots with the constants
#   pushed for it. The zone/unit/group slot addresses map to names via
#   the OnInit Register* bindings (see docs/missions.md).
import re, subprocess, sys

THUNKS = {0x10001046:'Spawn(dst,unit,zone,nat)',0x10001005:'SendKill2(dst,zone,dir)',
          0x1000100a:'SendKill200_0(dst,zone)',0x10001014:'GroupOwner(group)',
          0x10001019:'SendGroup(g,z,dir,type)',0x10001028:'GiveMinRes(nat)',
          0x10001032:'SendKill200_2(dst,zone)',0x10001050:'GiveBigRes(nat)'}

def main():
    dll = sys.argv[1]
    lo = int(sys.argv[2],16) if len(sys.argv)>2 else 0x10001120
    hi = int(sys.argv[3],16) if len(sys.argv)>3 else 0x10004310
    asm = subprocess.run(['objdump','-d',dll],capture_output=True,text=True).stdout
    insns=[]
    for l in asm.split('\n'):
        m = re.match(r'\s*([0-9a-f]+):\t[0-9a-f ]+\t(.*)', l)
        if m: insns.append((int(m.group(1),16), m.group(2).strip()))
    i=0
    while i < len(insns):
        addr, txt = insns[i]
        m = re.match(r'call\s+0x([0-9a-f]+)', txt)
        if m and int(m.group(1),16) in THUNKS and lo <= addr < hi:
            args=[]
            j=i-1
            while j>=0:
                pm = re.match(r'push\s+\$0x([0-9a-f]+)', insns[j][1])
                if pm: args.insert(0,int(pm.group(1),16)); j-=1; continue
                break
            print('%08x %-28s %s' % (addr, THUNKS[int(m.group(1),16)], [hex(a) for a in args]))
        i+=1

if __name__ == '__main__':
    main()
