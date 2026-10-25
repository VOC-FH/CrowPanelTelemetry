const assert = require('node:assert/strict');
const fs = require('node:fs');
const {decodeUplink} = require('../ttn-decoder.js');
const bytes = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
const result = decodeUplink({fPort: 1, bytes});
assert.equal(result.data.boot_id, 0x1234);
assert.equal(result.data.first_sample_minute, 0x010203);
assert.equal(result.data.readings.length, 10);
assert.equal(result.data.readings[0].voltage_V, 12.34);
assert.equal(result.data.readings[0].current_A, -0.37);
assert.equal(result.data.readings[0].power_W, 4.6);
assert.equal(result.data.readings[1].voltage_V, 36);
assert.equal(result.data.readings[1].current_A, 10.23);
assert.equal(result.data.readings[1].power_W, 102.2);
assert.equal(result.data.readings[2].current_A, -10.23);
for (let n = 3; n <= 7; ++n) {
    assert.equal(result.data.readings[n].valid, false);
    assert.equal(result.data.readings[n].voltage_V, undefined);
}
assert.equal(result.data.readings[8].valid, true);
assert.equal(result.data.readings[8].power_W, 0);
const tail = bytes.slice(0, 7).concat(bytes.slice(7 + 7 * 4));
tail[6] = 0x73;
const fragment = decodeUplink({fPort: 1, bytes: tail});
assert.equal(fragment.data.readings[0].index, 7);
assert.equal(fragment.data.readings[2].index, 9);
assert.equal(fragment.data.readings[2].uptime_minutes, 0x010203 + 9);
assert.deepEqual(decodeUplink({fPort: 2, bytes: [68, 69, 77, 79]}), {data: {message: 'DEMO'}});
for (const bad of [[], [1], bytes.slice(0, 46), bytes.concat([0])]) {
    assert.ok(decodeUplink({fPort: 1, bytes: bad}).errors);
}
for (const badHeader of [0, 11, 0x99, 0xF1]) {
    const bad = [...bytes];
    bad[6] = badHeader;
    assert.ok(decodeUplink({fPort: 1, bytes: bad}).errors);
}
assert.ok(decodeUplink({fPort: 99, bytes}).errors);
console.log('C++ encoder -> TTN decoder: values, limits, invalid slots, fragments and malformed packets passed');
