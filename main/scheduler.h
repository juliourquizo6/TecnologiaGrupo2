#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdint.h> // Para uint64_t

void scheduler_init(void);
bool scheduler_is_operational(void);
void scheduler_config_operational_hours(int start_hour, int end_hour);
void scheduler_set_time_configured(bool configured);
bool scheduler_time_configured(void);
uint64_t scheduler_get_sleep_duration(void);

#endif // SCHEDULER_H
