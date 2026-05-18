#include "mf_resources.h"
#include "mf_log.h"
#include <Arduino.h>
#include <esp_heap_caps.h>

void mf_log_resource_metrics(const char *tag) {
    const char *who = (tag && tag[0]) ? tag : "heap";
    uint32_t free_heap = ESP.getFreeHeap();
    uint32_t min_heap = ESP.getMinFreeHeap();
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    mf_log_info(who, "heap free=%lu min_free=%lu largest_block=%u",
                (unsigned long)free_heap, (unsigned long)min_heap, (unsigned)largest);
}

void mf_boot_log_resource_metrics() {
    uint32_t flash_kb = ESP.getFlashChipSize() / 1024u;
    mf_log_resource_metrics("boot");
    mf_log_info("boot", "flash_chip=%luKiB", (unsigned long)flash_kb);
}
