#include "countdown.h"
#include "keys.h"
#include "tea_cup.h"
#include "menu.h"

/********************/
/*  STATIC DECLARE  */
/********************/

static void countdown_back_handler(ClickRecognizerRef, void*);
static void countdown_cancel_handler(ClickRecognizerRef, void*);
static void vibrate_cancel_handler(ClickRecognizerRef, void*);
static void countdown_click_config_provider(void*);
static void countdown_timer_handler(void*);
static void countdown_update_layer(Layer*, GContext*);
static void countdown_window_load(Window*);
static void countdown_window_unload(Window*);
static void wakeup_vibrate_handler(void*);
static void completed_click_config_provider(void*);

/********************/
/*    VARIABLES     */
/********************/

// Display variables
static Window *s_countdown_window;
static Layer *s_tea_cup_canvas_layer;
static ActionBarLayer *s_action_bar_layer;
static GBitmap *s_cross_bitmap;
static GBitmap *s_check_bitmap;
static TextLayer *s_ready_text_layer;

// Reference variables
static AppTimer *s_countdown_timer;
static AppTimer *s_vibrate_timer;

// Other variables
static time_t s_wakeup_timestamp = 0;
static uint8_t s_countdown_percentage = 0;
static uint16_t s_countdown_duration = 1;
static uint8_t vibrate_count = 0;
static uint8_t s_count_mode;

/********************/
/*  WINDOW DISPLAY  */
/********************/

// Remotely callable function to display the window
void countdown_display() {
  // Force a window update if already displayed
  window_stack_remove(s_countdown_window, false);
  
  // Create window
  s_countdown_window = window_create();
  window_set_window_handlers(s_countdown_window, (WindowHandlers){
    .load = countdown_window_load,
    .unload = countdown_window_unload,
  });
    
  // Display window
  window_stack_push(s_countdown_window, false);
}

// Loading code for the window
static void countdown_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Get stored variables
  WakeupId wakeup_id = persist_read_int(PERSIST_WAKEUP);
  wakeup_query(wakeup_id, &s_wakeup_timestamp);
  s_countdown_duration = persist_read_int(PERSIST_DURATION);
  s_count_mode = persist_read_int(PERSIST_COUNT_MODE);

  // Display action bar while counting down
  if(s_count_mode != 2) {
    s_cross_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CROSS);
    s_check_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHECK);
    s_action_bar_layer = action_bar_layer_create();
    if(s_count_mode == 1 && s_vibrate_timer)
      action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, s_check_bitmap);
    else
      action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, s_cross_bitmap);
    action_bar_layer_set_click_config_provider(s_action_bar_layer, countdown_click_config_provider);
    action_bar_layer_add_to_window(s_action_bar_layer, window);
  }
  else
    window_set_click_config_provider(window, completed_click_config_provider);
  
  // Setup countdown timer
  if(s_count_mode != 2)
    s_countdown_timer = app_timer_register(0, countdown_timer_handler, NULL);
  
  // Create canvas Layer and set up the update procedure
  s_tea_cup_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_tea_cup_canvas_layer, countdown_update_layer);
  layer_add_child(window_layer, s_tea_cup_canvas_layer);
  
  // Display text based on state
  s_ready_text_layer = text_layer_create(GRect(0, bounds.size.h / 2 + 25, bounds.size.w, 40));
  switch(s_count_mode) {
    case 0:
      // Matcha tea
      if(persist_read_int(PERSIST_TEA) == 4)
        text_layer_set_text(s_ready_text_layer, "Whisk");
      else
        text_layer_set_text(s_ready_text_layer, "Let it steep");
      break;
    case 2:
      text_layer_set_text(s_ready_text_layer, "Enjoy your tea");
      break;
    default:
      text_layer_set_text(s_ready_text_layer, "Let it cool");
      break;
  }
  text_layer_set_text_alignment(s_ready_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_ready_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(s_ready_text_layer, GColorClear);
  layer_add_child(window_layer, text_layer_get_layer(s_ready_text_layer));
  
  // Change background color
  #ifdef PBL_COLOR
  switch(s_count_mode) {
    case 0:
      window_set_background_color(s_countdown_window, GColorVividCerulean);
      break;
    case 2:
      window_set_background_color(s_countdown_window, GColorOrange);
      break;
    default:
      window_set_background_color(s_countdown_window, GColorGreen);
      break;
  }
  #else
  window_set_background_color(s_countdown_window, GColorLightGray);
  #endif
}


// Update the display layer
static void countdown_update_layer(Layer *layer, GContext *ctx) {
  tea_cup_draw(layer, ctx, (s_count_mode == 2 ? 100 : s_countdown_percentage), s_count_mode != 2);
}

/********************/
/* BUTTON  HANDLING */
/********************/

// Set back button handler
static void countdown_back_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop_all(true);
}

