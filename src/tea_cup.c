#include "tea_cup.h"

/********************/
/*    VARIABLES     */
/********************/

// Display variable
static GPath *s_tea_cup = NULL;
static GPath *s_tea_cup_fill = NULL;
static GPath *s_plate = NULL;
static GPath *s_handle = NULL;

// Path
static GPathInfo s_path_tea_cup = {
  .num_points = 6,
  .points = (GPoint []) {
    {0, 0}, // Cup - top left
    {0, 25}, // Cup - centre left
    {15, 40}, // Cup - bottom left
    {35, 40}, // Cup - bottom right
    {50, 25}, // Cup - centre right
    {50, 0} // Cup - top right
  }
};
static GPathInfo s_path_tea_cup_fill = {
  .num_points = 6,
  .points = (GPoint []) {
    {0, 0}, // Cup - top left
    {0, 25}, // Cup - centre left
    {15, 40}, // Cup - bottom left
    {35, 40}, // Cup - bottom right
    {50, 25}, // Cup - centre right
    {50, 0} // Cup - top right
  }
};
static GPathInfo s_path_plate = {
  .num_points = 4,
  .points = (GPoint []) {
    {0, 0}, // Plate - top left
    {6, 6}, // Plate - bottom left
    {44, 6}, // Plate - bottom right
    {50, 0} // Plate - top right
  }
};
static GPathInfo s_path_handle = {
  .num_points = 6,
  .points = (GPoint []) {
    {12, 0}, // Handle - top right
    {4, 0}, // Handle - top top left
    {0, 4}, // Handle - center top left
    {0, 14}, // Handle - center bottom left
    {4, 18}, // Plate - bottom bottom left
    {12, 18} // Plate - bottom right
  }
};

// Other variables
static bool s_tea_cup_loaded;

/********************/
/*     DISPLAY      */
/********************/

// Draw tea cup on context
void tea_cup_draw(Layer *layer, GContext *ctx, uint8_t fill_percentage) {
  GRect bounds = layer_get_bounds(layer);
  if(fill_percentage < 100)
    fill_percentage = fill_percentage / 100.0 * 36 + 2;
  else
    fill_percentage = 40;
  
  // Initialize the tea cup
  if(!s_tea_cup_loaded) {
    s_tea_cup_loaded = true;
    
    // Create path
    s_tea_cup = gpath_create(&s_path_tea_cup);
    s_tea_cup_fill = gpath_create(&s_path_tea_cup_fill);
    s_plate = gpath_create(&s_path_plate);
    s_handle = gpath_create(&s_path_handle);
  
    // Translate to centre
    gpath_move_to(s_tea_cup, GPoint(bounds.size.w / 2 - 25, bounds.size.h / 2 - 23));
    gpath_move_to(s_tea_cup_fill, GPoint(bounds.size.w / 2 - 25, bounds.size.h / 2 - 23));
    gpath_move_to(s_plate, GPoint(bounds.size.w / 2 - 25, bounds.size.h / 2 + 17));
    gpath_move_to(s_handle, GPoint(bounds.size.w / 2 - 37, bounds.size.h / 2 - 20));
  }
  
  // Setup fill path
  if(fill_percentage <= 15) {
    // Remove top points
    s_path_tea_cup_fill.points[0] = GPoint(15 - fill_percentage, 40 - fill_percentage);
    s_path_tea_cup_fill.points[5] = GPoint(35 + fill_percentage, 40 - fill_percentage);
    
    // Set centre points correctly
    s_path_tea_cup_fill.points[1] = GPoint(15 - fill_percentage, 40 - fill_percentage);
    s_path_tea_cup_fill.points[4] = GPoint(35 + fill_percentage, 40 - fill_percentage);
  }
  else {
    // Set top points correctly
    s_path_tea_cup_fill.points[0] = GPoint(0, 40 - fill_percentage);
    s_path_tea_cup_fill.points[5] = GPoint(50, 40 - fill_percentage);
    
    // Set centre points to default
    s_path_tea_cup_fill.points[1] = GPoint(0, 25);
    s_path_tea_cup_fill.points[4] = GPoint(50, 25);
  }
  
  // Fill the cup
  graphics_context_set_fill_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, s_tea_cup);
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, s_plate);
  gpath_draw_filled(ctx, s_tea_cup_fill);
  
  // Outline the cup
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);
  gpath_draw_outline(ctx, s_plate);
  gpath_draw_outline(ctx, s_tea_cup);
  gpath_draw_outline(ctx, s_handle);
}

// Destroy the tea cup
void tea_cup_destroy() {
  if(s_tea_cup_loaded) {
    gpath_destroy(s_tea_cup);
    gpath_destroy(s_tea_cup_fill);
    gpath_destroy(s_plate);
    gpath_destroy(s_handle);
    
    s_tea_cup_loaded = false;
  }
}