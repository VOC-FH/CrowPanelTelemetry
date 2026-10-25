// Paste into TTN Console > Application > Payload formatters > Uplink > JavaScript.
function decodeUplink(input) {
    let result = {};
    const b = input.bytes;
    if (input.fPort === 2 && b.length === 4 && b[0] === 68 && b[1] === 69 && b[2] === 77 && b[3] === 79) {
        result = {data: {message: "DEMO"}};
    } else if (input.fPort !== 1 || b.length < 7 || b[0] !== 1) {
        result = {errors: ["Unsupported port, length or payload version"]};
    } else {
        const offset = b[6] >>> 4;
        const count = b[6] & 15;
        if (count < 1 || offset + count > 10 || b.length !== 7 + 4 * count) {
            result = {errors: ["Invalid batch fragment"]};
        } else {
            const firstMinute = b[3] * 65536 + b[4] * 256 + b[5];
            const readings = [];
            for (let n = 0; n < count; ++n) {
                const j = 7 + n * 4;
                const packed = ((b[j] << 24) | (b[j + 1] << 16) | (b[j + 2] << 8) | b[j + 3]) >>> 0;
                let sample = {index: offset + n, uptime_minutes: firstMinute + offset + n, valid: false};
                if (packed !== 0xFFFFFFFF) {
                    let current = (packed >>> 10) & 0x7FF;
                    if (current & 0x400) {
                        current -= 2048;
                    }
                    sample.valid = true;
                    sample.voltage_V = ((packed >>> 21) & 0x7FF) / 50;
                    sample.current_A = current / 100;
                    sample.power_W = (packed & 0x3FF) / 10;
                }
                readings.push(sample);
            }
            result = {data: {version: 1, boot_id: b[1] * 256 + b[2], first_sample_minute: firstMinute,
                fragment_offset: offset, sample_count: count, batch_size: 10, readings: readings}};
        }
    }
    return result;
}
if (typeof module !== "undefined") {
    module.exports = {decodeUplink};
}
