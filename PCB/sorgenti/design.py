# -*- coding: utf-8 -*-
"""
FilMachine Mainboard v1.1 — single source of truth for schematic + PCB (JLCPCB / LCSC parts).

Every part:  P(ref, symbol, value, footprint, pins={pin: net}, sheet, mpn, desc, dnp, mfr, lcsc)
Pin keys are symbol pin numbers (strings). Nets are plain strings. `lcsc` is the LCSC part number used by
JLCPCB assembly (verified in stock on 2026-10-13).
"""

PARTS = []
NC = None     # pin explicitly not connected (no-connect flag in schematic)

FP_R = "Resistor_SMD:R_0603_1608Metric"
FP_C = "Capacitor_SMD:C_0603_1608Metric"
FP_C0805 = "Capacitor_SMD:C_0805_2012Metric"
FP_C1206 = "Capacitor_SMD:C_1206_3216Metric"
FP_LED0603 = "LED_SMD:LED_0603_1608Metric"
FP_LED0805 = "LED_SMD:LED_0805_2012Metric"

# JLCPCB *Basic* 0603 1 % resistors (UNI-ROYAL 0603WAF…): value -> LCSC
R_LCSC = {"10R": "C22859", "100R": "C22775", "220R": "C22962", "1k": "C21190", "1.5k": "C22843", "4.7k": "C23162",
          "10k": "C25804", "15k": "C22809", "20k": "C4184", "24k": "C23352", "30k": "C22984", "56k": "C23206",
          "100k": "C25803", "120k": "C25808", "220k": "C22961", "1M": "C22935"}
# JLCPCB *Basic* MLCC: (value, footprint) -> LCSC
C_LCSC = {("100nF", FP_C): "C14663", ("1nF", FP_C): "C1588", ("1uF 50V", FP_C): "C15849",
          ("220nF 50V", FP_C0805): "C5378", ("100nF 50V", FP_C0805): "C49678", ("1uF 50V", FP_C0805): "C28323",
          ("4.7uF 25V", FP_C0805): "C1779", ("10uF 25V", FP_C0805): "C15850",
          ("10uF 50V", FP_C1206): "C13585", ("22uF 25V", FP_C1206): "C12891", ("47uF 10V", FP_C1206): "C96123"}


def P(ref, sym, value, fp, pins, sheet, mpn="", desc="", dnp=False, mfr="", unit_pins=None, lcsc=""):
    PARTS.append(dict(ref=ref, sym=sym, value=value, fp=fp, pins=pins, sheet=sheet,
                      mpn=mpn, desc=desc, dnp=dnp, mfr=mfr, lcsc=lcsc))


def R(ref, val, a, b, sheet, desc="", dnp=False, fp=FP_R, mpn=None):
    assert val in R_LCSC, val
    P(ref, "Device:R", val, fp, {"1": a, "2": b}, sheet,
      mpn or f"0603 {val} 1%", desc, dnp, mfr="UNI-ROYAL", lcsc=R_LCSC[val])


def C(ref, val, a, b, sheet, fp=FP_C, mpn=None, desc="", dnp=False):
    assert (val, fp) in C_LCSC, (val, fp)
    P(ref, "Device:C", val, fp, {"1": a, "2": b}, sheet,
      mpn or f"MLCC {val}", desc, dnp, mfr="Samsung/Yageo (JLCPCB basic)", lcsc=C_LCSC[(val, fp)])


def CP(ref, val, a, b, sheet, fp, mpn, desc="", mfr="", lcsc=""):
    P(ref, "Device:C_Polarized", val, fp, {"1": a, "2": b}, sheet, mpn, desc, mfr=mfr, lcsc=lcsc)


def LED(ref, anode, cathode, sheet, color="Green", desc=""):
    # JLCPCB basic LEDs: red 0603 KT-0603R, green/yellow 0805 KT-0805G / KT-0805Y (Hubei KENTO)
    mpn, fp, lcsc = {"Green": ("KT-0805G", FP_LED0805, "C2297"), "Yellow": ("KT-0805Y", FP_LED0805, "C2296"),
                     "Red": ("KT-0603R", FP_LED0603, "C2286")}[color]
    P(ref, "Device:LED", color, fp, {"1": cathode, "2": anode}, sheet, mpn, desc, mfr="Hubei KENTO", lcsc=lcsc)


TB2_508 = "FM:TB_WJ500V-5.08-2P"       # Ningbo Kangnex WJ500V-5.08-2P, 18 A screw terminal (C8465)
TB2_35 = "FM:TB_DB301V-3.5-2P"         # DORABO DB301V-3.5-2P, 7 A screw terminal (C695629)
FUSE2410 = "FM:FUSE_2410_SMD"

# ════════════════════════════════════════════════════════════════════════════
# SHEET 1 — POWER
# ════════════════════════════════════════════════════════════════════════════
S = "power"
# Three 12 V inputs, one per +V/-V pair of the LRS-350-12 (it has 3 pairs): each terminal carries <= 8.3 A.
P("J1", "Connector_Generic:Conn_01x02", "12V IN HEATER 1", TB2_508,
  {"1": "GND", "2": "+12V_IN_H1"}, S, "WJ500V-5.08-2P", "Ingresso 12 V riscaldatore 1 (LRS-350 coppia n.1), pin 2 = +", mfr="Ningbo Kangnex", lcsc="C8465")
