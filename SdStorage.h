#pragma once
#include "Config.h"

#include <driver/sdmmc_host.h>
#include <esp32-hal-ldo.h>
#include <esp_vfs_fat.h>
#include <soc/sdmmc_pins.h>

static_assert(Pins::SdClk == SDMMC_SLOT0_IOMUX_PIN_NUM_CLK && Pins::SdCmd == SDMMC_SLOT0_IOMUX_PIN_NUM_CMD &&
              Pins::SdD0 == SDMMC_SLOT0_IOMUX_PIN_NUM_D0);

// One media-worker instance owns slot 0. VO4 is shared, fixed at 3.3 V;
// no SD power GPIO, adjustable LDO, voltage switching or controller-wide deinit.
class SdStorage {
public:
    bool remount() {
        if (card && !unmountFailed) {
            unmountFailed = esp_vfs_fat_sdcard_unmount("/sdcard", card) != ESP_OK;
            card = nullptr;
        }
        bool ok = !unmountFailed;
        if (ok && !power) {
            ok = ldoAcquireChannel(4, 3300, false, &power) == ESP_OK;
        }
        if (ok) {
            sdmmc_host_t host = SDMMC_HOST_DEFAULT();
            host.slot = SDMMC_HOST_SLOT_0;
            sdmmc_host_state_t state{};
            ok = sdmmc_host_get_state(&state) == ESP_OK;
            slotsBefore = state.num_of_init_slots;
            host.deinit_p = [](int slot) {
                sdmmc_host_state_t state{};
                esp_err_t result = sdmmc_host_get_state(&state);
                if (result == ESP_OK && state.num_of_init_slots > slotsBefore) {
                    result = sdmmc_host_deinit_slot(slot);
                }
                return result;
            };
            sdmmc_slot_config_t slot{}; // slot-0 IO_MUX: all GPIO fields must be zero
            slot.width = 1;
            slot.cd = SDMMC_SLOT_NO_CD;
            slot.wp = SDMMC_SLOT_NO_WP;
            esp_vfs_fat_sdmmc_mount_config_t config{};
            config.max_files = 4;
            ok = ok && esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &config, &card) == ESP_OK;
        }
        // Keep the fixed rail reference for the worker lifetime, including retries.
        // A failed unmount has ambiguous SDK cleanup; latch it and require restart.
        return ok;
    }

private:
    inline static uint8_t slotsBefore = 0; // snapshot for the sole media-owned mount
    sdmmc_card_t* card = nullptr;
    ldo_channel_handle_t power = nullptr;
    bool unmountFailed = false;
};
