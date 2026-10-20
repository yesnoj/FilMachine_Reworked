"""Filetti metrici stampabili (profilo a 60 gradi con cresta e fondo spianati), per sweep elicoidale."""
import math
import cadquery as cq

def thread_solid(r_root, r_crest, pitch, length, z0=0.0, flat=0.15, steps=32):
    """Filetto (solo il risalto) tra r_root e r_crest, lungo length a partire da z0, come solido a facce piane
    (sezioni ogni 360/steps gradi lungo l'elica): le operazioni booleane restano affidabili."""
    from common import loft_planar
    half_root = pitch / 2 - 0.03
    half_crest = max(pitch * flat / 2, 0.05)
    s = 1 if r_crest > r_root else -1
    rr = r_root - s * 0.2                                         # sovrapposizione col nocciolo
    prof = [(rr, -half_root), (r_crest, -half_crest), (r_crest, half_crest), (rr, half_root)]
    n = int(math.ceil((length + pitch) / pitch * steps))
    secs = []
    for i in range(n + 1):
        a = 2 * math.pi * i / steps; dz = z0 - pitch / 2 + pitch * i / steps
        c, sn = math.cos(a), math.sin(a)
        secs.append([(r * c, r * sn, z + dz) for (r, z) in prof])
    return cq.Workplane("XY").add(loft_planar(secs))

def ext_thread(d_major, pitch, length, z0=0.0, clear=0.2):
    """Gambo filettato esterno (nocciolo + filetto), con smusso alle due estremita'."""
    rM = d_major / 2 - clear / 2
    rm = rM - 0.6134 * pitch
    core = cq.Workplane("XY").circle(rm + 0.01).extrude(length).translate((0, 0, z0))
    t = core.union(thread_solid(rm, rM, pitch, length, z0))
    env = cq.Workplane("XY").add(cq.Solid.makeCone(rm, rM + 0.6, rM + 0.6 - rm, cq.Vector(0, 0, z0)))
    env = env.union(cq.Workplane("XY").circle(rM + 0.6).extrude(length - 2 * (rM + 0.6 - rm)).translate((0, 0, z0 + rM + 0.6 - rm)))
    env = env.union(cq.Workplane("XY").add(cq.Solid.makeCone(rM + 0.6, rm, rM + 0.6 - rm, cq.Vector(0, 0, z0 + length - (rM + 0.6 - rm)))))
    return t.intersect(env)

def int_thread_cut(d_major, pitch, length, z0=0.0, clear=0.2):
    """Volume da togliere per un foro filettato interno (foro al diametro di nocciolo + vano del filetto)."""
    rM = d_major / 2 + clear / 2 + 0.1
    rm = rM - 0.6134 * pitch
    hole = cq.Workplane("XY").circle(rm).extrude(length).translate((0, 0, z0))
    return hole.union(thread_solid(rm, rM, pitch, length, z0))

if __name__ == "__main__":
    import time; t = time.time()
    a = ext_thread(16.0, 2.0, 12.0)
    print("est", a.val().isValid(), len(a.val().Solids()), round(a.val().Volume()), round(time.time() - t, 1))
    n = cq.Workplane("XY").polygon(6, 27.7).extrude(8).cut(int_thread_cut(16.0, 2.0, 8.0, -0.01).union(cq.Workplane("XY").circle(1).extrude(1)))
    print("int", n.val().isValid(), len(n.val().Solids()), round(n.val().Volume()), round(time.time() - t, 1))
    i = a.intersect(n.translate((0, 0, 2)))
    print("interferenza vite/dado", round(i.val().Volume(), 2) if i.vals() else 0)
