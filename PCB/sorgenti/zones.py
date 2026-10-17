# -*- coding: utf-8 -*-
"""Mappa delle zone della Mainboard: quale parte sostituisce quale modulo (Adafruit #6318, DBH-12V, IRF540, buck 5 V)."""
from PIL import Image, ImageDraw, ImageFont
# Uso: python3 zones.py <render_dall_alto.png> <uscita.png>
# Il render si ottiene con: kicad-cli pcb render --side top --width 2600 --height 2200 --quality high \
#     --background opaque -o top3d.png ../kicad/FilMachine_Mainboard.kicad_pcb
# Le zone sono rettangoli in mm (origine = angolo in alto a sinistra della scheda, 110 x 90 mm):
# se si spostano componenti in placement.py, aggiornare qui le coordinate.
import sys
im=Image.open(sys.argv[1]).convert("RGB")
# bordi della scheda nel render (pixel), ricavati dal colore verde del PCB
import numpy as np
_a=np.array(im).astype(int); _m=(_a[...,1]>_a[...,0])&(_a[...,1]>_a[...,2]+8)&(_a[...,1]<140)
_c=np.where(_m.sum(0)>500)[0]; _r=np.where(_m.sum(1)>500)[0]
X0,X1,Y0,Y1=_c.min(),_c.max(),_r.min(),_r.max()
sx=(X1-X0)/110; sy=(Y1-Y0)/90
def P(x,y): return (X0+x*sx, Y0+y*sy)
F=lambda s,b=True: ImageFont.truetype("DejaVuSans%s.ttf"%("-Bold" if b else ""), s)
# zones: id, color, rects(mm), dashed(new function)
Z=[
 ("1","#e53935",[(11.4,-0.3,42.4,29.6),(43.7,15.5,67.2,22.0)],False),
 ("2","#1e88e5",[(-0.3,53.5,10.9,65.2),(12.6,51.6,48.7,78.7)],False),
 ("3","#fb8c00",[(8.7,80.6,36.6,90.3)],False),
 ("4","#1565c0",[(42.1,44.2,53.1,51.9),(61.8,56.9,75.2,61.0)],False),
 ("5","#43a047",[(65.1,25.7,75.3,40.6),(58.7,62.7,94.4,80.7),(40.8,82.3,102.0,90.3)],False),
 ("6","#8e24aa",[(43.0,23.1,65.4,40.0)],False),
 ("7","#00acc1",[(95.3,8.8,110.1,76.3),(75.0,17.7,95.5,31.0),(76.8,52.3,94.3,56.5)],False),
 ("8","#43a047",[(52.9,35.8,62.0,54.6)],True),
 ("9","#757575",[(-0.3,7.7,11.0,47.1),(14.4,35.7,38.7,48.9),(52.8,54.3,60.1,61.6)],True),
 ("10","#757575",[(41.8,-0.3,84.0,15.4),(61.9,41.1,69.6,51.8)],True),
 ("11","#757575",[(83.8,-0.3,100.3,14.4)],True),
 ("12","#757575",[(47.3,61.4,58.1,77.4),(73.0,40.7,83.2,44.6),(85.7,40.7,93.1,47.8)],True),
]
def rgb(h): return tuple(int(h[i:i+2],16) for i in (1,3,5))
ov=Image.new("RGBA",im.size,(0,0,0,0)); d=ImageDraw.Draw(ov)
for zid,c,rs,dash in Z:
    for r in rs:
        a=P(r[0],r[1]); b=P(r[2],r[3])
        d.rectangle([a,b],fill=rgb(c)+((38 if dash else 70),))
im=Image.alpha_composite(im.convert("RGBA"),ov)
d=ImageDraw.Draw(im)
def dashed_rect(a,b,col,w=7,L=26,G=14):
    (x0,y0),(x1,y1)=a,b
    for (xa,ya,xb,yb) in ((x0,y0,x1,y0),(x0,y1,x1,y1),(x0,y0,x0,y1),(x1,y0,x1,y1)):
        n=max(abs(xb-xa),abs(yb-ya)); t=0
        while t<n:
            e=min(t+L,n); f=lambda k:(xa+(xb-xa)*k/n, ya+(yb-ya)*k/n)
            d.line([f(t),f(e)],fill=col,width=w); t+=L+G
