# -*- coding: utf-8 -*-
"""
FilMachine Mainboard v1 — single source of truth for schematic + PCB.

Every part:  P(ref, symbol, value, footprint, pins={pin: net}, sheet, mpn, desc, dnp)
Pin keys are symbol pin numbers (strings). Nets are plain strings.
"""

PARTS = []
NC = None     # pin explicitly not connected (no-connect flag in schematic)

FP_R = "Resistor_SMD:R_0603_1608Metric"
FP_C = "Capacitor_SMD:C_0603_1608Metric"
FP_C0805 = "Capacitor_SMD:C_0805_2012Metric"
FP_C1206 = "Capacitor_SMD:C_1206_3216Metric"
FP_C1210 = "Capacitor_SMD:C_1210_3225Metric"
FP_LED = "LED_SMD:LED_0603_1608Metric"


def P(ref, sym, value, fp, pins, sheet, mpn="", desc="", dnp=False, mfr="", unit_pins=None):
    PARTS.append(dict(ref=ref, sym=sym, value=value, fp=fp, pins=pins, sheet=sheet,
                      mpn=mpn, desc=desc, dnp=dnp, mfr=mfr))


def R(ref, val, a, b, sheet, desc="", dnp=False, fp=FP_R, mpn=None):
    P(ref, "Device:R", val, fp, {"1": a, "2": b}, sheet,
      mpn or f"0603 {val} 1%", desc, dnp, mfr="Yageo/UniOhm (generic)")


def C(ref, val, a, b, sheet, fp=FP_C, mpn=None, desc="", dnp=False):
    P(ref, "Device:C", val, fp, {"1": a, "2": b}, sheet,
      mpn or f"MLCC {val}", desc, dnp, mfr="Samsung/Murata (generic)")


def CP(ref, val, a, b, sheet, fp, mpn, desc=""):
    P(ref, "Device:C_Polarized", val, fp, {"1": a, "2": b}, sheet, mpn, desc, mfr="Panasonic")


def LED(ref, anode, cathode, sheet, color="Green", desc=""):
    mpn = {"Green": "KT-0603G", "Red": "KT-0603R", "Yellow": "KT-0603Y", "Blue": "KT-0603B"}[color]
    P(ref, "Device:LED", color, FP_LED, {"1": cathode, "2": anode}, sheet, mpn, desc, mfr="Hubei KENTO")


# ════════════════════════════════════════════════════════════════════════════
# SHEET 1 — POWER
# ════════════════════════════════════════════════════════════════════════════
S = "power"
P("J1", "Connector_Generic:Conn_01x02", "12V HEAT IN",
  "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-3-2-5.08_1x02_P5.08mm_Horizontal",
  {"1": "GND", "2": "+12V_IN_H"}, S, "1711026", "Ingresso 12 V riscaldatori (da LRS-350, coppia +V/-V n.1), 24 A (pin 2 = +)", mfr="Phoenix Contact")
P("J2", "Connector_Generic:Conn_01x02", "12V MAIN IN",
  "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-3-2-5.08_1x02_P5.08mm_Horizontal",
  {"1": "GND", "2": "+12V_IN_M"}, S, "1711026", "Ingresso 12 V pompa/logica (da LRS-350, coppia +V/-V n.2), 24 A (pin 2 = +)", mfr="Phoenix Contact")

FUSEHOLDER = "Fuse:Fuseholder_Blade_Mini_Keystone_3568"
P("F1", "Device:Fuse", "15A MINI", FUSEHOLDER, {"1": "+12V_IN_H", "2": "+12V_H1"}, S,
  "3568", "Portafusibile mini-blade + fusibile 15 A inserito (voce a parte) — riscaldatore 1", mfr="Keystone")
P("F2", "Device:Fuse", "15A MINI", FUSEHOLDER, {"1": "+12V_IN_H", "2": "+12V_H2"}, S,
  "3568", "Portafusibile mini-blade + fusibile 15 A inserito — riscaldatore 2", mfr="Keystone")
P("F3", "Device:Fuse", "15A MINI", FUSEHOLDER, {"1": "+12V_IN_M", "2": "+12V_PUMP"}, S,
  "3568", "Portafusibile mini-blade + fusibile 15 A inserito — pompa", mfr="Keystone")