P("J2", "Connector_Generic:Conn_01x02", "12V IN HEATER 2", TB2_508,
  {"1": "GND", "2": "+12V_IN_H2"}, S, "WJ500V-5.08-2P", "Ingresso 12 V riscaldatore 2 (LRS-350 coppia n.2), pin 2 = +", mfr="Ningbo Kangnex", lcsc="C8465")
P("J3", "Connector_Generic:Conn_01x02", "12V IN MAIN", TB2_508,
  {"1": "GND", "2": "+12V_IN_M"}, S, "WJ500V-5.08-2P", "Ingresso 12 V pompa/valvole/motore/logica (LRS-350 coppia n.3), pin 2 = +", mfr="Ningbo Kangnex", lcsc="C8465")

# SMD 2410 time-lag fuses (JDT), 125 V, 100 A breaking @ 125 V
P("F1", "Device:Fuse", "15A", FUSE2410, {"1": "+12V_IN_H1", "2": "+12V_H1"}, S,
  "JFC2410-2150TS", "Fusibile SMD 2410 ritardato 15 A — riscaldatore 1", mfr="Shenzhen JDT Fuse", lcsc="C136393")
P("F2", "Device:Fuse", "15A", FUSE2410, {"1": "+12V_IN_H2", "2": "+12V_H2"}, S,
  "JFC2410-2150TS", "Fusibile SMD 2410 ritardato 15 A — riscaldatore 2", mfr="Shenzhen JDT Fuse", lcsc="C136393")
P("F3", "Device:Fuse", "15A", FUSE2410, {"1": "+12V_IN_M", "2": "+12V_PUMP"}, S,
  "JFC2410-2150TS", "Fusibile SMD 2410 ritardato 15 A — pompa", mfr="Shenzhen JDT Fuse", lcsc="C136393")
P("F4", "Device:Fuse", "8A", FUSE2410, {"1": "+12V_IN_M", "2": "+12V"}, S,
  "JFC2410-1800TS", "Fusibile SMD 2410 ritardato 8 A — valvole, motore, logica", mfr="Shenzhen JDT Fuse", lcsc="C136390")

P("D1", "Device:D_Zener", "SMBJ16A", "Diode_SMD:D_SMB", {"1": "+12V", "2": "GND"}, S,
  "SMBJ16A", "TVS 16 V 600 W: transitori + crowbar inversione polarità (brucia F4)", mfr="hongjiacheng", lcsc="C19077571")
P("D2", "Device:D_Zener", "SMBJ16A", "Diode_SMD:D_SMB", {"1": "+12V_PUMP", "2": "GND"}, S,
  "SMBJ16A", "TVS 16 V sul rail pompa (energia di ritorno del motore)", mfr="hongjiacheng", lcsc="C19077571")

CP("C1", "100uF 35V", "+12V", "GND", S, "Capacitor_THT:CP_Radial_D6.3mm_P2.50mm",
   "ERS1VM101E12OT", "Bulk rail +12V logica/valvole (6.3x12, 5000 h)", mfr="AISHI", lcsc="C106694")

# 12 V -> 5.17 V / 3 A buck (TI LMR33630ADDA, 400 kHz; datasheet Tab. 9-3: L 8 uH, Cout 4x22 uF, Cin 10 uF + 220 nF)
P("U1", "Regulator_Switching:LMR33630ADDA", "LMR33630ADDA", "Package_SO:TI_SO-PowerPAD-8_ThermalVias",
  {"2": "+12V", "3": "+12V", "6": "BUCK_VCC", "1": "GND", "9": "GND",
   "7": "BUCK_BOOT", "8": "BUCK_SW", "5": "BUCK_FB", "4": NC}, S,
  "LMR33630ADDAR", "Buck sincrono 3,8–36 V -> 5 V 3 A", mfr="Texas Instruments", lcsc="C841384")
C("C2", "10uF 50V", "+12V", "GND", S, FP_C1206, desc="Cin buck")
C("C3", "10uF 50V", "+12V", "GND", S, FP_C1206, desc="Cin buck")
C("C4", "220nF 50V", "+12V", "GND", S, FP_C0805, desc="Cin HF buck")
C("C5", "100nF", "BUCK_BOOT", "BUCK_SW", S, desc="Bootstrap")
C("C6", "1uF 50V", "BUCK_VCC", "GND", S, desc="VCC interno buck")
P("L1", "Device:L", "10uH", "FM:L_SLO0630H", {"1": "BUCK_SW", "2": "+5V"}, S,
  "SLO0630H100MTT", "Induttore buck 10 uH, 4 A rms / 5,5 A sat, 66 mOhm", mfr="Sunltech", lcsc="C207842")
R("R1", "100k", "+5V", "BUCK_FB", S, "Partitore FB (Vout = 1 V x (1+100k/24k) = 5,17 V)")
R("R2", "24k", "BUCK_FB", "GND", S, "Partitore FB")
for i, ref in enumerate(["C7", "C8", "C9", "C10"]):
    C(ref, "22uF 25V", "+5V", "GND", S, FP_C1206, desc="Cout buck")

