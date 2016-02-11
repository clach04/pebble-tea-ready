#include "inbox.h"

/********************/
/*     MESSAGES     */
/********************/

void inbox_received_handler(DictionaryIterator *iter, void *context) {
  int i;
  
  // Load all settings
  for(i = PERSIST_TEA_BLACK; i <= PERSIST_TEA_WHITE; i++) {
    Tuple *steep_time = dict_find(iter, i);
    persist_write_int(i, steep_time->value->int32);
  }
  
  // Refresh menu
  menu_mark_dirty();
}