P("F4", "Device:Fuse", "7.5A MINI", FUSEHOLDER, {"1": "+12V_IN_M", "2": "+12V"}, S,
  "3568", "Portafusibile mini-blade + fusibile 7,5 A inserito — valvole, motore, logica", mfr="Keystone")

P("D1", "Device:D_Zener", "SMCJ16A", "Diode_SMD:D_SMC", {"1": "+12V", "2": "GND"}, S,
  "SMCJ16A", "TVS 16 V: transitori + crowbar inversione polarità (brucia F4)", mfr="Littelfuse")
P("D2", "Device:D_Zener", "SMCJ16A", "Diode_SMD:D_SMC", {"1": "+12V_PUMP", "2": "GND"}, S,
  "SMCJ16A", "TVS 16 V sul rail pompa (energia di ritorno del motore)", mfr="Littelfuse")

CP("C1", "100uF 25V", "+12V", "GND", S, "Capacitor_THT:CP_Radial_D8.0mm_P3.50mm",
   "EEU-FR1E101", "Bulk rail +12V logica/valvole")

# 12 V -> 5 V / 3 A buck (TI LMR33630ADDA, 400 kHz, valori da Tab. 9-2 del datasheet)
P("U1", "Regulator_Switching:LMR33630ADDA", "LMR33630ADDA", "Package_SO:TI_SO-PowerPAD-8_ThermalVias",
  {"2": "+12V", "3": "+12V", "6": "BUCK_VCC", "1": "GND", "9": "GND",
   "7": "BUCK_BOOT", "8": "BUCK_SW", "5": "BUCK_FB", "4": NC}, S,
  "LMR33630ADDAR", "Buck sincrono 3,8–36 V -> 5 V 3 A", mfr="Texas Instruments")
C("C2", "10uF 50V", "+12V", "GND", S, FP_C1210, "GRM32ER71H106KA12L", "Cin buck")
C("C3", "10uF 50V", "+12V", "GND", S, FP_C1210, "GRM32ER71H106KA12L", "Cin buck")
C("C4", "220nF 50V", "+12V", "GND", S, FP_C, "CL10B224KB8NNNC", "Cin HF buck")
C("C5", "100nF", "BUCK_BOOT", "BUCK_SW", S, desc="Bootstrap")
C("C6", "1uF 16V", "BUCK_VCC", "GND", S, desc="VCC interno buck")
P("L1", "Device:L", "6.8uH", "Inductor_SMD:L_Bourns_SRP7028A_7.3x6.6mm", {"1": "BUCK_SW", "2": "+5V"}, S,
  "SRP7028A-6R8M", "Induttore buck 6,8 uH (Isat > 5 A)", mfr="Bourns")
R("R1", "100k", "+5V", "BUCK_FB", S, "Partitore FB (Vout = 1 V x (1+100k/24.3k) = 5.12 V: margine per USB-C e display)")
R("R2", "24.3k", "BUCK_FB", "GND", S, "Partitore FB")
for i, ref in enumerate(["C7", "C8", "C9", "C10"]):
    C(ref, "22uF 16V", "+5V", "GND", S, FP_C1206, "CL31A226KOHNNNE", "Cout buck")

# 5 V -> 3.3 V LDO (logica scheda: MCP23017, MCP3424, porte AND, DS18B20)
P("U2", "Regulator_Linear:AP2112K-3.3", "AP2112K-3.3", "Package_TO_SOT_SMD:SOT-23-5",
  {"1": "+5V", "3": "+5V", "2": "GND", "4": NC, "5": "+3V3"}, S,
  "AP2112K-3.3TRG1", "LDO 3,3 V 600 mA", mfr="Diodes Inc.")
C("C11", "1uF 16V", "+5V", "GND", S)
C("C12", "2.2uF 16V", "+3V3", "GND", S)

# Uscite 5 V protette
P("F5", "Device:Polyfuse", "1.5A", "Fuse:Fuse_1812_4532Metric", {"1": "+5V", "2": "+5V_USB"}, S,
  "1812L150/12DR", "PTC 1,5 A uscita USB-C 5 V di servizio", mfr="Littelfuse")
P("F6", "Device:Polyfuse", "0.5A", "Fuse:Fuse_1206_3216Metric", {"1": "+5V", "2": "+5V_SENS"}, S,
  "1206L050YR", "PTC 0,5 A alimentazione sensori", mfr="Littelfuse")