# La logica della scheda (MCP23017, ADS1115, porte AND, pull-up, DS18B20) e' alimentata dai 3,3 V del display
# (JP1 pin 1/3/18, ~15 mA): cosi' e' viva anche con la scheda spenta e il display su USB (I2C mai bloccato).

# Uscite di servizio protette da PTC (autoripristinanti): 5 V (1,1 A) e 12 V (2 A) su morsetti a vite,
# per alimentare eventuali accessori (ventola, striscia LED, modulo rele', ...). Il 12 V e' preso a valle di F4.
P("F5", "Device:Polyfuse", "1.1A", "Fuse:Fuse_1812_4532Metric", {"1": "+5V", "2": "+5V_AUX"}, S,
  "SMD1812B110TF/33", "PTC 1,1 A uscita 5 V di servizio (J29)", mfr="Brightking", lcsc="C269134")
P("F6", "Device:Polyfuse", "0.5A", "Fuse:Fuse_1206_3216Metric", {"1": "+5V", "2": "+5V_SENS"}, S,
  "JK-nSMD050-30", "PTC 0,5 A alimentazione sensori", mfr="Jinrui", lcsc="C720075")
P("F8", "Device:Polyfuse", "2A", "Fuse:Fuse_1812_4532Metric", {"1": "+12V", "2": "+12V_AUX"}, S,
  "BSMD1812-200-30V", "PTC 2 A (scatto 4 A) uscita 12 V di servizio (J28)", mfr="BHFUSE", lcsc="C960026")
P("J28", "Connector_Generic:Conn_01x02", "12V OUT", TB2_35,
  {"1": "GND", "2": "+12V_AUX"}, S, "DB301V-3.5-2P-GN-S", "Uscita 12 V di servizio, max 2 A (pin 2 = +, pin 1 = -)", mfr="DORABO", lcsc="C695629")
P("J29", "Connector_Generic:Conn_01x02", "5V OUT", TB2_35,
  {"1": "GND", "2": "+5V_AUX"}, S, "DB301V-3.5-2P-GN-S", "Uscita 5 V di servizio, max 1 A (pin 2 = +, pin 1 = -)", mfr="DORABO", lcsc="C695629")
C("C63", "10uF 25V", "+12V_AUX", "GND", S, fp=FP_C0805, desc="Disaccoppiamento uscita 12 V di servizio")
C("C64", "10uF 25V", "+5V_AUX", "GND", S, fp=FP_C0805, desc="Disaccoppiamento uscita 5 V di servizio")

# LED di stato alimentazioni
R("R3", "4.7k", "+12V", "LED12_A", S, "LED 12V")
LED("D3", "LED12_A", "GND", S, "Green", "12 V logica presente")
R("R4", "1k", "+5V", "LED5_A", S, "LED 5V")
LED("D4", "LED5_A", "GND", S, "Green", "5 V presente")
R("R5", "1k", "+3V3", "LED33_A", S, "LED 3V3")
LED("D5", "LED33_A", "GND", S, "Green", "3,3 V dal display presente (display collegato e acceso)")

for i, ref in enumerate(["H1", "H2", "H3", "H4"]):
    P(ref, "Mechanical:MountingHole", "M3", "MountingHole:MountingHole_3.2mm_M3", {}, S,
      "", "Foro di fissaggio M3")

# ════════════════════════════════════════════════════════════════════════════
# SHEET 2 — DISPLAY LINK, I/O EXPANDER, ADC, SENSOR INPUTS
# ════════════════════════════════════════════════════════════════════════════
S = "io"
# JP1 del JC4880P433 (schema Guition, "Expand IO"): verificato pin per pin
JP1_MAP = {
    "1": "+3V3", "2": "5V_DISP", "3": "+3V3", "4": "5V_DISP",      # 3,3 V DAL display (VCC3V3) -> logica scheda
    "5": "GND", "6": "GND",
    "7": "FLOW_IN",      # GPIO52
    "8": "MOT_IN1",      # GPIO33
    "9": "PUMP_ENA",     # GPIO51
    "10": "HALL_IN",     # GPIO31
    "11": "PUMP_IN2",    # GPIO50
    "12": "BATH_MAX_IN", # GPIO30
    "13": "PUMP_IN1",    # GPIO49
    "14": "BATH_MIN_IN", # GPIO29
    "15": "ONEWIRE",     # GPIO35 (strapping: deve essere alto al boot -> pull-up 4k7 sui 3,3 V del display)
    "16": "GND",
    "17": "MOT_IN2",     # GPIO34 (strapping JTAG, solo con eFuse: il pull-down da 100k non cambia il boot)
    "18": NC,            # ESP_3V3 del display: lasciato scollegato (puo' essere un regolatore diverso da VCC3V3 dei pin 1/3)
    "19": "MOT_ENA",     # GPIO32
    "20": "C6_RXD",
    "21": "SERVO_PWM",   # GPIO28
    "22": "C6_TXD",
    "23": "I2C_SDA",     # ES_I2C_SDA (GPIO7) — pull-up 2k2 gia' sul display
    "24": "C6_IO9",
    "25": "I2C_SCL",     # ES_I2C_SCL (GPIO8)
    "26": "C6_PU",
}
P("J5", "Connector_Generic:Conn_02x13_Odd_Even", "DISPLAY JP1", "Connector_IDC:IDC-Header_2x13_P2.54mm_Horizontal",
  JP1_MAP, S, "DS1013-26RSiB-B-0", "Box header 2x13 ad angolo retto (cavo verso il bordo) per flat verso JP1 del display JC4880P433", mfr="CONNFLY", lcsc="C7430326")
