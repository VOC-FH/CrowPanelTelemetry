#pragma once
#include <driver/sdmmc_host.h>
using ldo_channel_handle_t = void*;
esp_err_t ldoAcquireChannel(uint8_t channel, int voltage, bool adjustable, ldo_channel_handle_t* out);
