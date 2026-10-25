#include <JPEGDEC.h>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>
static int draw(JPEGDRAW*) {
    return 1;
}
int main(int argc, char** argv) {
    assert(argc >= 3);
    for (int n = 1; n < argc; ++n) {
        std::ifstream file(argv[n], std::ios::binary);
        std::vector<uint8_t> compressed((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        JPEGDEC jpeg;
        bool ok = jpeg.openRAM(compressed.data(), compressed.size(), draw) != 0;
        if (n < 3) {
            assert(ok);
        } else {
            assert(!ok);
        }
        if (ok) {
            struct {
                uint16_t width, height;
            } event{static_cast<uint16_t>(jpeg.getWidth()), static_cast<uint16_t>(jpeg.getHeight())};
#include "ImagePolicy-under-test.h"
            assert(ok == (n == 1));
            if (ok) {
                assert(jpeg.decode(0, 0, 0) != 0);
            }
        }
        jpeg.close();
    }
    return 0;
}
