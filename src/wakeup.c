#include "keys.h"
#include "tea_cup.h"
#include "wakeup.h"

/********************/
/*  STATIC DECLARE  */
/********************/

static void wakeup_back_handler(ClickRecognizerRef, void*);
static void wakeup_click_config_provider(void*);
static void wakeup_update_layer(Layer*, GContext*);
static void wakeup_vibrate_handler(void*);
static void wakeup_window_load(Window*);
static void wakeup_window_unload(Window*);

/********************/
/*    VARIABLES     */
/********************/

// Display variables
static Layer *s_tea_cup_canvas_layer;
static Window *s_wakeup_window;

// Reference variables
static AppTimer *s_vibrate_timer;

// Other variables
static uint8_t vibrate_count = 0;

/********************/
/*  WINDOW DISPLAY  */
/********************/

// Loading code for the window
static void wakeup_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Set action buttons
  window_set_click_config_provider(window, wakeup_click_config_provider);

  // Change background color
  #ifdef PBL_COLOR
  window_set_background_color(window, GColorOrange);
  #else
  window_set_background_color(window, GColorLightGray);
  #endif

  // Create canvas Layer and set up the update procedure
  s_tea_cup_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_tea_cup_canvas_layer, wakeup_update_layer);
  layer_add_child(window_layer, s_tea_cup_canvas_layer);
}

// Update the display layer
static void wakeup_update_layer(Layer *layer, GContext *ctx) {
  tea_cup_draw(layer, ctx, 100);
}

/********************/
/* BUTTON  HANDLING */
/********************/

// Set back button handler
static void wakeup_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, wakeup_back_handler);
}

// Function called on back button press
static void wakeup_back_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop_all(true);
}

/********************/
/*  WINDOW DESTROY  */
/********************/

// Unloading code
static void wakeup_window_unload(Window *window) {
  app_timer_cancel(s_vibrate_timer);
  layer_destroy(s_tea_cup_canvas_layer);
  tea_cup_destroy();
  window_destroy(window);
}

/********************/
/*      TIMERS      */
/********************/

// Handle wakeup calls
void wakeup_timer_handler(WakeupId id, int32_t reason) {
  // Delete persistent storage value
  if (persist_exists(PERSIST_WAKEUP)) {
    persist_delete(PERSIST_WAKEUP);
    persist_delete(PERSIST_DURATION);
  }
  
  // Create wakeup window
  s_wakeup_window = window_create();
  window_set_window_handlers(s_wakeup_window, (WindowHandlers){
    .load = wakeup_window_load,
    .unload = wakeup_window_unload,
  });
  
  // Push wakeup window with animation 
  window_stack_push(s_wakeup_window, true);
  
  // Vibrate
  vibrate_count = 0;
  s_vibrate_timer = app_timer_register(0, wakeup_vibrate_handler, NULL);
}

// Vibration timer
static void wakeup_vibrate_handler(void *data) {
  if(vibrate_count < 10) {
    // Vibrate the watch
    vibes_double_pulse();
    
    // Call back in 10 seconds
    s_vibrate_timer = app_timer_register(10 * 1000, wakeup_vibrate_handler, data);
  }
  else if(vibrate_count == 10) {
    // Wait for 2 minutes then close the app
    s_vibrate_timer = app_timer_register(2 * 60 * 1000, wakeup_vibrate_handler, data);
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