# USB-C di servizio: SOLO USCITA 5 V (nessun dato) per alimentare un eventuale accessorio.
# Sorgente "cold socket" come vuole la specifica Type-C: il VBUS si accende solo quando il dispositivo collegato mette
# il suo Rd (5,1k) su una CC. Tutto (Rp, rilevatore, comparatore) e' alimentato dal nodo comune USBC_S dei due P-MOS in
# antiserie: con la scheda spenta e un alimentatore esterno sul connettore (cavo A->C) USBC_S sale solo tramite il diodo
# di Q12, il gate resta a USBC_S e la porta resta bloccata nei due versi.
# Rilevamento: min(CC1, CC2) + Vf (D45/D46) confrontato con una soglia raziometrica 0,43 x USBC_S (U11 TLV7041):
#   Rd 5,1k presente -> CC = 0,08 x S (~0,4 V) -> sotto soglia -> uscita a massa -> VBUS acceso (soft-start ~1 ms);
#   niente collegato, o un'altra sorgente con il suo Rp (5 V o 3,3 V) -> CC > 3,3 V -> VBUS spento.
# Rp 56k = "Default USB power" (500/900 mA): con 1,5 A la caduta su PTC + 2 MOS porterebbe il VBUS sotto 4,75 V.
P("J3", "Connector:USB_C_Receptacle_PowerOnly_6P", "USB-C 5V OUT",
  "Connector_USB:USB_C_Receptacle_GCT_USB4125-xx-x_6P_TopMnt_Horizontal",
  {"A9": "VBUS_C", "B9": "VBUS_C", "A12": "GND", "B12": "GND", "A5": "USBC_CC1", "B5": "USBC_CC2", "S1": "GND"}, S,
  "USB4125-GF-A", "USB-C solo alimentazione (6 poli), uscita 5 V (Default USB power)", mfr="GCT")
R("R89", "56k", "USBC_S", "USBC_CC1", S, "Rp CC1: Default USB power")
R("R90", "56k", "USBC_S", "USBC_CC2", S, "Rp CC2: Default USB power")
P("D45", "Diode:1N4148WS", "1N4148WS", "Diode_SMD:D_SOD-323", {"1": "USBC_CC1", "2": "USBC_CCMIN"}, S,
  "1N4148WS-7-F", "OR della CC piu' bassa", mfr="Diodes Inc")
P("D46", "Diode:1N4148WS", "1N4148WS", "Diode_SMD:D_SOD-323", {"1": "USBC_CC2", "2": "USBC_CCMIN"}, S,
  "1N4148WS-7-F", "OR della CC piu' bassa", mfr="Diodes Inc")
R("R91", "220k", "USBC_S", "USBC_CCMIN", S, "Polarizzazione dell'OR delle CC")
R("R93", "130k", "USBC_S", "USBC_REF", S, "Soglia raziometrica 0,43 x USBC_S")
R("R94", "100k", "USBC_REF", "GND", S, "Soglia raziometrica")
P("U11", "Comparator:TLV7041DBV", "TLV7041", "Package_TO_SOT_SMD:SOT-23-5",
  {"1": "USBC_DRV", "2": "GND", "3": "USBC_CCMIN", "4": "USBC_REF", "5": "USBC_S"}, S,
  "TLV7041DBVR", "Comparatore open-drain: Rd rilevato -> accende il VBUS", mfr="Texas Instruments")
C("C65", "100nF", "USBC_S", "GND", S, desc="Disaccoppiamento U11")
R("R96", "10k", "USBC_DRV", "USBC_GATE", S, "Soft-start del VBUS (con C64)")
R("R95", "100k", "USBC_S", "USBC_GATE", S, "Gate a USBC_S = VBUS spento")
C("C64", "100nF", "USBC_GATE", "USBC_S", S, desc="Soft-start: ~1 ms in accensione, ~10 ms in spegnimento")
P("Q11", "Transistor_FET:AO3401A", "AO3401A", "Package_TO_SOT_SMD:SOT-23",
  {"1": "USBC_GATE", "2": "USBC_S", "3": "+5V_USB"}, S, "AO3401A", "Interruttore VBUS lato 5 V (P-MOS, antiserie con Q12)", mfr="Alpha & Omega")
