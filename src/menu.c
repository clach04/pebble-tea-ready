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

static int get_tea_tount();
static int get_tea_index_by_pos(int);
  
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
  {"Matcha", 30, PERSIST_TEA_MATCHA, 75},
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
  return get_tea_tount();
}

// Get cell height
#ifdef PBL_ROUND
static int16_t menu_cell_height(MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  return 60;
}
#endif

// Display menu entry
static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  int index = get_tea_index_by_pos(cell_index->row);
  char* name = tea_array[index].name;
  int persist_key = tea_array[index].persist_key;
  int steep_time = (persist_read_int(persist_key) != 0 ? persist_read_int(persist_key) : tea_array[index].def_time);
  int temp = tea_array[index].temp;
  char* temp_unit_identifier = "°C";
  
  // Determine the text to display based on temperature unit
  int temp_unit = persist_exists(PERSIST_TEMP_UNIT) ? persist_read_int(PERSIST_TEMP_UNIT) : 0;
  switch(temp_unit) {
    case 1:
      temp = (int) (temp * 1.8 + 32);
      temp_unit_identifier = "°F";
      break;
    case 2:
      temp += 273;
      temp_unit_identifier = "K";
      break;
    case 3:
      temp = (int) ((temp + 273) * 1.8);
      temp_unit_identifier = "°R";
  }
  
  if(steep_time > 60) {
    if(steep_time % 60 == 0)
      snprintf(s_tea_text, sizeof(s_tea_text), "%u mins (%u%s)", steep_time / 60, temp, temp_unit_identifier);
    else
      snprintf(s_tea_text, sizeof(s_tea_text), "%u mins %u secs (%u%s)", steep_time / 60, steep_time % 60, temp, temp_unit_identifier);
  }
  else
    snprintf(s_tea_text, sizeof(s_tea_text), "%u secs (%u%s)", steep_time, temp, temp_unit_identifier);
  
  // Draw the cell
  menu_cell_basic_draw(ctx, cell_layer, name, s_tea_text, NULL);
}

/********************/
/* SELECT  HANDLING */
/********************/

// Function called on select press
static void menu_select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context) {
  int index = get_tea_index_by_pos(cell_index->row);
  int persist_key = tea_array[index].persist_key;
  int steep_time = (persist_read_int(persist_key) != 0 ? persist_read_int(persist_key) : tea_array[index].def_time);
  
  // Calculate time to wakeup
  time_t wakeup_time = time(NULL) + steep_time;

  // Schedule the wakeup with the tea ID as reason
  WakeupId s_wakeup_id = wakeup_schedule(wakeup_time, index, false);

  // Continue if wakeup event was scheduled
  if (s_wakeup_id > 0) {
    // Store information about the countdown in progress
    persist_write_int(PERSIST_WAKEUP, s_wakeup_id);
    persist_write_int(PERSIST_DURATION, steep_time);
    persist_write_int(PERSIST_COUNT_MODE, 0);
    persist_write_int(PERSIST_TEA, index);
  
    // Switch to countdown window
    countdown_display();
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

/********************/
/*  TEA PREFERENCE  */
/********************/

static int get_tea_index_by_pos(int position) {
  int count = 0, i;
  
  // Count options with a time attached
  for(i = 0; i < 9; i++) {
    if(!persist_exists(tea_array[i].persist_key) || persist_read_int(tea_array[i].persist_key) != 0) {
      // Return when the correct entry is found
      if(count == position)
        return i;
      
      count++;
    }
  }
  
  // Fallback value (green tea)
  return 1;
}

static int get_tea_tount() {
  int count = 0, i;
  
  // Count options with a time attached
  for(i = PERSIST_TEA_BLACK; i <= PERSIST_TEA_MATCHA; i++) {
    if(!persist_exists(i) || persist_read_int(i) != 0)
      count++;
  }
  
  // Fallback
  if(count == 0)
    return 1;
  
  return count;
}

int get_tea_temp(int index) {
  return tea_array[index].temp;
}