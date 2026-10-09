"""Posizionamento dei pezzi nel riferimento della tank."""
import math
import cadquery as cq
from common import *
import reel

def to_tank(p, psi=0.0):
    """Dal riferimento della spirale (asse Z) a quello della tank (asse Y, centro in (0,0,HC)); psi = rotazione."""
    return p.rotate((0, 0, 0), (0, 0, 1), -psi).rotate((0, 0, 0), (1, 0, 0), 90).translate((0, 0, HC))

def place_reel(fmt, psi=0.0, mode="alternata"):
    """Spirale montata: [flangia A, tubo, tamburo, flangia B, anello lato A, anello lato B]. La flangia A sta a +Y."""
    return [to_tank(p, psi) for p in reel.assembled(fmt) + reel.fin_rings_assembled(fmt, mode)]
