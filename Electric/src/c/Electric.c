#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_lcd_layer; //16x2 lcd
static TextLayer *s_matrix_layer;
static BitmapLayer *s_bitmap_layer;
static GFont s_time_font;
static GFont s_lcd_font;
static GFont s_matrix_font;
static GBitmap *s_bitmap;

static void set_matrix(char *row1, char *row2, char *row3, char *row4, char *row5, char *row6, char *row7, char *row8)
{
  static char s_matrix_buffer[75];
  strcat(s_matrix_buffer, row1);
  strcat(s_matrix_buffer, " "); //extra space is neccesary to add breaks between "words" to prevent bad wrapping
  strcat(s_matrix_buffer, row2);
  strcat(s_matrix_buffer, " ");
  strcat(s_matrix_buffer, row3);
  strcat(s_matrix_buffer, " ");
  strcat(s_matrix_buffer, row4);
  strcat(s_matrix_buffer, " ");
  strcat(s_matrix_buffer, row5);
  strcat(s_matrix_buffer, " ");
  strcat(s_matrix_buffer, row6);
  strcat(s_matrix_buffer, " ");
  strcat(s_matrix_buffer, row7);
  strcat(s_matrix_buffer, " ");
  strcat(s_matrix_buffer, row8);
  strcat(s_matrix_buffer, " ");

  text_layer_set_text(s_matrix_layer, s_matrix_buffer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H%M" : "%I%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);

  //deal with date
  static char s_date_buffer[16];
  strftime(s_date_buffer, sizeof(s_date_buffer), "%A %d %b", tick_time);
  text_layer_set_text(s_lcd_layer, s_date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create GBitmap for background
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BG_IMAGE);

  // Create bitmap layer
  s_bitmap_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));

  // Create GFont
  s_time_font   = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PIXEL_LCD_40));
  s_lcd_font    = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SMALL_LCD_8));
  s_matrix_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LEDMATRIX_6));

  // Create the TextLayers with specific bounds
  s_time_layer   = text_layer_create(GRect(-8, 5, bounds.size.w, 50));
  s_lcd_layer    = text_layer_create(GRect(30, 76, 96, 50));
  s_matrix_layer = text_layer_create(GRect(6, 113, 50, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBrightGreen);
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  //handle 16x2 lcd text layer
  text_layer_set_background_color(s_lcd_layer, GColorClear);
  text_layer_set_text_color(s_lcd_layer, /*GColorMidnightGreen*/ GColorBlack);
  text_layer_set_font(s_lcd_layer, s_lcd_font);
  text_layer_set_text_alignment(s_lcd_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(s_lcd_layer, GTextOverflowModeWordWrap);
  //text_layer_set_text(s_lcd_layer, "Sunday 20 Nov   7.22'C"); //TODO test

  //handle matrix text layer
  text_layer_set_background_color(s_matrix_layer, GColorClear);
  text_layer_set_text_color(s_matrix_layer, GColorBrightGreen);
  text_layer_set_font(s_matrix_layer, s_matrix_font);
  text_layer_set_text_alignment(s_matrix_layer, GTextAlignmentLeft);
  text_layer_set_overflow_mode(s_matrix_layer, GTextOverflowModeWordWrap);
  set_matrix("        ", 
             " 11  11 ", 
             "11111111",
             "11111111",
             "11111111",
             " 111111 ",
             "  1111  ",
             "   11   ");

  // Add Text as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_lcd_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_matrix_layer));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_lcd_layer);

  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_lcd_font);

  // Destroy bitmap & layer
  gbitmap_destroy(s_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Make sure the time is displayed from the start
  update_time();
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
