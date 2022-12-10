#define ROW1_KEY 1 //persistant data
#define ROW2_KEY 2
#define ROW3_KEY 3
#define ROW4_KEY 4
#define ROW5_KEY 5
#define ROW6_KEY 6
#define ROW7_KEY 7
#define ROW8_KEY 8
#define TEMPERATURE_KEY 10
#define BLUETOOTH_CONNECTED_KEY 11
#define LCD_OVERRIDE_TEXT_KEY 12 //string
#define RESET_LCD_KEY 13 //bool: true? print default to lcd, false? print overriden text, automatically jumps to true if lcd is overriden

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
static int s_battery_level;
static Layer *s_battery_layer;
static bool resetLcd;//true? print default to lcd, false? print overriden text, automatically jumps to true if lcd is overriden

static char s_matrix_buffer[75];
static char s_lcd_buffer[34];
static char s_date_buffer[17];
static char s_temperature_buffer[17];
static char s_lcd_override_text[34];

char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

static void set_lcd()
{
  //write on lcd
  if (resetLcd) //show default
  {  
    snprintf(s_lcd_buffer, sizeof(s_lcd_buffer), "%s %s", s_date_buffer, s_temperature_buffer); //combine test out
  }
  else //show override
  {
    snprintf(s_lcd_buffer, sizeof(s_lcd_buffer), "%s", s_lcd_override_text); 
  }
  text_layer_set_text(s_lcd_layer, s_lcd_buffer);
}

static void set_matrix(char *row1, char *row2, char *row3, char *row4, char *row5, char *row6, char *row7, char *row8)
{
  memset(s_matrix_buffer, 0, 75); //clear
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

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) { //handles drawing battery layer
  int battery_levels_to_show = (s_battery_level+5)/10; //should create a rounded integer between 0 and 10

  //draw the bars
  graphics_context_set_fill_color(ctx, GColorBrightGreen);
  for(int i = 0; i < battery_levels_to_show; i++)
  {
    graphics_fill_rect(ctx, GRect(27 - 3*i, 0, 2, 7), 0, GCornerNone);
  }
}