P("Q12", "Transistor_FET:AO3401A", "AO3401A", "Package_TO_SOT_SMD:SOT-23",
  {"1": "USBC_GATE", "2": "USBC_S", "3": "VBUS_C"}, S, "AO3401A", "Interruttore VBUS lato connettore (blocca il ritorno da fuori)", mfr="Alpha & Omega")
R("R92", "10k", "VBUS_C", "GND", S, "Scarica il VBUS quando si stacca il dispositivo")
C("C63", "4.7uF 16V", "VBUS_C", "GND", S, fp=FP_C0805, desc="Disaccoppiamento VBUS USB-C")

# LED di stato alimentazioni
R("R3", "4.7k", "+12V", "LED12_A", S, "LED 12V")
LED("D3", "LED12_A", "GND", S, "Green", "12 V logica presente")
R("R4", "1k", "+5V", "LED5_A", S, "LED 5V")
LED("D4", "LED5_A", "GND", S, "Green", "5 V presente")
R("R5", "1k", "+3V3", "LED33_A", S, "LED 3V3")
LED("D5", "LED33_A", "GND", S, "Green", "3,3 V presente")

for i, ref in enumerate(["H1", "H2", "H3", "H4"]):
    P(ref, "Mechanical:MountingHole", "M3", "MountingHole:MountingHole_3.2mm_M3", {}, S,
      "", "Foro di fissaggio M3")

# ════════════════════════════════════════════════════════════════════════════
# SHEET 2 — DISPLAY LINK, I/O EXPANDER, ADC, SENSOR INPUTS
# ════════════════════════════════════════════════════════════════════════════
S = "io"
# JP1 del JC4880P433 (schema Guition, "Expand IO"): verificato pin per pin
JP1_MAP = {
    "1": "3V3_DISP", "2": "5V_DISP", "3": "3V3_DISP", "4": "5V_DISP",
    "5": "GND", "6": "GND",
    "7": "FLOW_IN",      # GPIO52
    "8": "MOT_IN1",      # GPIO33
    "9": "PUMP_ENA",     # GPIO51
    "10": "HALL_IN",     # GPIO31
    "11": "PUMP_IN2",    # GPIO50
    "12": "BATH_MAX_IN", # GPIO30
    "13": "PUMP_IN1",    # GPIO49
    "14": "BATH_MIN_IN", # GPIO29
    "15": "ONEWIRE",     # GPIO35 (strapping: deve essere alto al boot -> pull-up 4k7)
    "16": "GND",
    "17": "MOT_IN2",     # GPIO34
    "18": "3V3_DISP",
    "19": "MOT_ENA",     # GPIO32
    "20": "C6_RXD",
    "21": "SERVO_PWM",   # GPIO28
    "22": "C6_TXD",
    "23": "I2C_SDA",     # ES_I2C_SDA (GPIO7) — pull-up 2k2 gia' sul display
    "24": "C6_IO9",
    "25": "I2C_SCL",     # ES_I2C_SCL (GPIO8)
    "26": "C6_PU",
}
P("J4", "Connector_Generic:Conn_02x13_Odd_Even", "DISPLAY JP1", "Connector_IDC:IDC-Header_2x13_P2.54mm_Horizontal",
  JP1_MAP, S, "AWHW-26G-0202-T-R", "Box header 2x13 ad angolo retto (cavo verso il bordo) per flat verso JP1 del display JC4880P433", mfr="Assmann WSW")
# Alimentazione del display dal flat (JP1 pin 2/4 = VCC5V del JC4880P433, a valle dell'IP5306 tramite R22 1 A).
# PTC dedicato + Schottky in serie: (1) il display non puo' mai alimentare all'indietro il 5 V della scheda;
# (2) con la USB-C del display collegata al PC, il VBUS del PC (5 V) sta ~0,4 V sopra il nostro (4,6 V) e vince:
#     nessuna corrente verso il PC, nessun conflitto tra le due alimentazioni. Display ~320 mA (datasheet Guition).
P("F7", "Device:Polyfuse", "1.1A", "Fuse:Fuse_1812_4532Metric", {"1": "+5V", "2": "5V_DISP_F"}, S,
  "MF-MSMF110-2", "PTC 1,1 A alimentazione display via JP1", mfr="Bourns")
P("D8", "Device:D_Schottky", "B340A", "Diode_SMD:D_SMA", {"1": "5V_DISP", "2": "5V_DISP_F"}, S,
  "B340A-13-F", "Schottky 3 A 40 V: blocca il ritorno dal display (USB-C) verso il 5 V della scheda", mfr="Diodes Inc")
