import re, sys
def pins(lib, name):
    s=open(f'/usr/share/kicad/symbols/{lib}.kicad_sym').read()
    i=s.index(f'(symbol "{name}"')
    j=i+20
    while True:
        k=s.find('(symbol "', j)
        if k<0: k=len(s); break
        nm=re.match(r'\(symbol "([^"]+)"', s[k:]).group(1)
        if not nm.startswith(name+"_"): break
        j=k+10
    blk=s[i:k]
    ext=re.findall(r'\(extends "([^"]+)"',blk)
    out=re.findall(r'\(pin (\w+) \w+\s*\(at [^)]*\)\s*\(length [^)]*\)\s*(?:\(hide yes\)\s*)?\(name "([^"]*)"\s*\(effects.*?\)\s*\)\s*\(number "([^"]*)"',blk,re.S)
    return ext, sorted(out, key=lambda t:(len(t[2]),t[2]))
if __name__=="__main__":
    for a in sys.argv[1:]:
        lib,name=a.split(":")
        ext,p=pins(lib,name)
        if ext and not p: ext2,p=pins(lib,ext[0])
        print(lib,name,ext); print("   ",[(num,n,t) for t,n,num in p])
