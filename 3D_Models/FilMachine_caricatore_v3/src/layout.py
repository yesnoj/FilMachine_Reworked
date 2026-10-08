"""Posizionamento dei pezzi nel sistema di riferimento del banco."""
import math
import cadquery as cq
from common import *
import reel, guide

def place_reel(fmt, psi=0.0):
    """Spirale montata: asse lungo Y, centro in (0,0,HC). psi = rotazione oraria (vista da -Y).
    Restituisce [flangia A, tubo, tamburo, flangia B]."""
    out = []
    for p in reel.assembled(fmt):
        p = p.rotate((0, 0, 0), (0, 0, 1), -psi)
        out.append(p.rotate((0, 0, 0), (1, 0, 0), 90).translate((0, 0, HC)))
    return out

def arm_angle(r):
    """Angolo del braccio (gradi, + = punta in alto) con la suola appoggiata sul raggio r."""
    px, pz = PIV_X, PIV_DZ
    def gap(phi):
        c, s_ = math.cos(phi), math.sin(phi)
        best = 1e9
        for i in range(0, 361):
            s = ARM_TAPER1 + (ARM_L - 3 - ARM_TAPER1) * i / 360
            x = px + s * c + H_EDGE * s_; z = pz + s * s_ - H_EDGE * c
            best = min(best, math.hypot(x, z))
        return best - r
    lo, hi = math.radians(-60), math.radians(40)
    for _ in range(50):
        mid = (lo + hi) / 2
        if gap(mid) > 0: hi = mid
        else: lo = mid
    return math.degrees((lo + hi) / 2)

def place_arm(fmt, r):
    a, d = guide.arm(fmt)
    phi = arm_angle(r)
    a = a.translate((0, 0, -H_EDGE)).rotate((0, 0, 0), (0, 1, 0), -phi).translate((PIV_X, 0, HC + PIV_DZ))
    return a, phi

if __name__ == "__main__":
    for fmt in ("135", "120"):
        parts = place_reel(fmt)
        for r in (SOLE_STOP_R, 30.0, 42.0):
            a, phi = place_arm(fmt, r)
            inter = sum(a.val().intersect(p.val()).Volume() for p in parts)
            dmin = min(a.val().distance(p.val()) for p in (parts[0], parts[3]))
            print(f"{fmt} r={r:4.1f} angolo braccio={phi:6.1f} gradi  interferenza={inter:.3f} mm3  "
                  f"gioco minimo braccio/flange={dmin:.2f} mm")
    parts = place_reel("135"); a, _ = place_arm("135", 30.0)
    render(parts[:3] + [a], "assieme_braccio_135", elev=18, azim=-100)