# Alimentazione del display dal flat (JP1 pin 2/4 = VCC5V del JC4880P433, a valle dell'IP5306 tramite R22 1 A).
# PTC dedicato + Schottky in serie: (1) il display non puo' mai alimentare all'indietro il 5 V della scheda;
# (2) con la USB-C del display collegata al PC, il VBUS del PC (5 V) sta sopra il nostro (~4,7 V) e vince:
#     nessuna corrente verso il PC, nessun conflitto tra le due alimentazioni. Display ~320 mA (datasheet Guition).
P("F7", "Device:Polyfuse", "1.1A", "Fuse:Fuse_1812_4532Metric", {"1": "+5V", "2": "5V_DISP_F"}, S,
  "SMD1812B110TF/33", "PTC 1,1 A alimentazione display via JP1", mfr="Brightking", lcsc="C269134")
P("D8", "Device:D_Schottky", "SS34", "Diode_SMD:D_SMA", {"1": "5V_DISP", "2": "5V_DISP_F"}, S,
  "SS34", "Schottky 3 A 40 V: blocca il ritorno dal display (USB-C) verso il 5 V della scheda", mfr="JLCPCB basic", lcsc="C8678")
C("C62", "10uF 25V", "5V_DISP", "GND", S, fp=FP_C0805, desc="Bulk sul 5 V verso il display")
P("J6", "Connector_Generic:Conn_01x05", "C6 UART", "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical",
  {"1": "GND", "2": "C6_RXD", "3": "C6_TXD", "4": "C6_IO9", "5": "C6_PU"}, S, "", "Accesso ESP32-C6 (non montato)", dnp=True)
R("R6", "4.7k", "I2C_SDA", "+3V3", S, "Pull-up I2C (non montata: c'e' gia' 2k2 sul display)", dnp=True)
R("R7", "4.7k", "I2C_SCL", "+3V3", S, "Pull-up I2C (non montata)", dnp=True)

# MCP23017 @0x20 — stessa mappa del firmware attuale (board_jc4880p433.h + note v6)
P("U3", "Interface_Expansion:MCP23017_SS", "MCP23017-E/SS", "Package_SO:SSOP-28_5.3x10.2mm_P0.65mm",
  {"9": "+3V3", "10": "GND", "13": "I2C_SDA", "12": "I2C_SCL", "18": "MCP_RST",
   "15": "GND", "16": "GND", "17": "GND", "19": NC, "20": NC, "11": NC, "14": NC,
   "21": "VLV_C1", "22": "VLV_C2", "23": "VLV_C3", "24": "VLV_WB",
   "25": "VLV_WASTE", "26": "VLV_INLET", "27": "VLV_SP1", "28": "VLV_SP2",
   "1": "CHEM1_MIN", "2": "CHEM1_MAX", "3": "CHEM2_MIN", "4": "CHEM2_MAX",
   "5": "CHEM3_MIN", "6": "CHEM3_MAX", "7": "HEAT1_CMD", "8": "HEAT2_CMD"}, S,
  "MCP23017T-E/SS", "Espansore I2C 16 I/O, indirizzo 0x20", mfr="Microchip", lcsc="C558584")
C("C13", "100nF", "+3V3", "GND", S, desc="Disaccoppiamento MCP23017")
R("R8", "10k", "MCP_RST", "+3V3", S, "Pull-up RESET MCP23017")
C("C14", "100nF", "MCP_RST", "GND", S, desc="Power-on reset MCP23017")

# ADS1115 @0x48 (ADDR a GND) — monitor tensioni (HAS_RAIL_MONITOR) + corrente pompa; PGA +-2,048 V, ingressi single-ended
P("U4", "Analog_ADC:ADS1115IDGS", "ADS1115", "Package_SO:VSSOP-10_3x3mm_P0.5mm",
  {"1": "GND", "2": NC, "3": "GND", "8": "+3V3", "9": "I2C_SDA", "10": "I2C_SCL",
   "4": "ADC_12V", "5": "ADC_5V", "6": "ADC_PUMP_IA", "7": "ADC_PUMP_IB"}, S,
  "ADS1115IDGSR", "ADC I2C 16 bit @0x48: AIN0 12 V, AIN1 5 V, AIN2/AIN3 corrente pompa lato A/B", mfr="Texas Instruments", lcsc="C37593")