static void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  persist_write_bool(BLUETOOTH_CONNECTED_KEY, true); //set to true just to be sure just in case 
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "inbox received");
  //get temperature if sent
  Tuple *temp_t = dict_find(iter, MESSAGE_KEY_temp); //temperature in string with correct unit already
  if (temp_t) 
  {
    snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%s", temp_t->value->cstring);
    //store in persistant data
    persist_write_string(TEMPERATURE_KEY, s_temperature_buffer);
    //write to lcd
    set_lcd();
  }

  //get reset key if sent (we're only interested if it's set to true)
  Tuple *resetLcd_t = dict_find(iter, MESSAGE_KEY_lcdReset);
  if (resetLcd_t)
  {
    bool resetLcd_temp = (bool)(resetLcd_t->value->int32);
    if (resetLcd_temp) //we only register if true
    {
      resetLcd = true;
      persist_write_bool(RESET_LCD_KEY, true);
      set_lcd(); //refresh
    }
  }

  //check for lcd override
  Tuple *lcd_override_text_t = dict_find(iter, MESSAGE_KEY_lcd);
  if (lcd_override_text_t)
  {
    resetLcd = false;
    persist_write_bool(RESET_LCD_KEY, false);
    
    char lcd_row_one[17] = ""; //16 length + null character
    char lcd_row_two[17] = "";
    char *row_two_temp = lcd_override_text_t->value->cstring + 16;
    snprintf(lcd_row_one, sizeof(lcd_row_one), "%s", lcd_override_text_t->value->cstring);

    if (strlen(lcd_override_text_t->value->cstring) > 16)
    {
      snprintf(lcd_row_two, sizeof(lcd_row_two), "%s", row_two_temp);
    }    

    snprintf(s_lcd_override_text, sizeof(s_lcd_override_text), "%s %s ", lcd_row_one, lcd_row_two);
    persist_write_string(LCD_OVERRIDE_TEXT_KEY, s_lcd_override_text);
    set_lcd();
  }


  //Get full 8X8 data if exist
  Tuple *fullMatrix_t = dict_find(iter, MESSAGE_KEY_fullMatrix);

  //Get row data if exist
  Tuple *row_1_t = dict_find(iter, MESSAGE_KEY_Row1);
  Tuple *row_2_t = dict_find(iter, MESSAGE_KEY_Row2);
  Tuple *row_3_t = dict_find(iter, MESSAGE_KEY_Row3);
  Tuple *row_4_t = dict_find(iter, MESSAGE_KEY_Row4);
  Tuple *row_5_t = dict_find(iter, MESSAGE_KEY_Row5);
  Tuple *row_6_t = dict_find(iter, MESSAGE_KEY_Row6);
  Tuple *row_7_t = dict_find(iter, MESSAGE_KEY_Row7);
  Tuple *row_8_t = dict_find(iter, MESSAGE_KEY_Row8);

  char *row_1_s = "000000000";
  char *row_2_s = "000000001"; //apparently the numbers are important else it thinks it's the same address
  char *row_3_s = "000000002";
  char *row_4_s = "000000003";
  char *row_5_s = "000000004";
  char *row_6_s = "000000005";
  char *row_7_s = "000000006";
  char *row_8_s = "000000007";

  if (row_1_t)
  {
    row_1_s = row_1_t ? (row_1_t->value->cstring) : "00000000";
    row_2_s = row_2_t ? (row_2_t->value->cstring) : "00000000";
    row_3_s = row_3_t ? (row_3_t->value->cstring) : "00000000";
    row_4_s = row_4_t ? (row_4_t->value->cstring) : "00000000";
    row_5_s = row_5_t ? (row_5_t->value->cstring) : "00000000";
    row_6_s = row_6_t ? (row_6_t->value->cstring) : "00000000";
    row_7_s = row_7_t ? (row_7_t->value->cstring) : "00000000";
    row_8_s = row_8_t ? (row_8_t->value->cstring) : "00000000";
    //replace 0 with space, since 0 has artifacts
    replace_char(row_1_s, '0', ' ');
    replace_char(row_2_s, '0', ' ');
    replace_char(row_3_s, '0', ' ');
    replace_char(row_4_s, '0', ' ');
    replace_char(row_5_s, '0', ' ');
    replace_char(row_6_s, '0', ' ');
    replace_char(row_7_s, '0', ' ');
    replace_char(row_8_s, '0', ' ');
  }
  else if (fullMatrix_t)
  {
    char *fullMatrix_s = fullMatrix_t->value->cstring;
    replace_char(fullMatrix_s, '0', ' '); //replace 0 with space
    APP_LOG(APP_LOG_LEVEL_DEBUG, "fullmatrix: %s", fullMatrix_s);

    strncpy(row_1_s, fullMatrix_s+0,8);
    strncpy(row_2_s, fullMatrix_s+8,8);
    strncpy(row_3_s, fullMatrix_s+16,8);
    strncpy(row_4_s, fullMatrix_s+24,8);
    strncpy(row_5_s, fullMatrix_s+32,8);
    strncpy(row_6_s, fullMatrix_s+40,8);
    strncpy(row_7_s, fullMatrix_s+48,8);
    strncpy(row_8_s, fullMatrix_s+56,8);
    row_1_s[8] = '\0';
    row_2_s[8] = '\0';
    row_3_s[8] = '\0';
    row_4_s[8] = '\0';
    row_5_s[8] = '\0';
    row_6_s[8] = '\0';
    row_7_s[8] = '\0';
    row_8_s[8] = '\0';
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", row_1_s);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", row_2_s);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", row_3_s);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", row_4_s);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", row_5_s);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", row_6_s);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", row_7_s);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", row_8_s);
  }

  if (row_1_t || fullMatrix_t) //if somehow matrix is being set set the matrix and write it to persistant data
  {
    set_matrix(row_1_s, 
              row_2_s,
              row_3_s,
              row_4_s,
              row_5_s,
              row_6_s,
              row_7_s,
              row_8_s);

    //store persistant data
    persist_write_string(ROW1_KEY, row_1_s);
    persist_write_string(ROW2_KEY, row_2_s);
    persist_write_string(ROW3_KEY, row_3_s);
    persist_write_string(ROW4_KEY, row_4_s);
    persist_write_string(ROW5_KEY, row_5_s);
    persist_write_string(ROW6_KEY, row_6_s);
    persist_write_string(ROW7_KEY, row_7_s);
    persist_write_string(ROW8_KEY, row_8_s);
  }
}

static void update_date()
{
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  strftime(s_date_buffer, sizeof(s_date_buffer), "%A %d %b", tick_time);
  set_lcd(); 
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

  update_date(); //TODO ideally call only on a day change
}

