#include <pebble.h>

#define CENTER_X    71
#define CENTER_Y    96 // 99 is real centre
#define DOTS_SIZE   4
#define ONE         TRIG_MAX_RATIO
#define THREESIXTY  TRIG_MAX_ANGLE


Window *window;
Layer *layer;
GFont date_font;
GBitmap *background;
GBitmap *bluetooth;
BitmapLayer *bluetooth_layer;
bool bt_connect_toggle;

const GPathInfo HOUR_POINTS = {
  6,
  (GPoint []) {
    { 6,-37},
    { 3,-40},
    {-3,-40},
    {-6,-37},
    {-6,  0},
    { 6,  0},
  }
};
static GPath *hour_path;

const GPathInfo MIN_POINTS = {
  6,
  (GPoint []) {
    { 5,-56},
    { 3,-60},
    {-3,-60},
    {-5,-56},
    {-5,  0},
    { 5,  0},
  }
};
static GPath *min_path;

void bluetooth_connection_handler(bool connected) {
  if (!bt_connect_toggle && connected) {
    bt_connect_toggle = true;
    vibes_short_pulse();
    layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), true);
  }
  if (bt_connect_toggle && !connected) {
    bt_connect_toggle = false;
    vibes_long_pulse();
    layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), false);
  }
}

void update_layer(Layer *me, GContext* ctx) 
{
  BatteryChargeState charge_state =  battery_state_service_peek();
  char text[16];
  char date[3];
  char day[4];
  char month[4];

  //draw watchface
  graphics_draw_bitmap_in_rect(ctx,background,GRect(0,0,144,168));

  //get tick_time
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  //get weekday
  strftime(date, 3, "%d", tick_time);

  if(date[0] == '0'){
    for(int i = 1; i < 3; i++){ //get rid of that leading 0
      date[i - 1] = date[i];      
    }
  }
  strftime(day, 4, "%a", tick_time);
  strftime(month, 4, "%b", tick_time);
  day[1] -= 32;
  day[2] -= 32;
  month[1] -= 32;
  month[2] -= 32;
  snprintf(text, sizeof(text), "%s %s %s", day, date, month);
  
  //draw weekday text
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text(ctx, text,  date_font, GRect(0,-5,144,24), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  
  //draw hands
  GPoint center = GPoint(CENTER_X,CENTER_Y);
  
  graphics_context_set_stroke_color(ctx,GColorWhite);
  graphics_context_set_fill_color(ctx,GColorWhite);
  
  graphics_draw_line(ctx, GPoint(36*1-1,15), GPoint(36*3,15));
  uint8_t bx = 36+ (36*2*charge_state.charge_percent/100);
  graphics_fill_circle(ctx, GPoint(bx-1,15), 2);

  // hours and minutes
  int32_t hour_angle = THREESIXTY * (tick_time->tm_hour * 5 + tick_time->tm_min / 12) / 60;
  int32_t min_angle = THREESIXTY * tick_time->tm_min / 60;
  gpath_rotate_to(hour_path, hour_angle);
  gpath_rotate_to(min_path, min_angle);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, hour_path);
  gpath_draw_outline(ctx, hour_path);
  graphics_draw_circle(ctx, center, DOTS_SIZE+4);
  gpath_draw_filled(ctx, min_path);
  gpath_draw_outline(ctx, min_path);
  graphics_fill_circle(ctx, center, DOTS_SIZE+3);

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, DOTS_SIZE);
}

void tick(struct tm *tick_time, TimeUnits units_changed)
{
  layer_mark_dirty(layer);
}

void init() 
{
  //create window
  window = window_create();
  window_set_background_color(window,GColorBlack);
  window_stack_push(window, true);
  Layer* window_layer = window_get_root_layer(window);  

  //load background
  background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);

  bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  bluetooth_layer = bitmap_layer_create(GRect(67, 92, 9, 9));
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth);
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), true);


  //load font
  date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BTBS_16));

  //create layer
  layer = layer_create(GRect(0,0,144,168));
  layer_set_update_proc(layer, update_layer);
  layer_add_child(window_layer, layer);  
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));

  hour_path = gpath_create(&HOUR_POINTS);
  gpath_move_to(hour_path, GPoint(CENTER_X, CENTER_Y));
  min_path = gpath_create(&MIN_POINTS);
  gpath_move_to(min_path, GPoint(CENTER_X, CENTER_Y));

  //subscribe to seconds tick event
  tick_timer_service_subscribe(MINUTE_UNIT, (TickHandler) tick);
  bluetooth_connection_service_subscribe(bluetooth_connection_handler);
  bt_connect_toggle = bluetooth_connection_service_peek();

}

void deinit() 
{
  gpath_destroy(min_path);
  gpath_destroy(hour_path);
  layer_destroy(layer);
  layer_destroy(bitmap_layer_get_layer(bluetooth_layer));
  fonts_unload_custom_font(date_font);
  gbitmap_destroy(background);
  gbitmap_destroy(bluetooth);
  window_destroy(window);
}

int main()
{
  init();
  app_event_loop();
  deinit();
}