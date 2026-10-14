import json, sys, subprocess, time
def detail(code):
    out=subprocess.run(["curl","-s","--max-time","25",f"https://cart.jlcpcb.com/shoppingCart/smtGood/getComponentDetail?componentCode={code}"],capture_output=True,text=True).stdout
    try: d=json.loads(out)['data']
    except Exception as e: return None
    return d
def line(d):
    if not d: return "NO DATA"
    p=d.get('prices') or []
    p1=p[0]['productPrice'] if p else None
    pref=[k for k in d if 'prefer' in k.lower()]
    extra={k:d[k] for k in pref}
    return f"{d['componentCode']:>10} {d['componentLibraryType']:6} stk={d['stockCount']:>8} ${p1} {d['assemblyProcess']}/{d['assemblyMode']} min={d['leastNumber']} loss={d['lossNumber']} status={d['componentStatus']} {d['componentModelEn'][:30]} | {d['componentBrandEn'][:22]} | {d['componentSpecificationEn'][:18]} {extra}"
if __name__=="__main__":
    for c in sys.argv[1:]:
        print(line(detail(c)))