C("C62", "10uF 16V", "5V_DISP", "GND", S, fp=FP_C0805, desc="Bulk sul 5 V verso il display")
P("J5", "Connector_Generic:Conn_01x05", "C6 UART", "Connector_PinHeader_2.54mm:PinHeader_1x05_P2.54mm_Vertical",
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
  "MCP23017-E/SS", "Espansore I2C 16 I/O, indirizzo 0x20", mfr="Microchip")
C("C13", "100nF", "+3V3", "GND", S, desc="Disaccoppiamento MCP23017")
R("R8", "10k", "MCP_RST", "+3V3", S, "Pull-up RESET MCP23017")
C("C14", "100nF", "MCP_RST", "GND", S, desc="Power-on reset MCP23017")

# MCP3424 @0x68 — monitor tensioni (HAS_RAIL_MONITOR) + corrente pompa
P("U4", "Analog_ADC:MCP3424x-xSL", "MCP3424-E/SL", "Package_SO:SOIC-14_3.9x8.7mm_P1.27mm",
  {"6": "+3V3", "5": "GND", "7": "I2C_SDA", "8": "I2C_SCL", "9": "GND", "10": "GND",
   "1": "ADC_12V", "2": "GND", "3": "ADC_5V", "4": "GND",
   "11": "ADC_PUMP_IA", "12": "GND", "13": "ADC_PUMP_IB", "14": "GND"}, S,
  "MCP3424-E/SL", "ADC I2C 18 bit @0x68: CH1 12 V, CH2 5 V, CH3/CH4 corrente pompa lato A/B", mfr="Microchip")
C("C15", "100nF", "+3V3", "GND", S, desc="Disaccoppiamento MCP3424")
R("R9", "100k", "+12V", "ADC_12V", S, "Partitore 12 V: x 15/115 (12 V -> 1,57 V)")
R("R10", "15k", "ADC_12V", "GND", S, "Partitore 12 V")
C("C16", "100nF", "ADC_12V", "GND", S)
R("R11", "20k", "+5V", "ADC_5V", S, "Partitore 5 V: x 1/3")
R("R12", "10k", "ADC_5V", "GND", S, "Partitore 5 V")
C("C17", "100nF", "ADC_5V", "GND", S)
for side, rs, cs, ds in (("A", "R13", "C18", "D6"), ("B", "R14", "C19", "D7")):
    R(rs, "10k", f"PUMP_IS_{side}", f"ADC_PUMP_I{side}", S, f"Serie IS lato {side} verso ADC (limita la corrente di fault)")
    C(cs, "100nF", f"ADC_PUMP_I{side}", "GND", S)
    P(ds, "Device:D_Schottky", "BAT54WS", "Diode_SMD:D_SOD-323", {"1": "+3V3", "2": f"ADC_PUMP_I{side}"}, S,
      "BAT54WS", f"Clamp ingresso ADC lato {side} a 3,3 V", mfr="Nexperia/Diodes")

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
    P(f"D{10+i}", "Device:D_Schottky", "BAT54WS", "Diode_SMD:D_SOD-323", {"1": sns, "2": mcu}, S,
      "BAT54WS", f"Adattatore livello ingresso {ch}", mfr="Nexperia/Diodes")
    R(f"R{20+i}", "10k", mcu, "+3V3", S, f"Pull-up ingresso {ch}")
    C(f"C{20+i}", "1nF", mcu, "GND", S, desc=f"Filtro ingresso {ch}")

XH4 = "Connector_JST:JST_XH_B4B-XH-A_1x04_P2.50mm_Vertical"
XH3 = "Connector_JST:JST_XH_B3B-XH-A_1x03_P2.50mm_Vertical"
P("J6", "Connector_Generic:Conn_01x04", "BATH LEVEL", XH4,
  {"1": "+5V_SENS", "2": "SNS_BATH_MIN", "3": "SNS_BATH_MAX", "4": "GND"}, S,
  "B4B-XH-A(LF)(SN)", "XKC-Y21 bagno MIN/MAX (5V, MIN, MAX, GND)", mfr="JST")
P("J7", "Connector_Generic:Conn_01x04", "CHEM 1", XH4,
  {"1": "+5V_SENS", "2": "SNS_C1_MIN", "3": "SNS_C1_MAX", "4": "GND"}, S,
  "B4B-XH-A(LF)(SN)", "XKC-Y21 chimica 1 MIN/MAX", mfr="JST")
