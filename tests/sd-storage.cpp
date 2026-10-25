#include "SdStorage.h"

#include <cassert>
#include <cstring>
#include <vector>

static int powerResult = 0, mountResult = 0, unmountResult = 0;
static sdmmc_card_t card;
static std::vector<char> calls;
static uint8_t initializedSlots = 1; // C6 owns slot 1 before the media worker starts
static bool earlyFailure = false;
static esp_err_t (*cleanup)(int) = nullptr;
esp_err_t sdmmc_host_get_state(sdmmc_host_state_t* state) {
    state->num_of_init_slots = initializedSlots;
    return ESP_OK;
}
esp_err_t sdmmc_host_deinit_slot(int slot) {
    assert(slot == 0 && initializedSlots == 2);
    --initializedSlots;
    return ESP_OK;
}
esp_err_t ldoAcquireChannel(uint8_t channel, int voltage, bool adjustable, ldo_channel_handle_t* out) {
    calls.push_back('P');
    assert(channel == 4 && voltage == 3300 && !adjustable);
    *out = powerResult == ESP_OK ? &card : nullptr;
    return powerResult;
}
esp_err_t esp_vfs_fat_sdmmc_mount(const char* path,
                                  const sdmmc_host_t* host,
                                  const sdmmc_slot_config_t* slot,
                                  const esp_vfs_fat_sdmmc_mount_config_t* config,
                                  sdmmc_card_t** out) {
    calls.push_back('M');
    assert(strcmp(path, "/sdcard") == 0 && host->slot == 0 && host->flags == DefaultFlags);
    assert(slot->width == 1 && slot->cd == -1 && slot->wp == -1);
    assert(slot->clk == 0 && slot->cmd == 0 && slot->d0 == 0 && slot->d1 == 0 && slot->d2 == 0 && slot->d3 == 0 && slot->d4 == 0 &&
           slot->d5 == 0 && slot->d6 == 0 && slot->d7 == 0);
    assert(!config->format_if_mount_failed && config->max_files == 4);
    cleanup = host->deinit_p;
    if (!earlyFailure) {
        ++initializedSlots;
    }
    if (mountResult == ESP_OK) {
        *out = &card;
    } else {
        assert(cleanup(0) == ESP_OK);
        assert(initializedSlots == 1); // an uncounted slot must never decrement C6
    }
    return mountResult;
}
esp_err_t esp_vfs_fat_sdcard_unmount(const char* path, sdmmc_card_t* value) {
    calls.push_back('U');
    assert(strcmp(path, "/sdcard") == 0 && value == &card);
    if (unmountResult == ESP_OK) {
        assert(cleanup(0) == ESP_OK);
    }
    return unmountResult;
}
int main() {
    SdStorage storage;
    powerResult = -1;
    assert(!storage.remount());
    assert(calls == std::vector<char>{'P'});
    powerResult = 0;
    earlyFailure = true;
    mountResult = -2; // no card: SDK returns failure without an output card
    assert(!storage.remount());
    earlyFailure = false;
    assert(!storage.remount());
    assert(calls == (std::vector<char>{'P', 'P', 'M', 'M'}));
    mountResult = 0;
    assert(storage.remount());
    for (int retry = 0; retry < 3; ++retry) {
        calls.clear();
        assert(storage.remount());
        assert(calls == (std::vector<char>{'U', 'M'}));
    }
    mountResult = -2; // removal during a retry: no old card is reused
    calls.clear();
    assert(!storage.remount());
    assert(calls == (std::vector<char>{'U', 'M'}));
    calls.clear();
    mountResult = 0;
    assert(storage.remount());
    assert(calls == std::vector<char>{'M'});
    calls.clear();
    unmountResult = -3; // ambiguous cleanup must not cause a second unmount/mount
    assert(!storage.remount());
    assert(!storage.remount());
    assert(calls == std::vector<char>{'U'});
    return 0;
}
