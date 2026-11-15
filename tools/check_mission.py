import re, sys
API = set()
for mo in re.finditer(r'extern "C" [^;]*?(\w+)\(', open('src/missions/common/mission_api.h').read()):
    API.add(mo.group(1))
MAP = {'GiveMinRes':'thunk_FUN_10001120','GiveBigRes':'thunk_FUN_10001260',
 'GroupOwner':'thunk_FUN_100012d0','SendKill2':'thunk_FUN_100014a0',
 'SendKill200_2':'thunk_FUN_10001450','SendKill200_0':'thunk_FUN_100014f0',
 'SpawnInto':'thunk_FUN_100016f0'}
CALL = re.compile(r'([A-Za-z_]\w*)\s*\(')

def strip_comments(s):
    s = re.sub(r'/\*.*?\*/', '', s, flags=re.S)
    s = re.sub(r'//[^\n]*', '', s)
    return s

def seq_decomp(path, fname):
    txt = open(path).read()
    m = re.search(r'// ===== %s @ .*? =====\n(.*?)(?=\n\n\n\n// ===== |\Z)' % fname, txt, re.S)
    body = re.sub(r'__chkesp\(\);', '', m.group(1))
    return [w for w in re.findall(r'\b(thunk_FUN_[0-9a-f]+|[A-Z]\w*)\s*\(', body)
            if w in API or w.startswith('thunk_FUN_')]

def exec_order_calls(body):
    """Walk statements left-to-right; inside a call, arguments first."""
    body = strip_comments(body)
    # pre-compute call spans
    st, pair = [], {}
    for i, c in enumerate(body):
        if c == '(': st.append(i)
        elif c == ')' and st: pair[st.pop()] = i
    out = []
    def walk(a, b):
        i = a
        while i < b:
            m = CALL.match(body, i)
            if not m:
                i += 1
                continue
            name = m.group(1)
            p = m.end() - 1
            if p in pair:
                # recurse into the argument region first
                walk(m.end(), pair[p])
                if name in MAP: out.append(MAP[name])
                elif name in API: out.append(name)
                i = pair[p] + 1
            else:
                i = m.end()
    walk(0, len(body))
    return out

def seq_src(path, fname):
    txt = open(path).read()
    m = re.search(r'extern "C" __declspec\(dllexport\) void %s\(\)\n\{(.*?)\n\}\n' % fname, txt, re.S)
    return exec_order_calls(m.group(1))

dmp, src = sys.argv[1], sys.argv[2]
for f in ('OnInit','ProcessScenary'):
    a, b = seq_decomp(dmp, f), seq_src(src, f)
    if a == b:
        print('%s: IDENTICAL (%d calls)' % (f, len(a)))
    else:
        print('%s: DIFFERS decomp=%d src=%d' % (f, len(a), len(b)))
        first = next((i for i,(x,y) in enumerate(zip(a,b)) if x!=y), min(len(a),len(b)))
        for i in range(max(0,first-4), min(len(a),first+8)):
            print('   decomp[%d]=%-24s src=%s' % (i, a[i], b[i] if i<len(b) else '<none>'))
