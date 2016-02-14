#include "keys.h"
#include "menu.h"
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
static TextLayer *s_ready_text_layer;

// Reference variables
static AppTimer *s_vibrate_timer;
static int s_wakeup_reason;

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
  if(s_wakeup_reason == -1)
    window_set_background_color(window, GColorGreen);
  else
    window_set_background_color(window, GColorOrange);
  #else
  window_set_background_color(window, GColorLightGray);
  #endif

  // Create canvas Layer and set up the update procedure
  s_tea_cup_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_tea_cup_canvas_layer, wakeup_update_layer);
  layer_add_child(window_layer, s_tea_cup_canvas_layer);
  
  // Display ready text if the tea is ready
  if(s_wakeup_reason == -1) {
    s_ready_text_layer = text_layer_create(GRect(0, bounds.size.h / 2 + 25, bounds.size.w, 40));
    text_layer_set_text(s_ready_text_layer, "Enjoy your tea!");
    text_layer_set_text_alignment(s_ready_text_layer, GTextAlignmentCenter);
    text_layer_set_font(s_ready_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_background_color(s_ready_text_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(s_ready_text_layer));
  }
}

// Update the display layer
static void wakeup_update_layer(Layer *layer, GContext *ctx) {
  tea_cup_draw(layer, ctx, 100, s_wakeup_reason == -1 ? false : true);
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
  if(s_vibrate_timer)
    app_timer_cancel(s_vibrate_timer);
  
  if(s_wakeup_reason == -1)
    text_layer_destroy(s_ready_text_layer);
  
  layer_destroy(s_tea_cup_canvas_layer);
  tea_cup_destroy();
  window_destroy(window);
}

/********************/
/*      TIMERS      */
/********************/

// Handle wakeup calls
void wakeup_timer_handler(WakeupId id, int32_t reason) {
  s_wakeup_reason = reason;
  
  // Close all currently open windows
  window_stack_pop_all(false);
  
  // Wakeup due to "Tea's ready" feature
  if(s_wakeup_reason == -1) {
    // Cancel the vibrate timer
    if(s_vibrate_timer) {
      app_timer_cancel(s_vibrate_timer);
      s_vibrate_timer = NULL;
    }
  }
  else {
    // Add "Tea's ready" reminder
    // http://archives.math.utk.edu/ICTCM/VOL07/C023/paper.pdf
    if (persist_exists(PERSIST_READY) && persist_read_int(PERSIST_READY) != 0) {
      time_t wakeup_time = time(NULL);
      int delay = 0;
      
      // Calculate appropriate time
      if (persist_exists(PERSIST_DURATION))
        delay -= persist_read_int(PERSIST_DURATION);
      delay += (get_tea_temp(reason) - 80) * 2.5 * 60;
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
      
      // Only setup notification if the delay long enough, otherwise consider as ready
      if(delay > 30)
        wakeup_schedule(wakeup_time + delay, -1, false);
      else
        s_wakeup_reason = -1;
    }
    
    // Delete persistent storage value
    if (persist_exists(PERSIST_WAKEUP)) {
      persist_delete(PERSIST_WAKEUP);
      persist_delete(PERSIST_DURATION);
    }
  }
  
  // Vibrate
  vibrate_count = 0;
  s_vibrate_timer = app_timer_register(0, wakeup_vibrate_handler, NULL);
    
  // Create wakeup window
  s_wakeup_window = window_create();
  window_set_window_handlers(s_wakeup_window, (WindowHandlers){
    .load = wakeup_window_load,
    .unload = wakeup_window_unload,
  });
  
  // Push wakeup window with animation 
  window_stack_push(s_wakeup_window, true);
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