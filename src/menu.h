#pragma once

#include <pebble.h>
#include "countdown.h"
#include "keys.h"

/********************/
/*     VARIABLE     */
/********************/

typedef struct {
  char name[8];        // Name of this tea
  uint16_t def_time;   // Minimum time in seconds (default)
  uint8_t persist_key; // Persist key for tea
  uint8_t temp;        // Temperature to steep this team (Celcius)
} TeaInfo;

/********************/
/*     FUNCTION     */
/********************/

void menu_destroy();
void menu_display();
void menu_mark_dirty();