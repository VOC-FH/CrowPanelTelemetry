#pragma once
#include <cstdint>
using esp_err_t = int;
constexpr int ESP_OK = 0, SDMMC_HOST_SLOT_0 = 0, SDMMC_SLOT_NO_CD = -1, SDMMC_SLOT_NO_WP = -1;
constexpr uint32_t DefaultFlags = 0x3F;
struct sdmmc_host_t {
    uint32_t flags;
    int slot;
    esp_err_t (*deinit_p)(int);
};
struct sdmmc_slot_config_t {
    int clk = 0, cmd = 0, d0 = 0, d1 = 0, d2 = 0, d3 = 0, d4 = 0, d5 = 0, d6 = 0, d7 = 0;
    int width = 0, cd = 0, wp = 0;
};
struct sdmmc_card_t {};
#define SDMMC_HOST_DEFAULT() {DefaultFlags, 1, sdmmc_host_deinit_slot}

struct sdmmc_host_state_t {
    uint8_t num_of_init_slots;
};
esp_err_t sdmmc_host_get_state(sdmmc_host_state_t* state);
esp_err_t sdmmc_host_deinit_slot(int slot);
