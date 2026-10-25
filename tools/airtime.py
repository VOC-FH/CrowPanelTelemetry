"""Semtech explicit-header LoRa formula: payload includes 13-byte LoRaWAN overhead."""
import math

def airtime(payload_bytes, sf, fopts=0):
    phy_bytes = payload_bytes + 13 + fopts
    low_data_rate = int(sf >= 11)
    symbols = 8 + max(math.ceil((8 * phy_bytes - 4 * sf + 28 + 16) / (4 * (sf - 2 * low_data_rate))) * 5, 0)
    return (8 + 4.25 + symbols) * (2 ** sf) / 125000

print('47 application bytes + 13 overhead; 125kHz, CR4/5, explicit header, CRC, preamble8')
print('DR | SF | packet ms | 144 batches/day seconds | pacing interval seconds')
for dr, sf in enumerate([12, 11, 10, 9, 8, 7]):
    toa = airtime(47, sf)
    print(f'{dr} | {sf} | {toa * 1000:.3f} | {toa * 144:.3f} | {math.ceil(toa * 2880)}')
assert abs(airtime(10, 7) - 0.061696) < 1e-8
assert abs(airtime(47, 7) - 0.112896) < 1e-8
assert airtime(47, 9) * 144 > 30
assert airtime(47, 7) * 144 < 30
print('PASS: reference 10-byte SF7 airtime; normal packet; low-rate policy conflict.')
