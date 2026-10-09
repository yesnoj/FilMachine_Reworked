"""Sagome dei componenti acquistati (REF_): servono per gli assiemi e per la nuova distinta."""
import math
import cadquery as cq
from common import *
from tank import box, xz_prism, yz_prism, xy_prism, ycyl, zcyl, xcyl, PAD_Y1, PIL_Y0
import drive as D

def shaft():
    return ycyl(0, HC, SHAFT_D / 2, D.DOG_Y0, D.SHAFT_Y1)

def bearing(i):
    y0, y1 = D.BRG_Y[i]
    return ycyl(0, HC, 8.0, y0, y1).cut(ycyl(0, HC, 4.0, y0 - 0.01, y1 + 0.01))

def seal():
    y0, y1 = D.SEAL_Y
    return ycyl(0, HC, 8.0, y0, y1).cut(ycyl(0, HC, 4.0, y0 - 0.01, y1 + 0.01))

def washer():
    return ycyl(0, HC, 8.0, D.WASHER_Y0, D.CART_IN).cut(ycyl(0, HC, 4.2, D.WASHER_Y0 - 0.01, D.CART_IN + 0.01))

def spring():
    """Molla del perno folle (Ø6 x 22 libera), disegnata come manicotto."""
    return ycyl(0, HC, 3.0, PIL_Y0 + 1.6, -TUBE_HALF - 6.1).cut(ycyl(0, HC, 2.3, PIL_Y0 + 1.5, -TUBE_HALF - 6.0))

def motor(pos=None, ang=None):
    """Motoriduttore JGB37-520: riduttore Ø37 x 27, motore Ø33 x 31, mozzetto Ø12 x 6, albero Ø6 x 15 a 7 mm dal centro."""
    a = math.radians(D.ECC_ANG if ang is None else ang)
    sx, sz = MOT_X + D.MOT_ECC * math.cos(a), MOT_Z + D.MOT_ECC * math.sin(a)
    m = ycyl(MOT_X, MOT_Z, 18.5, D.MOT_FACE - 27.0, D.MOT_FACE)
    m = m.union(ycyl(MOT_X, MOT_Z, 16.5, D.MOT_FACE - 58.0, D.MOT_FACE - 27.0))
    m = m.union(ycyl(sx, sz, 6.0, D.MOT_FACE, D.MOT_FACE + 6.0)).union(ycyl(sx, sz, 3.0, D.MOT_FACE + 6.0, D.MOT_FACE + 21.0))
    return m.cut(box(sx - 3.1, sx + 3.1, D.MOT_FACE + 9.0, D.MOT_FACE + 21.01, sz + 2.5, sz + 3.2))      # spianatura a D

def servo():
    """Micro-servo MG90S nel vano: corpo 22.8 x 12.2 x 22.5, alette, torretta e albero verso -Y."""
    yb = SRV_YTAB + 15.9                                     # base del servo
    zc = SRV_Z - 5.5                                         # centro del corpo
    s = box(SRV_X - 6.1, SRV_X + 6.1, yb - 22.5, yb, zc - 11.4, zc + 11.4)
    s = s.union(box(SRV_X - 6.1, SRV_X + 6.1, SRV_YTAB - 2.5, SRV_YTAB, zc - 16.2, zc + 16.2))      # alette
    s = s.union(ycyl(SRV_X, SRV_Z, 5.9, yb - 26.5, yb - 22.5)).union(ycyl(SRV_X, SRV_Z, 2.4, 3.3, yb - 26.5))
    return s

def belt(pos=None):
    """Cinghia tonda Ø3 (O-ring 80x3) tesa tra le due pulegge, disegnata a sezione quadra 3x3."""
    x1, z1 = pos or D.MOT_SH
    x2, z2 = 0.0, HC
    ang = math.degrees(math.atan2(z2 - z1, x2 - x1)); L = math.hypot(x2 - x1, z2 - z1)
    def stadium(rr):
        s = ycyl(0, 0, rr, D.PUL_GC - 1.5, D.PUL_GC + 1.5).union(ycyl(L, 0, rr, D.PUL_GC - 1.5, D.PUL_GC + 1.5))
        return s.union(box(0, L, D.PUL_GC - 1.5, D.PUL_GC + 1.5, -rr, rr))
    b = stadium(PUL_R + 1.5).cut(stadium(PUL_R - 1.5))
    return b.rotate((0, 0, 0), (0, 1, 0), -ang).translate((x1, 0, z1))

def magnets():
    m = None
    for k in range(4):
        a = math.radians(45 + 90 * k)
        c = ycyl(D.MAG_R * math.cos(a), HC + D.MAG_R * math.sin(a), D.MAG_D / 2, D.WHEEL_Y1 - D.MAG_H, D.WHEEL_Y1)
        m = c if m is None else m.union(c)
    return m

def hall():
    """Modulo KY-003 (scheda 18.5 x 15 x 1.6) con il sensore A3144 sulla pista dei magneti."""
    yf = D.HALL_YF
    h = box(-9.25, 9.25, yf - 1.6, yf, HC + 8.2, HC + 23.2)
    return h.union(box(4.0, 8.0, yf - 4.0, yf - 1.6, HC + 10.0, HC + 13.0))