P("J8", "Connector_Generic:Conn_01x04", "CHEM 2", XH4,
  {"1": "+5V_SENS", "2": "SNS_C2_MIN", "3": "SNS_C2_MAX", "4": "GND"}, S,
  "B4B-XH-A(LF)(SN)", "XKC-Y21 chimica 2 MIN/MAX", mfr="JST")
P("J9", "Connector_Generic:Conn_01x04", "CHEM 3", XH4,
  {"1": "+5V_SENS", "2": "SNS_C3_MIN", "3": "SNS_C3_MAX", "4": "GND"}, S,
  "B4B-XH-A(LF)(SN)", "XKC-Y21 chimica 3 MIN/MAX", mfr="JST")
P("J10", "Connector_Generic:Conn_01x03", "FLOW", XH3,
  {"1": "+5V_SENS", "2": "SNS_FLOW", "3": "GND"}, S, "B3B-XH-A(LF)(SN)", "Flussimetro YF-S201", mfr="JST")
P("J11", "Connector_Generic:Conn_01x03", "HALL", XH3,
  {"1": "+5V_SENS", "2": "SNS_HALL", "3": "GND"}, S, "B3B-XH-A(LF)(SN)", "Sensore Hall KY-003", mfr="JST")
P("J12", "Connector_Generic:Conn_01x03", "TEMP 1", XH3,
  {"1": "+3V3", "2": "ONEWIRE", "3": "GND"}, S, "B3B-XH-A(LF)(SN)", "DS18B20 bagno", mfr="JST")
P("J13", "Connector_Generic:Conn_01x03", "TEMP 2", XH3,
  {"1": "+3V3", "2": "ONEWIRE", "3": "GND"}, S, "B3B-XH-A(LF)(SN)", "DS18B20 chimica", mfr="JST")
R("R16", "4.7k", "ONEWIRE", "3V3_DISP", S, "Pull-up OneWire ai 3,3 V del display: GPIO35 alto al boot anche a scheda spenta")

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
    R(f"R{30+n}", "100R", sig, gate, S, f"Gate valvola {name}")
    R(f"R{40+n}", "100k", gate, "GND", S, f"Pull-down gate valvola {name} (spenta al boot)")
    P(f"Q{n}", "Transistor_FET:AO3400A", "AO3400A", "Package_TO_SOT_SMD:SOT-23",
      {"1": gate, "2": "GND", "3": drain}, S, "AO3400A", f"Low-side valvola {name} (30 V, 5,7 A)", mfr="Alpha & Omega")
    P(f"D{20+n}", "Device:D_Schottky", "B5819W", "Diode_SMD:D_SOD-123", {"1": "+12V", "2": drain}, S,
      "B5819W", f"Ricircolo bobina valvola {name}", mfr="Diodes/CJ")
    R(f"R{50+n}", "4.7k", "+12V", f"VLV{n}_LED", S, f"LED valvola {name}")
    LED(f"D{30+n}", f"VLV{n}_LED", drain, S, "Yellow", f"Valvola {name} aperta")

PT4 = "TerminalBlock_Phoenix:TerminalBlock_Phoenix_PT-1,5-4-3.5-H_1x04_P3.50mm_Horizontal"
P("J14", "Connector_Generic:Conn_01x04", "VALVES C1 C2", PT4,
  {"1": "+12V", "2": "VLV1_N", "3": "+12V", "4": "VLV2_N"}, S, "1984888", "Valvole C1, C2 (+, -, +, -)", mfr="Phoenix Contact")
P("J15", "Connector_Generic:Conn_01x04", "VALVES C3 WB", PT4,
  {"1": "+12V", "2": "VLV3_N", "3": "+12V", "4": "VLV4_N"}, S, "1984888", "Valvole C3, WB", mfr="Phoenix Contact")
P("J16", "Connector_Generic:Conn_01x04", "VALVES WASTE INLET", PT4,
  {"1": "+12V", "2": "VLV5_N", "3": "+12V", "4": "VLV6_N"}, S, "1984888", "Valvole scarico, ingresso acqua", mfr="Phoenix Contact")
