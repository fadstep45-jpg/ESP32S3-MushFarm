#include "mf_resources.h"
#include "mf_log.h"
#include <Arduino.h>
#include <esp_heap_caps.h>

void mf_boot_log_resource_metrics() {
    uint32_t flash_kb = ESP.getFlashChipSize() / 1024u;
    uint32_t free_heap = ESP.getFreeHeap();
    uint32_t min_heap = ESP.getMinFreeHeap();
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    mf_log_info("boot", "heap free=%lu min_free=%lu largest_block=%u flash_chip=%luKiB",
                (unsigned long)free_heap, (unsigned long)min_heap, (unsigned)largest,
                (unsigned long)flash_kb);
}