C("C15", "100nF", "+3V3", "GND", S, desc="Disaccoppiamento ADS1115")
R("R9", "100k", "+12V", "ADC_12V", S, "Partitore 12 V: x 15/115 (12 V -> 1,57 V)")
R("R10", "15k", "ADC_12V", "GND", S, "Partitore 12 V")
C("C16", "100nF", "ADC_12V", "GND", S)
R("R11", "20k", "+5V", "ADC_5V", S, "Partitore 5 V: x 1/3 (5,17 V -> 1,72 V)")
R("R12", "10k", "ADC_5V", "GND", S, "Partitore 5 V")
C("C17", "100nF", "ADC_5V", "GND", S)
for side, rs, cs, ds in (("A", "R13", "C18", "D6"), ("B", "R14", "C19", "D7")):
    R(rs, "10k", f"PUMP_IS_{side}", f"ADC_PUMP_I{side}", S, f"Serie IS lato {side} verso ADC (limita la corrente di fault)")
    C(cs, "100nF", f"ADC_PUMP_I{side}", "GND", S)
    P(ds, "Device:D_Schottky", "1N5819WS", "Diode_SMD:D_SOD-323", {"1": "+3V3", "2": f"ADC_PUMP_I{side}"}, S,
      "1N5819WS", f"Clamp ingresso ADC lato {side} a 3,3 V", mfr="JLCPCB basic", lcsc="C191023")

# Ingressi sensori digitali: diodo Schottky + pull-up a 3,3 V.
# Il sensore (5 V) puo' solo tirare GIU' la linea; un'uscita a 5 V non arriva mai al GPIO.
# Funziona con uscite push-pull, NPN open-collector e moduli con pull-up a 5 V.
INPUTS = [  # (canale, net lato MCU, net lato connettore)
    ("BATH_MIN", "BATH_MIN_IN", "SNS_BATH_MIN"),
    ("BATH_MAX", "BATH_MAX_IN", "SNS_BATH_MAX"),
    ("HALL",     "HALL_IN",     "SNS_HALL"),
    ("FLOW",     "FLOW_IN",     "SNS_FLOW"),
    ("C1_MIN",   "CHEM1_MIN",   "SNS_C1_MIN"),
    ("C1_MAX",   "CHEM1_MAX",   "SNS_C1_MAX"),
    ("C2_MIN",   "CHEM2_MIN",   "SNS_C2_MIN"),
    ("C2_MAX",   "CHEM2_MAX",   "SNS_C2_MAX"),
    ("C3_MIN",   "CHEM3_MIN",   "SNS_C3_MIN"),
    ("C3_MAX",   "CHEM3_MAX",   "SNS_C3_MAX"),
]
for i, (ch, mcu, sns) in enumerate(INPUTS):
    P(f"D{10+i}", "Device:D_Schottky", "1N5819WS", "Diode_SMD:D_SOD-323", {"1": sns, "2": mcu}, S,
      "1N5819WS", f"Adattatore livello ingresso {ch}", mfr="JLCPCB basic", lcsc="C191023")
    R(f"R{20+i}", "10k", mcu, "+3V3", S, f"Pull-up ingresso {ch}")
    C(f"C{20+i}", "1nF", mcu, "GND", S, desc=f"Filtro ingresso {ch}")

XH4 = "Connector_JST:JST_XH_B4B-XH-A_1x04_P2.50mm_Vertical"
XH3 = "Connector_JST:JST_XH_B3B-XH-A_1x03_P2.50mm_Vertical"
P("J7", "Connector_Generic:Conn_01x04", "BATH LEVEL", XH4,
  {"1": "+5V_SENS", "2": "SNS_BATH_MIN", "3": "SNS_BATH_MAX", "4": "GND"}, S,
  "XH-4A", "XKC-Y21 bagno MIN/MAX (5V, MIN, MAX, GND)", mfr="BOOMELE", lcsc="C37815")
P("J8", "Connector_Generic:Conn_01x04", "CHEM 1", XH4,
  {"1": "+5V_SENS", "2": "SNS_C1_MIN", "3": "SNS_C1_MAX", "4": "GND"}, S,
  "XH-4A", "XKC-Y21 chimica 1 MIN/MAX", mfr="BOOMELE", lcsc="C37815")
P("J9", "Connector_Generic:Conn_01x04", "CHEM 2", XH4,
  {"1": "+5V_SENS", "2": "SNS_C2_MIN", "3": "SNS_C2_MAX", "4": "GND"}, S,
  "XH-4A", "XKC-Y21 chimica 2 MIN/MAX", mfr="BOOMELE", lcsc="C37815")
P("J10", "Connector_Generic:Conn_01x04", "CHEM 3", XH4,
  {"1": "+5V_SENS", "2": "SNS_C3_MIN", "3": "SNS_C3_MAX", "4": "GND"}, S,
  "XH-4A", "XKC-Y21 chimica 3 MIN/MAX", mfr="BOOMELE", lcsc="C37815")
P("J11", "Connector_Generic:Conn_01x03", "FLOW", XH3,
  {"1": "+5V_SENS", "2": "SNS_FLOW", "3": "GND"}, S, "XH-3A", "Flussimetro YF-S201", mfr="BOOMELE", lcsc="C2316")
P("J12", "Connector_Generic:Conn_01x03", "HALL", XH3,
  {"1": "+5V_SENS", "2": "SNS_HALL", "3": "GND"}, S, "XH-3A", "Sensore Hall KY-003", mfr="BOOMELE", lcsc="C2316")
P("J13", "Connector_Generic:Conn_01x03", "TEMP 1", XH3,
  {"1": "+3V3", "2": "ONEWIRE", "3": "GND"}, S, "XH-3A", "DS18B20 bagno", mfr="BOOMELE", lcsc="C2316")
