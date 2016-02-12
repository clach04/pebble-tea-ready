#pragma once
#include <pebble.h>

/********************/
/*     FUNCTION     */
/********************/

void wakeup_destroy();
void wakeup_timer_handler(WakeupId, int32_t);