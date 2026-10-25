"""Create a deterministic vector overlay; the embedded original photo is unchanged."""
from pathlib import Path
import base64
import html
import json

root = Path(__file__).resolve().parents[1]
photo = root / 'docs/source-photo.png'
encoded = base64.b64encode(photo.read_bytes()).decode()
parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="2600" height="2460" viewBox="0 0 2600 2460"><rect width="2600" height="2460" fill="white"/><image x="0" y="120" width="2600" height="1650" href="data:image/png;base64,{encoded}"/>']

def text(x, y, value, size=25, color='#132b40', weight='normal'):
    parts.append(f'<text x="{x}" y="{y}" font-family="DejaVu Sans,sans-serif" font-size="{size}" fill="{color}" font-weight="{weight}">{html.escape(value)}</text>')

def box(x, y, w, h, fill='#ffffff', stroke='#a8b5c3'):
    parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="{fill}" stroke="{stroke}" stroke-width="2"/>')

def wire(points, color, dashed=False):
    d = ' '.join(f'{x},{y}' for x, y in points)
    dash = ' stroke-dasharray="12 9"' if dashed else ''
    parts.append(f'<polyline points="{d}" fill="none" stroke="white" stroke-width="10" stroke-linejoin="round"/>')
    parts.append(f'<polyline points="{d}" fill="none" stroke="{color}" stroke-width="5" stroke-linejoin="round"{dash}/>')
    for x, y in [points[0], points[-1]]:
        parts.append(f'<circle cx="{x}" cy="{y}" r="5" fill="{color}" stroke="white"/>')

text(40, 48, 'CrowPanel Advanced 10.1 V1.2 — module wiring', 38, weight='bold')
text(40, 88, 'Exact net labels govern wiring. GPS supply is conditional. Power off before assembly.', 26, '#9a4d00')
# Sources on J7: rear view as photographed; even row toward board, odd row toward bottom edge.
xs = [807, 822, 838, 853, 869, 884, 899, 915, 930, 945, 961, 976]

