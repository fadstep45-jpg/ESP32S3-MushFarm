#include "mf_log.h"
#include "mf_batch_logger.h"
#include <stdarg.h>
#include <stdio.h>

#define MF_LOG_LINE_MAX 192

static void mf_log_emit(char level, const char *tag, const char *fmt, va_list ap) {
    char body[MF_LOG_LINE_MAX];
    vsnprintf(body, sizeof(body), fmt, ap);

    char line[MF_LOG_LINE_MAX];
    snprintf(line, sizeof(line), "%c [%lu][%s] %s", level, (unsigned long)millis(), tag, body);

    Serial.println(line);
    mf_batch_logger_push(line);
}

void mf_log_init(unsigned long baud_rate) {
    Serial.begin(baud_rate);
    // Give the host time to attach the USB CDC port before first prints.
    unsigned long started = millis();
    while (!Serial && (millis() - started) < 1500) {
        delay(10);
    }
}

void mf_log_info(const char *tag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); mf_log_emit('I', tag, fmt, ap); va_end(ap);
}
void mf_log_warn(const char *tag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); mf_log_emit('W', tag, fmt, ap); va_end(ap);
}
void mf_log_error(const char *tag, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); mf_log_emit('E', tag, fmt, ap); va_end(ap);
}
