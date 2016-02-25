#pragma once
#include <pebble.h>

/********************/
/*     FUNCTION     */
/********************/

void countdown_destroy();
void countdown_display();
void wakeup_timer_handler(WakeupId, int32_t);