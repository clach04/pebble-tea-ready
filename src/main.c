#include <pebble.h>
#include "countdown.h"
#include "keys.h"
#include "menu.h"
#include "wakeup.h"
#include "inbox.h"

static void init(void) {
  // Check if the app was launched through a wakeup event
  if (launch_reason() == APP_LAUNCH_WAKEUP) {
    WakeupId id = 0;
    int32_t reason = 0;
    if (wakeup_get_launch_event(&id, &reason)) {
      wakeup_timer_handler(id, reason);
      return;
    }
  }
  
  // Display menu in all cases
  menu_display();
  
  // Check if there is a scheduled event
  if (persist_exists(PERSIST_WAKEUP)) {
    WakeupId wakeup_id = persist_read_int(PERSIST_WAKEUP);
    
    // Query if the event is still valid
    if (wakeup_query(wakeup_id, NULL)) {
      countdown_display();
    }
    else {
      persist_delete(PERSIST_WAKEUP);
    }
  }

  // Handle messages
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(1024, 0);
  
  // Subscribe to wakeup service to get wakeup events while app is running
  wakeup_service_subscribe(wakeup_timer_handler);
}

static void deinit(void) {
  wakeup_destroy();
  countdown_destroy();
  menu_destroy();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
