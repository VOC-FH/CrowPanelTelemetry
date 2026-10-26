#include <JPEGDEC.h>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
static uint64_t digest;
static int draw(JPEGDRAW* block) {
    for (int y = 0; y < block->iHeight; ++y) {
        for (int x = 0; x < block->iWidthUsed; ++x) {
            digest = (digest ^ block->pPixels[y * block->iWidth + x]) * UINT64_C(1099511628211);
        }
    }
    return 1;
}
int main(int argc, char** argv) {
    assert(argc >= 3);
    bool metadata = false;
    uint64_t baselineDigest = 0;
    for (int n = 1; n < argc; ++n) {
        if (std::string(argv[n]) == "--metadata") {
            metadata = true;
        } else {
            std::ifstream file(argv[n], std::ios::binary);
            std::vector<uint8_t> compressed((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            JPEGDEC jpeg;
            bool ok = jpeg.openRAM(compressed.data(), compressed.size(), draw) != 0;
            if (n < 3 || metadata) {
                assert(ok);
            } else {
                assert(!ok);
            }
            if (ok) {
                struct {
                    uint16_t width, height;
                } event{static_cast<uint16_t>(jpeg.getWidth()), static_cast<uint16_t>(jpeg.getHeight())};
#include "ImagePolicy-under-test.h"
                assert(ok == (n == 1 || metadata));
                if (ok) {
                    digest = UINT64_C(14695981039346656037);
                    assert(jpeg.decode(0, 0, 0) != 0);
                    if (metadata) {
                        assert(digest == baselineDigest);
                        assert(jpeg.getOrientation() == 0 && jpeg.getThumbWidth() == 0 && jpeg.getThumbHeight() == 0);
                    } else {
                        baselineDigest = digest;
                    }
                }
            }
            jpeg.close();
        }
    }
    return 0;
}