P("J14", "Connector_Generic:Conn_01x03", "TEMP 2", XH3,
  {"1": "+3V3", "2": "ONEWIRE", "3": "GND"}, S, "XH-3A", "DS18B20 chimica", mfr="BOOMELE", lcsc="C2316")
R("R16", "4.7k", "ONEWIRE", "+3V3", S, "Pull-up OneWire ai 3,3 V del display: GPIO35 (strapping) alto al boot")

# ════════════════════════════════════════════════════════════════════════════
# SHEET 3 — VALVOLE E RISCALDATORI
# ════════════════════════════════════════════════════════════════════════════
S = "outputs"
VALVES = [("C1", "VLV_C1"), ("C2", "VLV_C2"), ("C3", "VLV_C3"), ("WB", "VLV_WB"),
          ("WASTE", "VLV_WASTE"), ("INLET", "VLV_INLET"), ("SP1", "VLV_SP1"), ("SP2", "VLV_SP2")]
for i, (name, sig) in enumerate(VALVES):
    n = i + 1
    drain = f"VLV{n}_N"
    gate = f"VLV{n}_G"
    spare = name.startswith("SP")
    R(f"R{30+n}", "100R", sig, gate, S, f"Gate valvola {name}")
    # SP1/SP2 (A6/A7, liberi nel firmware): pull-down da 1 M cosi' il pin puo' anche fare da INGRESSO su J30
    # con il pull-up interno (100k) dell'MCP23017; gate comunque a 0 durante il reset
    R(f"R{40+n}", "1M" if spare else "100k", gate, "GND", S, f"Pull-down gate {'uscita spare' if spare else 'valvola'} {name} (spenta al boot)")
    P(f"Q{n}", "Transistor_FET:AO3400A", "AO3400A", "Package_TO_SOT_SMD:SOT-23",
      {"1": gate, "2": "GND", "3": drain}, S, "AO3400A", f"Low-side {'uscita spare' if spare else 'valvola'} {name} (30 V, 5,7 A)", mfr="Alpha & Omega", lcsc="C20917")
    P(f"D{20+n}", "Device:D_Schottky", "B5819W", "Diode_SMD:D_SOD-123", {"1": "+12V", "2": drain}, S,
      "B5819W SL", f"Ricircolo carico induttivo {name}", mfr="JLCPCB basic", lcsc="C8598")
    R(f"R{50+n}", "4.7k", "+12V", f"VLV{n}_LED", S, f"LED {name}")
    LED(f"D{30+n}", f"VLV{n}_LED", drain, S, "Yellow", f"{'Uscita spare' if spare else 'Valvola'} {name} attiva")
    P(f"J{14+n}", "Connector_Generic:Conn_01x02", f"SPARE OUT {name[-1]}" if spare else f"VALVE {name}", TB2_35,
      {"1": "+12V", "2": drain}, S, "DB301V-3.5-2P-GN-S",
      (f"Uscita 12 V commutata di riserva {name[-1]} (MCP23017 A{5+int(name[-1])}): valvola, rele', carico fino a 2 A; pin 1 = +12 V, pin 2 = -" if spare
       else f"Valvola {name} (pin 1 = +12 V, pin 2 = -)"), mfr="DORABO", lcsc="C695629")
# I due pin liberi dell'MCP23017 (A6, A7 = SP1, SP2) anche come GPIO 3,3 V su header, con 3,3 V, 5 V (PTC F6) e GND.
# Usati come uscite commutano anche SPARE OUT 1/2; come ingressi vanno letti col pull-up interno dell'MCP.
R("R97", "1k", "VLV_SP1", "SP1_IO", S, "Serie verso J30 pin 1 (limita la corrente se vi arriva un segnale a 5 V)")
R("R98", "1k", "VLV_SP2", "SP2_IO", S, "Serie verso J30 pin 2")
P("D47", "Device:D_Schottky", "1N5819WS", "Diode_SMD:D_SOD-323", {"1": "+3V3", "2": "SP1_IO"}, S,
  "1N5819WS", "Clamp J30 pin 1 a 3,3 V", mfr="JLCPCB basic", lcsc="C191023")
P("D48", "Device:D_Schottky", "1N5819WS", "Diode_SMD:D_SOD-323", {"1": "+3V3", "2": "SP2_IO"}, S,
  "1N5819WS", "Clamp J30 pin 2 a 3,3 V", mfr="JLCPCB basic", lcsc="C191023")
P("J30", "Connector_Generic:Conn_01x05", "GPIO SPARE", "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical",
  {"1": "SP1_IO", "2": "SP2_IO", "3": "+3V3", "4": "+5V_AUX", "5": "GND"}, S, "PZ254V-11-05P",
  "Header 2,54: A6, A7 (GPIO liberi MCP23017, 3,3 V, tramite 1k + clamp), +3V3, +5V di servizio (PTC F5), GND", mfr="XFCN", lcsc="C492404")

# Riscaldatori 2 x 100 W (8,3 A): gate driver a 12 V + MOSFET 40 V / 7 mOhm
P("U5", "Driver_FET:UCC27524D", "UCC27524D", "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm",
  {"6": "+12V", "3": "GND", "2": "HEAT1_CMD", "4": "HEAT2_CMD", "1": NC, "8": NC,
   "7": "HEAT1_DRV", "5": "HEAT2_DRV"}, S,
  "UCC27524DR", "Gate driver doppio low-side (ingressi 3,3 V, uscite 12 V); EN interni a pull-up", mfr="Texas Instruments", lcsc="C465729")