P("J17", "Connector_Generic:Conn_01x04", "VALVES SPARE", PT4,
  {"1": "+12V", "2": "VLV7_N", "3": "+12V", "4": "VLV8_N"}, S, "1984888", "Uscite di riserva A6, A7", mfr="Phoenix Contact")

# Riscaldatori 2 x 100 W (8,3 A): gate driver a 12 V + MOSFET 40 V / 7 mOhm
P("U5", "Driver_FET:UCC27524D", "UCC27524D", "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm",
  {"6": "+12V", "3": "GND", "2": "HEAT1_CMD", "4": "HEAT2_CMD", "1": NC, "8": NC,
   "7": "HEAT1_DRV", "5": "HEAT2_DRV"}, S,
  "UCC27524DR", "Gate driver doppio low-side (ingressi 3,3 V, uscite 12 V); EN interni a pull-up", mfr="Texas Instruments")
C("C40", "1uF 25V", "+12V", "GND", S, FP_C0805, desc="Disaccoppiamento gate driver")
C("C41", "100nF", "+12V", "GND", S, desc="Disaccoppiamento gate driver")
R("R61", "100k", "HEAT1_CMD", "GND", S, "Pull-down comando riscaldatore 1")
R("R62", "100k", "HEAT2_CMD", "GND", S, "Pull-down comando riscaldatore 2")
for n in (1, 2):
    q = f"Q{8+n}"
    R(f"R{62+n*2-1}", "10R", f"HEAT{n}_DRV", f"HEAT{n}_G", S, f"Gate riscaldatore {n}")
    R(f"R{62+n*2}", "10k", f"HEAT{n}_G", "GND", S, f"Pull-down gate riscaldatore {n}")
    P(q, "Transistor_FET:IPD50R380CE", "AOD4184A", "Package_TO_SOT_SMD:TO-252-2",
      {"1": f"HEAT{n}_G", "2": f"HEAT{n}_N", "3": "GND"}, S,
      "AOD4184A", f"Low-side riscaldatore {n} (40 V, 50 A, 7 mOhm @10 V)", mfr="Alpha & Omega")
    P(f"D{40+n}", "Device:D_Schottky", "SS34", "Diode_SMD:D_SMA", {"1": f"+12V_H{n}", "2": f"HEAT{n}_N"}, S,
      "SS34", f"Ricircolo induttanza cavi riscaldatore {n}", mfr="MDD/onsemi")
    R(f"R{66+n}", "4.7k", f"HEAT{n}_DRV", f"HEAT{n}_LED", S, f"LED riscaldatore {n} (dal gate driver, 12 V = acceso)")
    LED(f"D{42+n}", f"HEAT{n}_LED", "GND", S, "Red", f"Riscaldatore {n} comandato acceso")
    P(f"J{17+n}", "Connector_Generic:Conn_01x02", f"HEATER {n}",
      "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-3-2-5.08_1x02_P5.08mm_Horizontal",
      {"1": f"HEAT{n}_N", "2": f"+12V_H{n}"}, S, "1711026", f"Riscaldatore {n} 100 W 12 V (pin 2 = +, pin 1 = -)", mfr="Phoenix Contact")

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
   "8": "+3V3", "4": "GND"}, S, "SN74LVC2G08DCTR", "AND: IN1·ENA, IN2·ENA pompa", mfr="Texas Instruments")
C("C50", "100nF", "+3V3", "GND", S)
P("U7", "74xGxx:74LVC2G08", "74LVC2G08", "Package_SO:SSOP-8_2.95x2.8mm_P0.65mm",
  {"1": "MOT_IN1", "2": "MOT_ENA", "7": "MOT_A_CTL",
   "5": "MOT_IN2", "6": "MOT_ENA", "3": "MOT_B_CTL",
   "8": "+3V3", "4": "GND"}, S, "SN74LVC2G08DCTR", "AND: IN1·ENA, IN2·ENA motore", mfr="Texas Instruments")
C("C51", "100nF", "+3V3", "GND", S)

