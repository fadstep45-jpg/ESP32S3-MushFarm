#pragma once

/** S1 roadmap: log heap / fragmentation baseline once at boot (Serial + batch logger). */
void mf_boot_log_resource_metrics();

/** Log current heap stats with a short tag (e.g. "mqtt" after connect). */
void mf_log_resource_metrics(const char *tag);