for zid,c,rs,dash in Z:
    for r in rs:
        a=P(r[0],r[1]); b=P(r[2],r[3])
        if dash: dashed_rect(a,b,c)
        else: d.rectangle([a,b],outline=c,width=8)
    # badge on each rect's top-left
    for r in rs:
        a=P(r[0],r[1]); cx,cy=a[0]+34,a[1]+34; R=30
        d.ellipse([cx-R,cy-R,cx+R,cy+R],fill=c,outline="white",width=4)
        f=F(34 if len(zid)==1 else 28); w=d.textlength(zid,font=f)
        d.text((cx-w/2,cy-21),zid,font=f,fill="white")
# crop & add legend
im=im.crop((120,150,2460,2070))
W=im.width+1500; H=im.height
can=Image.new("RGB",(W,H),"white"); can.paste(im,(0,0)); d=ImageDraw.Draw(can)
x=im.width+50; y=40
d.text((x,y),"FilMachine Mainboard v1.1",font=F(52),fill="#222"); y+=70
d.text((x,y),"cosa sostituisce ogni zona della scheda",font=F(36,False),fill="#444"); y+=80
L=[("Moduli che la scheda sostituisce",None,None,None),
 ("1","#e53935","Modulo MOSFET IRF540 (2 canali) — riscaldatori","Q9/Q10 + gate driver U5, fusibili F1/F2, morsetti HEATER 1/2"),
 ("2","#1e88e5","DBH-12V canale B — pompa","ponte U8/U9 (BTN8962), condensatori, morsetto PUMP"),
 ("3","#fb8c00","DBH-12V canale A — motore spirale","ponte H U10 (DRV8871), morsetto MOTOR"),
 ("4","#1565c0","DBH-12V — logica di comando ENA·IN","porte AND U6/U7 + pull-down dei comandi"),
 ("5","#43a047","Adafruit #6318 — MCP23017 + 8 uscite","U3 (I2C 0x20) + MOSFET Q1–Q8, LED, morsetti valvole"),
 ("6","#8e24aa","Convertitore 12 → 5 V","buck U1 (LMR33630, 3 A), L1, condensatori"),
 ("7","#00acc1","Cablaggio sensori e servo (oggi fili diretti a JP1)","JST di livelli, flusso, Hall, DS18B20 + filtri/pull-up; header SERVO"),
 ("Funzioni nuove (tratteggiate)",None,None,None),
 ("8","#43a047","GPIO SPARE: pin A6/A7 del #6318","header J30, protezioni R97/R98, D47/D48"),
 ("9","#757575","Ingressi 12 V e protezioni","3 morsetti 12V IN, fusibili F3/F4, TVS D1/D2, C1"),
 ("10","#757575","Connettore e alimentazione display","box header 2×13, PTC F7 + diodo D8"),
 ("11","#757575","Uscite di servizio 12 V / 5 V","morsetti 12V OUT / 5V OUT, PTC F8/F5"),
 ("12","#757575","Monitor tensioni/corrente pompa + LED","ADC U4 (ADS1115), partitori, LED 12V/5V/3V3"),
]
for zid,c,t,s in L:
    if c is None:
        y+=20; d.text((x,y),zid,font=F(38),fill="#222"); y+=62; continue
    R=30; cx,cy=x+R,y+R
    d.ellipse([cx-R,cy-R,cx+R,cy+R],fill=c)
    f=F(34 if len(zid)==1 else 28); w=d.textlength(zid,font=f); d.text((cx-w/2,cy-21),zid,font=f,fill="white")
    d.text((x+80,y-6),t,font=F(32),fill="#222"); d.text((x+80,y+36),s,font=F(26,False),fill="#555"); y+=104
can.save(sys.argv[2]); print(can.size,y)