# Ponte H pompa (UP3-R, fino a 10 A): 2 x Infineon BTN8982TA
for n, side in ((8, "A"), (9, "B")):
    P(f"U{n}", "Power_Management:BTN8982TA", "BTN8982TA", "Package_TO_SOT_SMD:TO-263-7_TabPin8",
      {"7": "+12V_PUMP", "1": "GND", "2": f"PUMP_{side}_IN", "3": "PUMP_INH", "6": f"PUMP_IS_{side}",
       "5": "GND", "4": f"PUMP_OUT_{side}", "8": f"PUMP_OUT_{side}"}, S,
      "BTN8982TAAUMA1", f"Mezzo ponte pompa lato {side} (30 A, 10 mOhm); SR a GND = slew rate massimo", mfr="Infineon")
    R(f"R{80 if side=='A' else 82}", "10k", f"PUMP_{side}_CTL", f"PUMP_{side}_IN", S,
      f"Serie ingresso IN (datasheet: > 7 kOhm)")
    C(f"C{52 if side=='A' else 53}", "100nF 50V", "+12V_PUMP", "GND", S, FP_C0805, desc="VS locale")
R("R84", "10k", "+3V3", "PUMP_INH", S, "INH alto = ponte attivo")
R("R85", "1.5k", "PUMP_IS_A", "GND", S, "Sense IS lato A: 10 A -> ~0,8 V (peggior caso < 2,048 V)")
C("C54", "1nF", "PUMP_IS_A", "GND", S)
R("R88", "1.5k", "PUMP_IS_B", "GND", S, "Sense IS lato B: 10 A -> ~0,8 V (peggior caso < 2,048 V)")
C("C61", "1nF", "PUMP_IS_B", "GND", S)
CP("C55", "470uF 25V", "+12V_PUMP", "GND", S, "Capacitor_THT:CP_Radial_D10.0mm_P5.00mm", "EEU-FR1E471", "Bulk ponte pompa (basso ESR)")
CP("C56", "470uF 25V", "+12V_PUMP", "GND", S, "Capacitor_THT:CP_Radial_D10.0mm_P5.00mm", "EEU-FR1E471", "Bulk ponte pompa (basso ESR)")
C("C57", "10uF 50V", "+12V_PUMP", "GND", S, FP_C1210, "GRM32ER71H106KA12L", "Ceramico ponte pompa")
P("J20", "Connector_Generic:Conn_01x02", "PUMP",
  "TerminalBlock_Phoenix:TerminalBlock_Phoenix_MKDS-3-2-5.08_1x02_P5.08mm_Horizontal",
  {"1": "PUMP_OUT_A", "2": "PUMP_OUT_B"}, S, "1711026", "Pompa Marco UP3-R", mfr="Phoenix Contact")

# Motore spirale JGB37-520 (0,2 A, ~1,5 A bloccato): TI DRV8871, limite 2,1 A
P("U10", "Driver_Motor:DRV8871DDA", "DRV8871DDA", "Package_SO:TI_SO-PowerPAD-8_ThermalVias",
  {"5": "+12V", "1": "GND", "7": "GND", "9": "GND", "3": "MOT_A_CTL", "2": "MOT_B_CTL",
   "6": "MOT_OUT_A", "8": "MOT_OUT_B", "4": "MOT_ILIM"}, S,
  "DRV8871DDAR", "Ponte H motore spirale 3,6 A picco", mfr="Texas Instruments")
R("R86", "30k", "MOT_ILIM", "GND", S, "ILIM: 64 kV / 30k = 2,1 A")
C("C58", "100nF 50V", "+12V", "GND", S, FP_C0805)
C("C59", "22uF 25V", "+12V", "GND", S, FP_C1210, "GRM32ER71E226KE15L", "Bulk VM motore")
P("J21", "Connector_Generic:Conn_01x02", "MOTOR",
  "TerminalBlock_Phoenix:TerminalBlock_Phoenix_PT-1,5-2-3.5-H_1x02_P3.50mm_Horizontal",
  {"1": "MOT_OUT_A", "2": "MOT_OUT_B"}, S, "1984617", "Motore spirale JGB37-520", mfr="Phoenix Contact")

# Servo taglierina MG90S
R("R87", "220R", "SERVO_PWM", "SERVO_SIG", S, "Serie segnale servo")
P("J22", "Connector_Generic:Conn_01x03", "SERVO", "Connector_PinHeader_2.54mm:PinHeader_1x03_P2.54mm_Vertical",
  {"1": "SERVO_SIG", "2": "+5V", "3": "GND"}, S, "PZ254V-11-03P", "Servo MG90S (S, +5V, GND)", mfr="XFCN")
C("C60", "100uF 10V", "+5V", "GND", S, FP_C1210, "GRM32ER61A107ME20L", "Riserva per i picchi del servo")

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
