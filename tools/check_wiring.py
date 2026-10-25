"""Verify project pin mapping directly against manufacturer's Eagle XML."""
from pathlib import Path
import hashlib
import json
import re
import sys
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
sch, brd = Path(sys.argv[1]), Path(sys.argv[2])
schematic = ET.parse(sch)
board = ET.parse(brd)
nets = {}
for net in schematic.findall('.//sheet/nets/net'):
    for ref in net.findall('.//pinref'):
        nets[(ref.get('part'), ref.get('pin'))] = net.get('name')
config = (root / 'Config.h').read_text()
radio = json.loads((root / 'docs/radio-nets.json').read_text())
variables = {'CLK': 'Sck', 'MOSI': 'Mosi', 'MISO': 'Miso', 'CS': 'Cs', 'RESET': 'Reset',
             'BUSY': 'Busy', 'DIO1': 'Irq', 'RXEN': 'RxEn', 'TXEN': 'TxEn'}
for connection in radio:
    gpio = connection['gpio']
    assert nets[('J7', str(connection['pin']))] == nets[('U7', 'GPIO' + str(gpio))]
    assert int(re.search(r'\b' + variables[connection['module']] + r'\s*=\s*(\d+)', config)[1]) == gpio
    # These nets may only connect the MCU and the expansion header.
    consumers = {part for (part, pin), name in nets.items() if name == nets[('U7', 'GPIO' + str(gpio))]}
    assert consumers == {'U7', 'J7'}, (connection, consumers)
for name, gpio in {'Sda':45, 'Scl':46, 'GpsRx':48, 'TouchReset':40, 'TouchIrq':42,
                   'SdClk':43, 'SdCmd':44, 'SdD0':39, 'Bclk':22, 'Lrclk':21, 'Dout':23,
                   'Mclk':24, 'Amp':30, 'BacklightPower':29, 'Backlight':31,
                   'WifiClk':18, 'WifiCmd':19, 'WifiD0':17, 'WifiD1':16,
                   'WifiD2':15, 'WifiD3':14, 'WifiReset':32}.items():
    assert int(re.search(r'\b' + name + r'\s*=\s*(\d+)', config)[1]) == gpio, name
assert len({r['gpio'] for r in radio}) == 9
for pin in [1, 3, 5]:
    assert nets[('J7', str(pin))] == 'VDD_3V3'
for pin in [2, 4, 6]:
    assert nets[('J7', str(pin))] == 'VDD5V'
for pin in [7, 8, 10, 21, 22, 23, 24]:
    assert nets[('J7', str(pin))] == 'GND'
assert nets[('J2', '1')] == nets[('U7', 'GPIO48')]
assert nets[('J2', '2')] == nets[('U7', 'GPIO47')]
assert nets[('J2', '3')] == nets[('J13', '3')] == 'VDD_3V3'
assert nets[('J2', '4')] == nets[('J13', '4')] == 'GND'
assert nets[('J13', '1')] == nets[('Q9', 'D')]
assert nets[('Q9', 'S')] == nets[('U7', 'GPIO46')]
assert nets[('J13', '2')] == nets[('Q8', 'D')]
assert nets[('Q8', 'S')] == nets[('U7', 'GPIO45')]
assert nets[('U7', 'GPIO32')] == nets[('IC1', 'EN')] == 'C6_EN'
assert nets[('J10', '3')] == '+5V_IN'
header = board.find('.//elements/element[@name="J7"]')
assert header.get('rot') == 'R90'
library = board.find('.//libraries/library[@name="' + header.get('library') + '"]')
package = library.find('./packages/package[@name="' + header.get('package') + '"]')
pads = {pad.get('name'): (float(pad.get('x')), float(pad.get('y'))) for pad in package.findall('smd')}
# Rotation R90 maps package (x,y) -> board (-y,x): even pads lie above odd pads.
for column in range(12):
    odd, even = pads[str(column * 2 + 1)], pads[str(column * 2 + 2)]
    assert odd[0] < even[0] and odd[1] == even[1]
    if column > 0:
        assert odd[1] < pads[str(column * 2 - 1)][1]
print('PASS: 9 distinct radio GPIOs have no on-board consumers; Config.h matches Eagle.')
print('PASS: UART1, external I2C/level shifters, power rails, C6 reset and J7 orientation.')
print('Schematic SHA256:', hashlib.sha256(sch.read_bytes()).hexdigest())
print('PCB SHA256:', hashlib.sha256(brd.read_bytes()).hexdigest())
