#include "countdown.h"

/********************/
/*  STATIC DECLARE  */
/********************/

static void countdown_back_handler(ClickRecognizerRef, void*);
static void countdown_cancel_handler(ClickRecognizerRef, void*);
static void countdown_click_config_provider(void*);
static void countdown_timer_handler(void*);
static void countdown_update_layer(Layer*, GContext*);
static void countdown_window_load(Window*);
static void countdown_window_unload(Window*);

/********************/
/*    VARIABLES     */
/********************/

// Display variables
static Window *s_countdown_window;
//static TextLayer *s_countdown_text_layer;
static Layer *s_tea_cup_canvas_layer;
//static GDrawCommandImage *s_tea_cup_blob;
static ActionBarLayer *s_action_bar_layer;
static GBitmap *s_cross_bitmap;
//static char s_countdown_text[32];

// Reference variables
static AppTimer *s_countdown_timer;

// Other variables
static time_t s_wakeup_timestamp = 0;
static uint8_t s_countdown_percentage = 0;
static uint16_t s_countdown_duration = 1;

/********************/
/*  WINDOW DISPLAY  */
/********************/

// Remotely callable function to display the window
void countdown_display() {
  // Create window
  s_countdown_window = window_create();
  window_set_window_handlers(s_countdown_window, (WindowHandlers){
    .load = countdown_window_load,
    .unload = countdown_window_unload,
  });
  
  // Display window
  window_stack_push(s_countdown_window, true);
}

// Loading code for the window
static void countdown_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Get stored variables
  if (persist_exists(PERSIST_WAKEUP)) {
    WakeupId wakeup_id = persist_read_int(PERSIST_WAKEUP);
    wakeup_query(wakeup_id, &s_wakeup_timestamp);
    s_countdown_duration = persist_read_int(PERSIST_DURATION);
  }
  
  // Change background color
  #ifdef PBL_COLOR
  window_set_background_color(window, GColorVividCerulean);
  #endif

  // Display action bar
  s_cross_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CROSS);
  s_action_bar_layer = action_bar_layer_create();
  action_bar_layer_set_icon(s_action_bar_layer, BUTTON_ID_SELECT, s_cross_bitmap);
  action_bar_layer_set_click_config_provider(s_action_bar_layer, countdown_click_config_provider);
  action_bar_layer_add_to_window(s_action_bar_layer, window);

  // Create canvas Layer and set up the update procedure
  s_tea_cup_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_tea_cup_canvas_layer, countdown_update_layer);
  layer_add_child(window_layer, s_tea_cup_canvas_layer);
  
  // Setup countdown timer
  s_countdown_timer = app_timer_register(0, countdown_timer_handler, NULL);
}


// Update the display layer
static void countdown_update_layer(Layer *layer, GContext *ctx) {
  tea_cup_draw(layer, ctx, s_countdown_percentage);
}

/********************/
/* BUTTON  HANDLING */
/********************/

// Set back button handler
static void countdown_back_handler(ClickRecognizerRef recognizer, void *context) {
  window_stack_pop_all(true); // Exit app while waiting for tea to brew
}

// Set select button handler
static void countdown_cancel_handler(ClickRecognizerRef recognizer, void *context) {
  // Cancel the wakeup
  if (persist_exists(PERSIST_WAKEUP)) {
    WakeupId wakeup_id = persist_read_int(PERSIST_WAKEUP);
    if(wakeup_query(wakeup_id, NULL))
      wakeup_cancel(wakeup_id);
    persist_delete(PERSIST_WAKEUP);
    persist_delete(PERSIST_DURATION);
  }
  
  // Go back to tea selection
  window_stack_pop(true);
}

// Function called on button press
static void countdown_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_BACK, countdown_back_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, countdown_cancel_handler);
}

/********************/
/*  WINDOW DESTROY  */
/********************/

// Remotely callable function to kill the window
void countdown_destroy() {
  window_destroy(s_countdown_window);
}

// Unloading code
static void countdown_window_unload(Window *window) {
  app_timer_cancel(s_countdown_timer);
  layer_destroy(s_tea_cup_canvas_layer);
  tea_cup_destroy();
  gbitmap_destroy(s_cross_bitmap);
  action_bar_layer_destroy(s_action_bar_layer);
}

/********************/
/*      TIMERS      */
/********************/

// Countdown timer
static void countdown_timer_handler(void *data) {
  // Get progress
  s_countdown_percentage = (s_countdown_duration - (s_wakeup_timestamp - time(NULL))) * 100 / s_countdown_duration;
  
  // Update layer
  layer_mark_dirty(s_tea_cup_canvas_layer);
  
  // Schedule next event every tick
  s_countdown_timer = app_timer_register(s_countdown_duration * 1000 / 38, countdown_timer_handler, data);
}