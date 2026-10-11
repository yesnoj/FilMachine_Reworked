import sexpdata,sys
sys.path.insert(0,'/home/claude/pcb'); import design
S=lambda x: x.value() if isinstance(x,sexpdata.Symbol) else x
d=sexpdata.loads(open(sys.argv[1]).read())
got={}
comps=0
for e in d:
    if isinstance(e,list) and S(e[0])=='components': comps=len(e)-1
    if isinstance(e,list) and S(e[0])=='nets':
        for n in e[1:]:
            name=[x for x in n if isinstance(x,list) and S(x[0])=='name'][0][1].lstrip('/')
            nodes=set((x[1][1],x[2][1]) for x in n if isinstance(x,list) and S(x[0])=='node')
            got[name]=nodes
exp={k:set(v) for k,v in design.nets().items()}
print('components in netlist',comps,'design',len(design.PARTS))
bad=0
for k in sorted(set(exp)|set(got)):
    if k.startswith('unconnected-') : continue
    if exp.get(k)!=got.get(k):
        bad+=1; print('MISMATCH',k,'exp-got',exp.get(k,set())-got.get(k,set()),'got-exp',got.get(k,set())-exp.get(k,set()))
unc=[k for k in got if k.startswith('unconnected-')]
print('mismatches',bad,'unconnected nets in sch',len(unc), unc[:10])