static void tick_handler_min(struct tm *tick_time, TimeUnits units_changed) {
  update_time(); 

  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void tick_handler_day(struct tm *tick_time, TimeUnits units_changed) {
  update_date(); 
}

static void reset_matrix()
{
  bool isBTConnected = persist_exists(BLUETOOTH_CONNECTED_KEY) ? persist_read_bool(BLUETOOTH_CONNECTED_KEY) : true;

  if (isBTConnected)
  {
    //handle matrix persistant data
    if (!persist_exists(ROW1_KEY)) //if no persistant data load default (heart)
    {
      set_matrix("        ", 
                  " 11  11 ", 
                  "11111111",
                  "11111111",
                  "11111111",
                  " 111111 ",
                  "  1111  ",
                  "   11   ");
                  
    }
    else //load persistant data
    {
      char row1_buffer[9];
      char row2_buffer[9];
      char row3_buffer[9];
      char row4_buffer[9];
      char row5_buffer[9];
      char row6_buffer[9];
      char row7_buffer[9];
      char row8_buffer[9];

      persist_read_string(ROW1_KEY, row1_buffer, sizeof(row1_buffer));
      persist_read_string(ROW2_KEY, row2_buffer, sizeof(row2_buffer));
      persist_read_string(ROW3_KEY, row3_buffer, sizeof(row3_buffer));
      persist_read_string(ROW4_KEY, row4_buffer, sizeof(row4_buffer));
      persist_read_string(ROW5_KEY, row5_buffer, sizeof(row5_buffer));
      persist_read_string(ROW6_KEY, row6_buffer, sizeof(row6_buffer));
      persist_read_string(ROW7_KEY, row7_buffer, sizeof(row7_buffer));
      persist_read_string(ROW8_KEY, row8_buffer, sizeof(row8_buffer));

      set_matrix(row1_buffer,
                row2_buffer,
                row3_buffer,
                row4_buffer,
                row5_buffer,
                row6_buffer,
                row7_buffer,
                row8_buffer);
    }
  }
  else //if bt disconnected
  {
     set_matrix("  1     ", 
                "  11   1", 
                "1 1 1  1",
                " 111   1",
                " 111   1",
                "1 1 1   ",
                "  11   1",
                "  1     ");
  }
}

static void bluetooth_callback(bool connected) {
  persist_write_bool(BLUETOOTH_CONNECTED_KEY, connected);
  if (!connected)
  {
      // Issue a vibrating alert
      vibes_double_pulse();
  }
  reset_matrix(); //update led matrix
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
  s_lcd_font    = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SMALL_LCD_9));
  s_matrix_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LEDMATRIX_6));

  // Create the TextLayers with specific bounds
  s_time_layer   = text_layer_create(GRect(-8, 5, bounds.size.w, 50));
  s_lcd_layer    = text_layer_create(GRect(30, 75, 96, 50));
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

  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(104, 111, 29, 7));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Add to Window  
  layer_add_child(/*window_get_root_layer(window)*/window_layer, s_battery_layer);

  //handle inital matrix state
  reset_matrix();

  //handle lcd persistant data
  if(!persist_exists(RESET_LCD_KEY))
  {
    resetLcd = true; //default
  }
  else
  {
    resetLcd = persist_read_bool(RESET_LCD_KEY);
  }
  if(!persist_exists(LCD_OVERRIDE_TEXT_KEY))
  {
    snprintf(s_lcd_override_text, sizeof(s_lcd_override_text), "%s", ""); //empty string if not set
  }
  else
  {
    persist_read_string(LCD_OVERRIDE_TEXT_KEY, s_lcd_override_text, sizeof(s_lcd_override_text));
  }

  //handle temperature persistant data
  if(!persist_exists(TEMPERATURE_KEY))
  {
    snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%s", ""); //empty string if no api key so isn't intrusive
  }
  else
  {
    persist_read_string(TEMPERATURE_KEY, s_temperature_buffer, sizeof(s_temperature_buffer));
    set_lcd(); //gets called in time as well
  }

  // Add Text as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_lcd_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_matrix_layer));
}

static void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_lcd_layer);
  text_layer_destroy(s_matrix_layer);

  // Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_lcd_font);
  fonts_unload_custom_font(s_matrix_font);

  // Destroy bitmap & layer
  gbitmap_destroy(s_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);

  // Destroy battery layer
  layer_destroy(s_battery_layer);
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
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler_min);
  //tick_timer_service_subscribe(DAY_UNIT, tick_handler_day); //can't do both

  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);

  // Make sure the time is displayed from the start
  update_time();
  update_date();

  // Ensure battery level is displayed from the start
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Battery level %d", battery_state_service_peek().charge_percent);
  battery_callback(battery_state_service_peek());

  // Open AppMessage connection
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
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