C("C40", "1uF 50V", "+12V", "GND", S, FP_C0805, desc="Disaccoppiamento gate driver")
C("C41", "100nF", "+12V", "GND", S, desc="Disaccoppiamento gate driver")
R("R61", "100k", "HEAT1_CMD", "GND", S, "Pull-down comando riscaldatore 1 (in parallelo ai 120 k interni dell'UCC27524)")
R("R62", "100k", "HEAT2_CMD", "GND", S, "Pull-down comando riscaldatore 2")
for n in (1, 2):
    q = f"Q{8+n}"
    R(f"R{62+n*2-1}", "10R", f"HEAT{n}_DRV", f"HEAT{n}_G", S, f"Gate riscaldatore {n}")
    R(f"R{62+n*2}", "10k", f"HEAT{n}_G", "GND", S, f"Pull-down gate riscaldatore {n}")
    P(q, "Transistor_FET:IPD50R380CE", "AOD4184A", "Package_TO_SOT_SMD:TO-252-2",
      {"1": f"HEAT{n}_G", "2": f"HEAT{n}_N", "3": "GND"}, S,
      "AOD4184A", f"Low-side riscaldatore {n} (40 V, 50 A, 7 mOhm @10 V)", mfr="Alpha & Omega", lcsc="C99124")
    P(f"D{40+n}", "Device:D_Schottky", "SS34", "Diode_SMD:D_SMA", {"1": f"+12V_H{n}", "2": f"HEAT{n}_N"}, S,
      "SS34", f"Ricircolo induttanza cavi riscaldatore {n}", mfr="JLCPCB basic", lcsc="C8678")
    R(f"R{66+n}", "4.7k", f"HEAT{n}_DRV", f"HEAT{n}_LED", S, f"LED riscaldatore {n} (dal gate driver, 12 V = acceso)")
    LED(f"D{42+n}", f"HEAT{n}_LED", "GND", S, "Red", f"Riscaldatore {n} comandato acceso")
    P(f"J{22+n}", "Connector_Generic:Conn_01x02", f"HEATER {n}", TB2_508,
      {"1": f"HEAT{n}_N", "2": f"+12V_H{n}"}, S, "WJ500V-5.08-2P", f"Riscaldatore {n} 100 W 12 V (pin 2 = +, pin 1 = -)", mfr="Ningbo Kangnex", lcsc="C8465")

# ════════════════════════════════════════════════════════════════════════════
# SHEET 4 — POMPA, MOTORE SPIRALE, SERVO
# ════════════════════════════════════════════════════════════════════════════
S = "drives"
# Pull-down: tutti i comandi a 0 durante il boot dell'ESP32-P4 (pompa e motore fermi)
for i, net in enumerate(["PUMP_IN1", "PUMP_IN2", "PUMP_ENA", "MOT_IN1", "MOT_IN2", "MOT_ENA", "SERVO_PWM"]):
    R(f"R{70+i}", "100k", net, "GND", S, f"Pull-down {net}")

# Logica DBH-12V ricreata: ramo X attivo = INx AND ENA (ENA porta il PWM)
P("U6", "74xGxx:74LVC2G08", "74LVC2G08", "Package_SO:SSOP-8_2.95x2.8mm_P0.65mm",
  {"1": "PUMP_IN1", "2": "PUMP_ENA", "7": "PUMP_A_CTL",
   "5": "PUMP_IN2", "6": "PUMP_ENA", "3": "PUMP_B_CTL",
   "8": "+3V3", "4": "GND"}, S, "SN74LVC2G08DCTR", "AND: IN1·ENA, IN2·ENA pompa", mfr="Texas Instruments", lcsc="C352973")
C("C50", "100nF", "+3V3", "GND", S)
P("U7", "74xGxx:74LVC2G08", "74LVC2G08", "Package_SO:SSOP-8_2.95x2.8mm_P0.65mm",
  {"1": "MOT_IN1", "2": "MOT_ENA", "7": "MOT_A_CTL",
   "5": "MOT_IN2", "6": "MOT_ENA", "3": "MOT_B_CTL",
   "8": "+3V3", "4": "GND"}, S, "SN74LVC2G08DCTR", "AND: IN1·ENA, IN2·ENA motore", mfr="Texas Instruments", lcsc="C352973")
C("C51", "100nF", "+3V3", "GND", S)

# Ponte H pompa (UP3-R 12 V, 6 A): 2 x Infineon BTN8962TA (stesso pinout del BTN8982TA, 14 mOhm, limite 30 A min)
for n, side in ((8, "A"), (9, "B")):
    P(f"U{n}", "Power_Management:BTN8982TA", "BTN8962TA", "Package_TO_SOT_SMD:TO-263-7_TabPin8",
      {"7": "+12V_PUMP", "1": "GND", "2": f"PUMP_{side}_IN", "3": "PUMP_INH", "6": f"PUMP_IS_{side}",
       "5": "GND", "4": f"PUMP_OUT_{side}", "8": f"PUMP_OUT_{side}"}, S,
      "BTN8962TAAUMA1", f"Mezzo ponte pompa lato {side} (30 A, 14 mOhm); SR a GND = slew rate massimo", mfr="Infineon", lcsc="C112630")
    R(f"R{80 if side=='A' else 82}", "10k", f"PUMP_{side}_CTL", f"PUMP_{side}_IN", S,
      f"Serie ingresso IN (datasheet: > 7 kOhm)")
    C(f"C{52 if side=='A' else 53}", "100nF 50V", "+12V_PUMP", "GND", S, FP_C0805, desc="VS locale")
