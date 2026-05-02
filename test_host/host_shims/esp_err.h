#pragma once
/* Minimal host-test shim for ESP-IDF's esp_err.h. Real header lives in
 * the IDF tree; on the host build we only need the typedef so component
 * headers that expose esp_err_t in their public API can compile. */
typedef int esp_err_t;
#define ESP_OK 0
