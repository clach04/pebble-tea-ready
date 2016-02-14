#include "countdown.h"
#include "keys.h"
#include "menu.h"

/********************/
/*  STATIC DECLARE  */
/********************/

static uint16_t menu_sections_count(struct MenuLayer*, uint16_t, void*);
static void menu_draw_row(GContext*, const Layer*, MenuIndex*, void*);
static void menu_select_callback(struct MenuLayer*, MenuIndex*, void*);
static void menu_window_load(Window*);
static void menu_window_unload(Window*);
  
#ifdef PBL_ROUND
static int16_t menu_cell_height(MenuLayer*, MenuIndex*, void*);
#endif

/********************/
/*    VARIABLES     */
/********************/

// Display variables
static char s_tea_text[32];
static MenuLayer *s_menu_layer;
static Window *s_menu_window;

// Tea information
// http://blog.davidstea.com/en/how-long-should-i-let-my-tea-steep/
// http://blog.davidstea.com/en/hot-stuff-tea-steeping-temperatures/
static TeaInfo tea_array[] = {
  {"Black", 240, PERSIST_TEA_BLACK, 96},
  {"Green", 120, PERSIST_TEA_GREEN, 80},
  {"Herbal", 240, PERSIST_TEA_HERBAL, 96},
  {"Maté", 240, PERSIST_TEA_MATE, 85},
  {"Oolong", 240, PERSIST_TEA_OOLONG, 85},
  {"Pu'erh", 240, PERSIST_TEA_PUERH, 96},
  {"Rooibos", 240, PERSIST_TEA_ROOIBOS, 96},
  {"White", 240, PERSIST_TEA_WHITE, 90}
};

/********************/
/*  WINDOW DISPLAY  */
/********************/

// Remotely callable function to display the window
void menu_display() {
  // Create window
  s_menu_window = window_create();
  window_set_window_handlers(s_menu_window, (WindowHandlers){
    .load = menu_window_load,
    .unload = menu_window_unload,
  });
  
  // Display window
  window_stack_push(s_menu_window, true);
}

// Loading code for the window
static void menu_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create menu layer
  s_menu_layer = menu_layer_create(bounds);
  
  // Setup menu control
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_rows = menu_sections_count,
    .get_cell_height = PBL_IF_ROUND_ELSE(menu_cell_height, NULL),
    .draw_row = menu_draw_row,
    .select_click = menu_select_callback
  }); 
  menu_layer_set_click_config_onto_window(s_menu_layer,	window);
  
  // Change color scheme
  #ifdef PBL_COLOR
  menu_layer_set_highlight_colors(s_menu_layer, GColorDukeBlue, GColorWhite);
  #endif
  
  // Display menu
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

// Get entry count
static uint16_t menu_sections_count(struct MenuLayer *menulayer, uint16_t section_index, void *callback_context) {
  int count = sizeof(tea_array) / sizeof(TeaInfo);
  return count;
}

// Get cell height
#ifdef PBL_ROUND
static int16_t menu_cell_height(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  return 60;
}
#endif

// Display menu entry
static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  char* name = tea_array[cell_index->row].name;
  int steep_time = tea_array[cell_index->row].def_time;
  int persist_key = tea_array[cell_index->row].persist_key;
  int temp = tea_array[cell_index->row].temp;
  
  // Get user preference
  if(persist_exists(persist_key)) {
    steep_time = persist_read_int(persist_key);
  }
  
  // Determine the text to display
  if(persist_exists(PERSIST_TEMP_UNIT) && persist_read_int(PERSIST_TEMP_UNIT) == 1)
    snprintf(s_tea_text, sizeof(s_tea_text), "%u mins (%u°F)", steep_time / 60, (int) (temp * 1.8 + 32));
  else
    snprintf(s_tea_text, sizeof(s_tea_text), "%u mins (%u°C)", steep_time / 60, temp);
  
  // Draw the cell
  menu_cell_basic_draw(ctx, cell_layer, name, s_tea_text, NULL);
}

/********************/
/* SELECT  HANDLING */
/********************/

// Function called on select press
static void menu_select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context) {
  // Calculate time to wakeup
  int steep_time = tea_array[cell_index->row].def_time;
  int persist_key = tea_array[cell_index->row].persist_key;
  if(persist_exists(persist_key)) {
    steep_time = persist_read_int(persist_key);
  }
  time_t wakeup_time = time(NULL) + steep_time;

  // Schedule the wakeup with the tea ID as reason
  WakeupId s_wakeup_id = wakeup_schedule(wakeup_time, cell_index->row, true);

  // Continue if wakeup event was scheduled
  if (s_wakeup_id > 0) {
    // Store the handle so we can cancel if necessary
    persist_write_int(PERSIST_WAKEUP, s_wakeup_id);
    persist_write_int(PERSIST_DURATION, steep_time);
  
    // Switch to countdown window
    countdown_display(false);
  }
}

/********************/
/*  WINDOW DESTROY  */
/********************/

void menu_mark_dirty() {
  menu_layer_reload_data(s_menu_layer);
}

// Remotely callable function to kill the window
void menu_destroy() {
  window_stack_remove(s_menu_window, true);
}

// Unloading code
static void menu_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  window_destroy(window);
}