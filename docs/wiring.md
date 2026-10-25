# Wiring: CrowPanel Advanced 10.1-inch V1.2

This assignment is derived from the supplied V1.2 Eagle schematic **and board**,
not generic ESP32 pin defaults or the older Elecrow demo. The schematic title
inside the PDF still says V1.0 / 7.0-inch; the actual linked V1.2 netlist/PCB are
used here. The header numbers below come from the Eagle connector devices and
physical footprint rotation. Read the board from the rear, upright as photographed.

The governing invariant is one owner per peripheral and no externally driven pin
sharing a built-in panel signal. SPI is routed through the P4 GPIO matrix, not
through fixed generic-Arduino SPI defaults. GPIO2/3/4/5/25/49/50/51/52 are exposed,
unused digital GPIOs on this schematic; outputs, MISO/BUSY inputs and interrupt
DIO1 are suitable digital functions. No ADC/DAC ability is required on these pins.
The P4 boot straps GPIO34/35/36/37/38 are avoided by the radio assignment.
Core 3.3.12 enables the 3.3V VO4 I/O supply for GPIO39–48. Its generic SD wrapper
also drives GPIO45 as SD power and is unsuitable here. The application instead
uses an IDF slot-0 mount with a fixed shared 3.3V supply reference, no power GPIO
and no SD voltage switching. All physical pin assignments remain unchanged.

The photograph's small silkscreen is difficult to read. Verify connector orientation
and rails on the actual board with power off, then verify voltages before attaching
modules. Wire by the **J-pin and signal names**, never by cable color alone.
The SVG/PNG is an overlay with labelled breakout insets. GPS pads are deliberately
not guessed from indistinct photos. The earlier AI-generated draft was rejected
for misplaced endpoints and is not part of this project.

## Core1262-868M / SX1262

WiFi comes from the separate onboard ESP32-C6 (IC1). The upper-right wireless
extension socket is not required for WiFi. The external Core1262 remains on J7
by user decision; no socket adapter or pin reassignment is used.

| Panel contact | P4 GPIO / net | Radio pad | Direction at P4 |
|---|---|---|---|
| J7.9 | GPIO2 | CLK | Output, SPI clock |
| J7.11 | GPIO3 | MOSI | Output, SPI data |
| J7.13 | GPIO4 | MISO | Input, SPI data |
| J7.15 | GPIO5 | CS | Output, active low |
| J7.17 | GPIO25 | RESET | Output, active low |
| J7.19 | GPIO49 | BUSY | Input |
| J7.20 | GPIO50 | DIO1 | Input, interrupt-capable |
| J7.18 | GPIO51 | RXEN | Output, RF switch receive control |
| J7.16 | GPIO52 | TXEN | Output, RF switch transmit control |
| J7.1 | 3.3V | 3V3 | Power |
| J7.7 | GND | All exposed GND pads | Common ground / RF return |
| No P4 contact | — | DIO2 | Leave unconnected |
| No P4 contact | — | ANT | 868MHz antenna via short 50Ω RF feed |

Fit 100nF ceramic and about 10µF bulk decoupling close to the radio supply.
Use a suitable carrier for the castellated module, short digital wires, and a
short ground return. Do not run ANT along a breadboard jumper: use a proper
antenna connector/coax and ground layout. Attach the antenna before transmitting.
Radio initialization requests14dBm; LoRaWAN/ADR subsequently controls power up
to the EU868 profile maximum (16dBm). Verify actual conducted power and total
EIRP with the antenna/cable you actually use before RF testing.

RXEN and TXEN are **separate switch inputs**. RX uses RXEN=1/TXEN=0; TX uses
RXEN=0/TXEN=1; idle uses both low. RadioLib controls them through
`setRfSwitchPins()`, and its automatic DIO2 switch output is disabled. The English
Waveshare wiki reverses the RX/TX sentence; its Chinese documentation and the RF
paths on the schematic establish the mapping above. Do not short DIO2 to RXEN.

The published module schematic shows TCXO power from 3V3 through R22, with an
alternative DIO3 ferrite path. The default `RADIO_TCXO_VOLTAGE=0.0f` disables DIO3
supply control for the fixed-supply arrangement. **Population of R22/FB1 on your
actual module is not established by the photograph.** Confirm it before RF bring-up.
If only DIO3 supplies the oscillator, obtain its rated voltage and set that exact
value; do not guess 1.8V or 3.3V. DIO3 needs no external P4 wire.

## INA226 external I2C1 connector J13

J13 sits immediately beside UART1 at the top left. Its contact order, viewed from
the same rear board view, left → right, is **SCL, SDA, 3V3, GND**. Cable mating
views mirror this order; confirm the housing's pin 1 rather than copying a cable
photo. Shared I2C includes the on-board touch controller and audio codec.

| Panel contact | Signal | INA226 pad |
|---|---|---|
| J13.1 | SCL / GPIO46 | SCL |
| J13.2 | SDA / GPIO45 | SDA |
| J13.3 | 3.3V | VCC |
| J13.4 | GND | GND |
| GND | Address strap | A0 and A1, for 0x40 |
| No connection | Alert output | ALE |
| Load-side IN− | Bus sense | VBS |

Inspect existing A0/A1 straps before adding jumpers. Do not short a strap already
connected to another rail. Default software address is 0x40; change `INA_ADDRESS`
if the actual module uses another non-conflicting address. The ES8311 uses 0x18;
touch is normally 0x5D (0x14 is also supported). Start I2C at 100kHz. Existing
pull-ups may suffice; measure bus rise time if long wiring causes errors. All
external pull-ups must go to 3.3V. Do not power this breakout from 5V.