R("R84", "1k", "+3V3", "PUMP_INH", S, "INH alto = ponte attivo (basso senza display: pompa inibita); 1k: I_INH(H) fino a 2 x 93 uA a 3,3 V")
R("R85", "1.5k", "PUMP_IS_A", "GND", S, "Sense IS lato A: kILIS 10000 -> 6 A = 0,8 mA = 1,2 V (10 A = 1,8 V)")
C("C54", "1nF", "PUMP_IS_A", "GND", S)
R("R88", "1.5k", "PUMP_IS_B", "GND", S, "Sense IS lato B")
C("C61", "1nF", "PUMP_IS_B", "GND", S)
CP("C55", "470uF 25V", "+12V_PUMP", "GND", S, "Capacitor_THT:CP_Radial_D10.0mm_P5.00mm", "25ZLH470MEFC10X12.5",
   "Bulk ponte pompa (ZLH basso ESR, 1,33 A ripple)", mfr="Rubycon", lcsc="C338188")
CP("C56", "470uF 25V", "+12V_PUMP", "GND", S, "Capacitor_THT:CP_Radial_D10.0mm_P5.00mm", "25ZLH470MEFC10X12.5",
   "Bulk ponte pompa (ZLH basso ESR, 1,33 A ripple)", mfr="Rubycon", lcsc="C338188")
C("C57", "10uF 50V", "+12V_PUMP", "GND", S, FP_C1206, desc="Ceramico ponte pompa")
P("J25", "Connector_Generic:Conn_01x02", "PUMP", TB2_508,
  {"1": "PUMP_OUT_A", "2": "PUMP_OUT_B"}, S, "WJ500V-5.08-2P", "Pompa Marco UP3-R", mfr="Ningbo Kangnex", lcsc="C8465")

# Motore spirale JGB37-520 (0,2 A, ~1,5 A bloccato): TI DRV8871, limite 2,1 A
P("U10", "Driver_Motor:DRV8871DDA", "DRV8871DDA", "Package_SO:TI_SO-PowerPAD-8_ThermalVias",
  {"5": "+12V", "1": "GND", "7": "GND", "9": "GND", "3": "MOT_A_CTL", "2": "MOT_B_CTL",
   "6": "MOT_OUT_A", "8": "MOT_OUT_B", "4": "MOT_ILIM"}, S,
  "DRV8871DDAR", "Ponte H motore spirale 3,6 A picco", mfr="Texas Instruments", lcsc="C75864")
R("R86", "30k", "MOT_ILIM", "GND", S, "ILIM: 64 kV / 30k = 2,1 A")
C("C58", "100nF 50V", "+12V", "GND", S, FP_C0805)
C("C59", "22uF 25V", "+12V", "GND", S, FP_C1206, desc="Bulk VM motore")
P("J26", "Connector_Generic:Conn_01x02", "MOTOR", TB2_35,
  {"1": "MOT_OUT_A", "2": "MOT_OUT_B"}, S, "DB301V-3.5-2P-GN-S", "Motore spirale JGB37-520", mfr="DORABO", lcsc="C695629")

# Servo taglierina MG90S
R("R87", "220R", "SERVO_PWM", "SERVO_SIG", S, "Serie segnale servo")
P("J27", "Connector_Generic:Conn_01x03", "SERVO", "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical",
  {"1": "SERVO_SIG", "2": "+5V", "3": "GND"}, S, "PZ254V-11-03P", "Servo MG90S (S, +5V, GND)", mfr="XFCN", lcsc="C2937625")
C("C60", "47uF 10V", "+5V", "GND", S, FP_C1206, desc="Riserva per i picchi del servo")

SHEETS = [
    ("power", "Alimentazione"),
    ("io", "Display, I/O e sensori"),
    ("outputs", "Valvole e riscaldatori"),
    ("drives", "Pompa, motore, servo"),
]


def nets():
    d = {}
    for p in PARTS:
        for pin, n in p["pins"].items():
            if n is None:
                continue
            d.setdefault(n, []).append((p["ref"], pin))
    return d


if __name__ == "__main__":
    import collections
    refs = [p["ref"] for p in PARTS]
    dup = [r for r, c in collections.Counter(refs).items() if c > 1]
    print("parts:", len(PARTS), "dups:", dup)
    nn = nets()
    print("nets:", len(nn))
    for n, l in sorted(nn.items()):
        if len(l) < 2:
            print("  SINGLE-PIN NET:", n, l)
    missing = [p["ref"] for p in PARTS if not p["lcsc"] and not p["dnp"] and not p["sym"].startswith("Mechanical")]
    print("no LCSC:", missing)
