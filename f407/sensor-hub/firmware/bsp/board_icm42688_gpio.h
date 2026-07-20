#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int board_icm42688_data_ready(void);
int board_icm42688_interrupt_init(void);
int board_icm42688_take_data_ready_event(void);
void board_icm42688_discard_data_ready_events(void);
uint32_t board_icm42688_data_ready_event_count(void);

#ifdef __cplusplus
}
#endif
