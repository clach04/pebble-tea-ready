#pragma once

#include <pebble.h>
#include "keys.h"
#include "tea_cup.h"

/********************/
/*     FUNCTION     */
/********************/

void wakeup_destroy();
void wakeup_timer_handler(WakeupId, int32_t);