def jp(pin):
    return xs[(pin - 1) // 2], 120 + (960 if pin % 2 == 0 else 978)

# Radio pad centers were inspected on the unchanged original photo.
nets = [
    (9, 2, 'CLK', (2076, 1256), '#D49700'),
    (11, 3, 'MOSI', (2076, 1289), '#008a53'),
    (13, 4, 'MISO', (2076, 1322), '#1565c0'),
    (15, 5, 'CS', (2076, 1224), '#a832ad'),
    (17, 25, 'RESET', (2076, 1354), '#6d4c41'),
    (19, 49, 'BUSY', (2076, 1387), '#e27300'),
    (20, 50, 'DIO1', (2273, 1322), '#455a64'),
    (18, 51, 'RXEN', (2273, 1224), '#008b8b'),
    (16, 52, 'TXEN', (2273, 1256), '#9c4a00'),
]
for n, (pin, gpio, target, end, color) in enumerate(nets):
    x, y = jp(pin)
    sy = 1190 + n * 35
    ex, ey = end
    ey += 120
    if ex < 2200:
        points = [(x, y), (x, sy), (1860 - n * 18, sy), (1860 - n * 18, ey), (ex, ey)]
    else:
        points = [(x, y), (x, sy), (2385 + (n - 6) * 22, sy), (2385 + (n - 6) * 22, ey), (ex, ey)]
    wire(points, color)
    box(1090, sy - 25, 600, 32, '#ffffff', color)
    text(1102, sy - 2, f'{n + 1:02d}   J7.{pin} / GPIO{gpio}  →  {target}', 23, color, 'bold')
# Radio supply and common grounds use a separate outside bus.
wire([jp(1), (790, 1140), (2485, 1140), (2485, 1507), (2273, 1507)], '#c62828')
for end in [(2076, 1312), (2273, 1281), (2273, 1312), (2273, 1474)]:
    wire([jp(7), (780, 1160), (2505, 1160), (2505, end[1]), end], '#20252a')
box(1100, 1096, 600, 64)
text(1112, 1120, '10  J7.1 (3V3) → radio 3V3', 24, '#c62828', 'bold')
text(1112, 1150, '11  J7.7 (GND) → all radio GND pads', 24, '#20252a', 'bold')
# RF is a short impedance-controlled feed, never a long GPIO jumper.
wire([(2076, 1281), (1980, 1281), (1980, 1225)], '#b58a19')
box(1710, 1165, 300, 54, '#fff8e5', '#b58a19')
text(1723, 1200, 'ANT → 868 MHz antenna', 20, '#765607', 'bold')
# These connectors are marked as connector groups, avoiding invented GPS pad locations.
box(60, 300, 580, 118)
text(78, 334, 'UART1 / J2  (top-left connector)', 26, weight='bold')
text(78, 370, 'J2.1 RX48 ← GPS TXD; J2.4 → GPS GND', 23, '#00824c')
text(78, 402, 'J2.2 TX47 and J2.3 3V3: unused', 22)
wire([(247, 255), (247, 288), (130, 288), (130, 300)], '#00824c')
box(720, 300, 640, 155)
text(738, 334, 'I2C1 / J13  (beside UART1)', 26, weight='bold')
text(738, 370, '1 SCL46 → SCL; 2 SDA45 ↔ SDA', 23, '#315fbc')
text(738, 402, '3 3V3 → VCC; 4 GND → GND (INA226)', 23)
wire([(390, 255), (390, 278), (790, 278), (790, 300)], '#315fbc')
box(1810, 650, 730, 118, '#fff6e6', '#dd8d00')
text(1828, 684, 'GPS: identify actual VCC / GND / TXD pads.', 25, weight='bold')
text(1828, 721, 'Conditional power: J7.2 5V → VCC only if rated for 5V.', 22, '#9a4d00')
text(1828, 751, 'Verify 3.3V UART compatibility; RXD/PPS unused; GPS antenna.', 21)
wire([(100, 418), (100, 470), (1730, 470), (1730, 700), (1810, 700)], '#00824c')
wire([(500, 418), (500, 495), (1760, 495), (1760, 738), (1810, 738)], '#20252a')
wire([jp(2), (740, 1080), (740, 1060), (1785, 1060), (1785, 760), (1810, 760)], '#df8b00', True)
for n, (sourceX, targetX, label, color) in enumerate([(760, 2148, '1 SCL', '#8e24aa'), (910, 2113, '2 SDA', '#1565c0'), (1060, 2046, '3 VCC', '#c62828'), (1210, 2079, '4 GND', '#20252a')]):
    text(sourceX - 30, 437, label, 18, color, 'bold')
    lane = 1640 + n * 24
    wire([(sourceX, 450), (sourceX, 520 + n * 24), (lane, 520 + n * 24), (lane, 1060 + n * 15), (targetX, 1060 + n * 15), (targetX, 1027)], color)
wire([(2217, 1027), (2217, 1054), (2252, 1054), (2252, 1027)], '#a05c00')
box(1820, 1090, 720, 65, '#ffffff')
text(1838, 1118, 'INA226: VBS → IN−; A0/A1 → GND for 0x40.', 24)
text(1838, 1146, 'Verify shunt; ALE unused. See high-side circuit below.', 23)
box(1080, 700, 570, 82, '#fff2df', '#dd8d00')
text(1096, 734, 'UART3-IN is level-shifted to 5V.', 25, '#9a4d00', 'bold')
text(1096, 766, 'Use UART1 for the GPS.', 25, '#9a4d00')
box(1080, 820, 570, 74, '#ecfff8', '#008a53')
text(1096, 851, 'WiFi: separate onboard C6 (IC1).', 25, '#006b43', 'bold')
text(1096, 880, 'V1.2 WiFi reset = GPIO32.', 24, '#006b43')
# Detailed physical header map makes row orientation explicit without relying on blurry silkscreen.
box(40, 1770, 2520, 235, '#f3f7fa')
text(65, 1810, 'J7 rear view as photographed: left → right; top row toward PCB, bottom row toward board edge', 29, weight='bold')
even = ['5V', '5V', '5V', 'GND', 'GND', 'IO54', 'IO53', 'IO52', 'IO51', 'IO50', 'GND', 'GND']
odd = ['3V3', '3V3', '3V3', 'GND', 'IO2', 'IO3', 'IO4', 'IO5', 'IO25', 'IO49', 'GND', 'GND']
for row, labels in enumerate([even, odd]):
    for col, label in enumerate(labels):
        x, y = 70 + col * 204, 1835 + row * 73
        box(x, y, 185, 60, '#ffffff')
        pin = col * 2 + (2 if row == 0 else 1)
        text(x + 10, y + 24, f'pin {pin}', 20, '#62788a')
        text(x + 10, y + 49, label, 23, weight='bold')
text(65, 1990, 'Derived from V1.2 Eagle J7 footprint (R90) and netlist. Confirm rails and physical orientation before connecting.', 22, '#9a4d00')
box(40, 2030, 1300, 235, '#ffffff')
text(65, 2070, 'INA226 high-side measurement circuit', 28, weight='bold')
text(65, 2120, 'Source +  →  IN+  →  fitted shunt  →  IN−  →  Load +', 30)
text(65, 2162, 'IN− → VBS     |     Source − = Load − = module GND', 27)
text(65, 2208, 'VCC is 3.3V logic supply. Sense inputs: 0–36V maximum.', 25, '#9a4d00')
text(65, 2244, 'Shunt value / current / resistor power rating must be checked.', 25, '#9a4d00')
box(1370, 2030, 1190, 235, '#ffffff')
text(1395, 2070, 'Radio assembly and RF switch', 28, weight='bold')
text(1395, 2113, '100nF + 10µF across 3V3/GND close to the module.', 25)
text(1395, 2154, 'RX: RXEN=1, TXEN=0     TX: RXEN=0, TXEN=1', 25)
text(1395, 2195, 'DIO2: leave unconnected. ANT: short 50Ω RF feed.', 25)
text(1395, 2236, 'J7 remains the selected LoRa connection.', 24, '#9a4d00')
text(45, 2310, 'Crossings are not junctions. Wires are schematic connections, not suggested physical cable routes.', 27, weight='bold')
text(45, 2355, 'Photo endpoints are visual guides; the labelled J-pin / module-pin wiring table in docs/wiring.md is authoritative.', 25)
text(45, 2400, 'Original photographs are embedded unchanged. GPS power and shunt calibration remain conditional.', 25, '#9a4d00')
parts.append('</svg>')
(root / 'docs/wiring.svg').write_text('\n'.join(parts))
(root / 'docs/radio-nets.json').write_text(json.dumps([dict(pin=p, gpio=g, module=t, color=c) for p,g,t,e,c in nets], indent=2) + '\n')