Connect the measured circuit as:

```
source + ---- IN+ ---- fitted shunt ---- IN− ---- load +
                                        |
                                       VBS
source − ------------------------------------- load −
   |
panel/module GND
```

The chip's sensing range is 0–36V and shunt differential range is approximately
±81.92mV. These are chip limits, **not a current rating for the photographed PCB**.
Confirm whether a shunt is installed and its resistance/power rating, then respect
`I × R < 81.9mV` and `I² × R` heating plus connector/trace ratings. For example,
R100=0.1Ω with the default 0.8A calibration gives 80mV and 0.064W in the shunt;
that example is not identification of your board's resistor.

The photo does not establish the shunt value. `INA_SHUNT_OHMS=0` intentionally
keeps current/power unavailable until you enter the real value. Uncalibrated
batches are marked invalid; measured voltage can still appear on screen.
Power sent to TTN is the INA226 power register, not a second U×I calculation.
Power is represented as the monitor's unsigned register output; signed current
is preserved. This template targets a DC source/load measurement, not AC/mains.

## GT-U7 GPS and UART1 J2

J2 is the top-left UART1 connector, **not the right-side UART3-IN**. From the rear
board view, left → right: **RX48, TX47, 3V3, GND**.

| Panel contact | GPS pad | Status |
|---|---|---|
| J2.1 RX / GPIO48 | TXD | Only after confirming TXD is 3.3V-compatible UART |
| J2.4 GND | GND | Common ground |
| J7.2 5V | VCC | **Conditional: connect only if this exact breakout accepts 5V** |
| Unused | RXD | Not needed for NMEA reception |
| Unused | PPS | Not needed |
| Unused J2.2 / J2.3 | — | No wire |

The photograph identifies a GT-U7 family board but not a manufacturer-backed
schematic for its regulator, VCC rating, or UART level. Therefore the GPS power
connection is **not verified for your exact module**. Obtain the seller's exact
specification or inspect its regulator/board first. If it needs regulated 3.3V,
use the documented 3.3V rail instead; do not feed 5V into a bare 3.3V GPS supply.
Do not connect its USB supply simultaneously without verifying the board's power
path. If TXD is 5V or1.8V, use an appropriate UART level translator to3.3V first;
a merely lower-than3.3V high level is not sufficient to guarantee input recognition.

Use 9600 baud NMEA initially; change `GPS_BAUD` if its configured rate differs.
Attach its appropriate GPS antenna and place it with a clear sky view. No GPS RX
wire is needed, so the P4 cannot accidentally reconfigure it.

UART3-IN J10 has 5V-level translation on TX/RX and its VCC is a **power input**.
It is unsuitable as a guessed 3.3V GPS connector or as a GPS power output.

## Complete J7 orientation

The Eagle J7 package is 2×12, rotation R90. As seen in the supplied upright rear
photo, columns run left → right; the even row is toward the board interior and
the odd row toward the lower board edge.

| Column | Top / even contact | Net | Bottom / odd contact | Net |
|---|---|---|---|---|
| 1 | 2 | 5V | 1 | 3V3 |
| 2 | 4 | 5V | 3 | 3V3 |
| 3 | 6 | 5V | 5 | 3V3 |
| 4 | 8 | GND | 7 | GND |
| 5 | 10 | GND | 9 | GPIO2 |
| 6 | 12 | GPIO54 | 11 | GPIO3 |
| 7 | 14 | GPIO53 | 13 | GPIO4 |
| 8 | 16 | GPIO52 | 15 | GPIO5 |
| 9 | 18 | GPIO51 | 17 | GPIO25 |
| 10 | 20 | GPIO50 | 19 | GPIO49 |
| 11 | 22 | GND | 21 | GND |
| 12 | 24 | GND | 23 | GND |

## Built-in peripherals retained

| Function | V1.2 assignment / ownership |
|---|---|
| LCD | Two-lane MIPI DSI; reset41, backlight31, power29 active low |
| GT911 | I2C45/46, reset40, interrupt42; polling ready flag |
| C6 WiFi | SDIO clock18, CMD19, D0=17, D1=16, D2=15, D3=14, reset32 |
| SD card | SDMMC slot0 / one-bit CLK43, CMD44, D0=39 |
| Audio | ES8311 I2C0x18; I2S BCLK22, LRCLK21, data23, MCLK24; amp30 |
| GPS | UART1 RX48; TX47 unused |
| INA226 | Shared I2C45/46, default address0x40 |
| LoRa | Dedicated SPI GPIO matrix mapping on J7, separate from onboard C6 |

On-board microphones, CSI camera, USB host, battery charging telemetry and the
alternative unpopulated NS4168 circuit are outside the requested template scope.

## Power budget

Use the panel's intended regulated5V input and its manufacturer-rated supply/cable;
the documented UART3-IN input rating is5V±5%, up to2A. Expansion5V/3V3 contacts
are supply rails, not arbitrarily interchangeable power inputs. The external I2C
and UART1 ports are specified as3.3V. Confirm total available rail current with the
panel/backlight/WiFi/audio active, rather than treating a connector rating as free
headroom after all on-board loads.

Waveshare lists approximately45mA TX at14dBm and5.3mA receive; the LoRaWAN stack
can request higher power, so do not size the supply to that typical TX value alone.
INA226 itself draws about330µA typical; breakout LEDs/pull-ups can add current.
The exact GT-U7 breakout's current demand remains unverified. During hardware
testing, check3.3V at the radio and5V at the panel during simultaneous audio/WiFi/RF.
