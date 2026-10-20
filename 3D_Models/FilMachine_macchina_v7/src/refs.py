"""Sagome dei componenti acquistati del modulo tank (REF_), rilascio 7: perno inox, rondella PTFE, magneti, molla,
motoriduttore coassiale, servo, sensore Hall."""
import math
import cadquery as cq
from common import *
from tank import box, xz_prism, yz_prism, xy_prism, ycyl, zcyl, xcyl, PIL_Y0, WIN_T
import drive as D

def pin():
    """Perno inox Ø6 x 22 incollato nel boss della testata (6,5 mm), il resto nel mozzo magnetico."""
    return ycyl(0, HC, 3.0, Y_DRV + 0.5 - D.PIN_LEN, Y_DRV + 0.5)

def ptfe_washer():
    """Rondella di spinta in PTFE Ø24 x Ø13 x 1 tra la flangia del mozzo e la testata (attorno al boss)."""
    return ycyl(0, HC, 12.0, D.HUB_Y1, Y_DRV).cut(ycyl(0, HC, 6.5, D.HUB_Y1 - 0.01, Y_DRV + 0.01))

def magnets():
    """Magneti al neodimio: 6 + 6 di coppia Ø8x3 N52 (mozzo e disco), 4 indicatori Ø5x2 (mozzo)."""
    P = {}
    for k in range(D.MAG_N):
        a = math.radians(360.0 / D.MAG_N * k)
        x, z = D.MAG_R * math.cos(a), HC + D.MAG_R * math.sin(a)
        P[f"REF_magnete_8x3_mozzo_{k + 1}"] = ycyl(x, z, D.MAG_D / 2, D.HUB_Y1 - D.CAP_T - D.MAG_H, D.HUB_Y1 - D.CAP_T)
        P[f"REF_magnete_8x3_disco_{k + 1}"] = ycyl(x, z, D.MAG_D / 2, D.DISC_Y0 + D.CAP_T, D.DISC_Y0 + D.CAP_T + D.MAG_H)
    for k in range(D.IND_N):
        a = math.radians(D.IND_A0 + 360.0 / D.IND_N * k)
        P[f"REF_magnete_5x2_indicatore_{k + 1}"] = ycyl(D.IND_R * math.cos(a), HC + D.IND_R * math.sin(a), D.IND_D / 2, D.HUB_Y1 - D.CAP_T - D.IND_H, D.HUB_Y1 - D.CAP_T)
    return P

def spring():
    """Molla del perno folle (Ø6 x 22 libera), disegnata come manicotto."""
    return ycyl(0, HC, 3.0, PIL_Y0 + 1.6, -TUBE_HALF - 6.1).cut(ycyl(0, HC, 2.3, PIL_Y0 + 1.5, -TUBE_HALF - 6.0))

def motor():
    """Motoriduttore JGB37-520 coassiale con la spirale: riduttore Ø37 x 27, motore Ø33 x 31, mozzetto Ø12 x 6, albero Ø6 x 15 a D.
    Il corpo sta 7 mm sopra l'asse (albero d'uscita eccentrico)."""
    mx, mz = D.MOT_CEN
    m = ycyl(mx, mz, 18.5, D.MOT_FACE, D.MOT_FACE + 27.0).union(ycyl(mx, mz, 16.5, D.MOT_FACE + 27.0, D.MOT_FACE + 58.0))
    m = m.union(ycyl(0, HC, 6.0, D.MOT_FACE - 6.0, D.MOT_FACE)).union(ycyl(0, HC, 3.0, D.MOT_FACE - 15.0, D.MOT_FACE - 6.0))
    return m.cut(box(-3.1, 3.1, D.MOT_FACE - 15.01, D.MOT_FACE - 9.0, HC + 2.5, HC + 3.2))

def servo():
    """Micro-servo MG90S nel vano: corpo 22.8 x 12.2 x 22.5, alette, torretta e albero verso -Y."""
    yb = SRV_YTAB + 15.9
    zc = SRV_Z - 5.5
    s = box(SRV_X - 6.1, SRV_X + 6.1, yb - 22.5, yb, zc - 11.4, zc + 11.4)
    s = s.union(box(SRV_X - 6.1, SRV_X + 6.1, SRV_YTAB - 2.5, SRV_YTAB, zc - 16.2, zc + 16.2))
    s = s.union(ycyl(SRV_X, SRV_Z, 5.9, yb - 26.5, yb - 22.5)).union(ycyl(SRV_X, SRV_Z, 2.4, 3.3, yb - 26.5))
    return s

def hall():
    """Modulo KY-003: PCB 18,5 x 15 x 1,6 di piatto (piano XZ) a sinistra del disco, sensore A3144 piegato di piatto sul bordo
    +X con la faccia verso la testata (a r = 23,5 sull'orizzontale), LED e resistenza dietro, tre pin verso +Y all'altro capo."""
    L, T, W = D.HALL_BODY
    p = box(D.HALL_PCB_X0, D.HALL_PCB_X1, D.HALL_PCB[0], D.HALL_PCB[1], HC - D.HALL_PCB_W / 2, HC + D.HALL_PCB_W / 2)
    p = p.union(box(D.HALL_X - L / 2, D.HALL_X + L / 2, D.HALL_YF, D.HALL_YF + T, HC - W / 2, HC + W / 2))                     # corpo del sensore
    p = p.union(box(D.HALL_X - L / 2, D.HALL_X - L / 2 + 0.6, D.HALL_YF + T, D.HALL_PCB[0] + 0.01, HC - 1.5, HC + 1.5))      # piega dei reofori
    p = p.union(box(D.HALL_PCB_X0 + 4.0, D.HALL_PCB_X0 + 12.0, D.HALL_PCB[1], D.HALL_PCB[1] + 2.0, HC - 4.0, HC + 4.0))       # LED e resistenza
    return p.union(box(D.HALL_PCB_X0 + 1.5, D.HALL_PCB_X0 + 5.5, D.HALL_PCB[1], D.HALL_PCB[1] + 8.5, HC - 3.9, HC + 3.9))     # connettore a tre pin