// Set select button handler to cancel
static void countdown_cancel_handler(ClickRecognizerRef recognizer, void *context) {
  // Cancel the wakeup
  if (persist_exists(PERSIST_WAKEUP)) {
    // Cancel the wakeup
    WakeupId wakeup_id = persist_read_int(PERSIST_WAKEUP);
    if(wakeup_query(wakeup_id, NULL))
      wakeup_cancel(wakeup_id);
  }
  
  // Clear the storage
  persist_delete(PERSIST_WAKEUP);
  persist_delete(PERSIST_DURATION);
  persist_delete(PERSIST_COUNT_MODE);
  persist_delete(PERSIST_TEA);
  
  // Close current window
  window_stack_pop(true);
}

// Set select button handler to stop vibration
static void vibrate_cancel_handler(ClickRecognizerRef recognizer, void *context) {
  if(s_vibrate_timer)
    app_timer_cancel(s_vibrate_timer);
  s_vibrate_timer = NULL;
  countdown_display();
}

// Function called on button press
static void countdown_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, countdown_back_handler);
  if(s_count_mode == 1 && s_vibrate_timer)
    window_single_click_subscribe(BUTTON_ID_SELECT, vibrate_cancel_handler);
  else
    window_single_click_subscribe(BUTTON_ID_SELECT, countdown_cancel_handler);
}
static void completed_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, countdown_back_handler);
  window_single_click_subscribe(BUTTON_ID_UP, countdown_back_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, countdown_back_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, countdown_back_handler);
}

/********************/
/*  WINDOW DESTROY  */
/********************/

// Remotely callable function to kill the window
void countdown_destroy() {
  window_stack_remove(s_countdown_window, true);
}

// Unloading code
static void countdown_window_unload(Window *window) {
  // Destroy interface
  layer_destroy(s_tea_cup_canvas_layer);
  tea_cup_destroy();
  if(s_count_mode != 2) {
    action_bar_layer_destroy(s_action_bar_layer);
    gbitmap_destroy(s_cross_bitmap);
    gbitmap_destroy(s_check_bitmap);
  }
  text_layer_destroy(s_ready_text_layer);
  window_destroy(window);
}

/********************/
/*      TIMERS      */
/********************/

// Countdown timer
static void countdown_timer_handler(void *data) {
  // Get progress
  s_countdown_percentage = (s_countdown_duration - (s_wakeup_timestamp - time(NULL))) * 100 / s_countdown_duration;
  if(s_count_mode == 1)
    s_countdown_percentage = 100 - s_countdown_percentage;
 
  // Update layer
  if(s_tea_cup_canvas_layer)
    layer_mark_dirty(s_tea_cup_canvas_layer);
  
  // Schedule next event every tick
  if(s_count_mode != 2)
    s_countdown_timer = app_timer_register(s_countdown_duration * 1000 / 38, countdown_timer_handler, data);
  else
    s_countdown_timer = NULL;
}

// Vibration timer
static void wakeup_vibrate_handler(void *data) {
  if(vibrate_count < 3) {
    // Vibrate the watch
    vibes_double_pulse();
    
    // Call back in 10 seconds
    s_vibrate_timer = app_timer_register(10 * 1000, wakeup_vibrate_handler, data);
  }
  else if(vibrate_count == 3) {
    if(s_count_mode == 2)
      // Wait for 2 minutes then close the app if tea is ready
      s_vibrate_timer = app_timer_register(2 * 60 * 1000, wakeup_vibrate_handler, data);
    else {
      // Otherwise remove the option to dismiss vibration
      s_vibrate_timer = NULL;
      countdown_display();
    }
  }
  else {
    // Vibrate one last time
    vibes_double_pulse();
    
    // Close after the timeout
    window_stack_pop_all(true);
  }
  
  // Increment the counter
  vibrate_count++;
}

/********************/
/*      WAKEUP      */
/********************/

// Handle wakeup calls
void wakeup_timer_handler(WakeupId id, int32_t reason) {
  // Set to tea complete
  persist_write_int(PERSIST_COUNT_MODE, 2);
  
  // Wakeup not due to "Tea's ready" feature and it is enabled
  if(reason != -1 && persist_read_int(PERSIST_READY) != 0) {
    time_t wakeup_time = time(NULL);
    int delay = 0;
    
    // Reduce the delay by the time needed to steep
    if (persist_exists(PERSIST_DURATION))
      delay -= persist_read_int(PERSIST_DURATION);
    
    // Adjust the delay based on the tea's initial temperature
    delay += (get_tea_temp(reason) - 80) * 0.5 * 60;
    
    // Increase the delay based on the selected temperature
    switch(persist_read_int(PERSIST_READY)) {
      case 2:
        delay += 6 * 60;
        break;
      case 3:
        delay += 14 * 60;
        break;
      case 4:
        delay += 24 * 60;
    }
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Tea should cool for %i seconds", delay);
    
    // Only setup notification if the delay is longer than 30 seconds
    if(delay > 30) {
      persist_write_int(PERSIST_WAKEUP, wakeup_schedule(wakeup_time + delay, -1, false));
      persist_write_int(PERSIST_DURATION, delay);
      persist_write_int(PERSIST_COUNT_MODE, 1);
    }
  }
  
  // Vibrate
  vibrate_count = 0;
  s_vibrate_timer = app_timer_register(0, wakeup_vibrate_handler, NULL);
    
  // Display the countdown
  countdown_display();
}