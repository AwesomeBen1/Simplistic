#include "pebble.h"

static Window *s_main_window;
static TextLayer *s_day_layer, *s_date_layer, *s_time_layer;
static Layer *s_line_layer;
static BitmapLayer *s_bt_layer;
static GBitmap *s_bt_on_bitmap, *s_bt_off_bitmap;

static void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char s_time_text[] = "00:00";
  static char s_date_text[] = "Xxxxxxxxx 00";
	static char s_day_text[] = "Xxxxxxxxx";

	// Date is currently updated every minute, should be changed later.
	strftime(s_day_text, sizeof(s_day_text), "%A,", tick_time);
  text_layer_set_text(s_day_layer, s_day_text);
	
  strftime(s_date_text, sizeof(s_date_text), "%B %e", tick_time);
  text_layer_set_text(s_date_layer, s_date_text);

  char *time_format;
  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }
  strftime(s_time_text, sizeof(s_time_text), time_format, tick_time);

  // Handle lack of non-padded hour format string for twelve hour clock.
  if (!clock_is_24h_style() && (s_time_text[0] == '0')) {
    memmove(s_time_text, &s_time_text[1], sizeof(s_time_text) - 1);
  }
  text_layer_set_text(s_time_layer, s_time_text);
}

static void bt_handler(bool connected) {
  // Show current connection state
  if (connected) {
    bitmap_layer_set_bitmap(s_bt_layer, s_bt_on_bitmap);
  } else {
    bitmap_layer_set_bitmap(s_bt_layer, s_bt_off_bitmap);
  }
	vibes_short_pulse();
	light_enable_interaction();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

	// Day of week
	s_day_layer = text_layer_create(GRect(8, 44, 144-8, 168-44));
  text_layer_set_text_color(s_day_layer, GColorWhite);
  text_layer_set_background_color(s_day_layer, GColorClear);
  text_layer_set_font(s_day_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
	
	// Month and day
  s_date_layer = text_layer_create(GRect(8, 68, 144-8, 168-68));
  text_layer_set_text_color(s_date_layer, GColorWhite);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

	// The time
  s_time_layer = text_layer_create(GRect(7, 92, 144-7, 168-92));
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

	// Line seperating time and date
  s_line_layer = layer_create(GRect(8, 97, 144-20, 2));
  layer_set_update_proc(s_line_layer, line_layer_update_callback);
  layer_add_child(window_layer, s_line_layer);
	
	// Bluetooth indicator
	s_bt_on_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_ON);
	s_bt_off_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_OFF);
	s_bt_layer = bitmap_layer_create(GRect(5, 3, 12, 11));
	if (bluetooth_connection_service_peek()) {
  bitmap_layer_set_bitmap(s_bt_layer, s_bt_on_bitmap);
  } else {
  bitmap_layer_set_bitmap(s_bt_layer, s_bt_off_bitmap);
  }
  bitmap_layer_set_alignment(s_bt_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bt_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_day_layer);
	text_layer_destroy(s_date_layer);
  text_layer_destroy(s_time_layer);
	layer_destroy(s_line_layer);
	bitmap_layer_destroy(s_bt_layer);
  gbitmap_destroy(s_bt_on_bitmap);
	gbitmap_destroy(s_bt_off_bitmap);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);

	bluetooth_connection_service_subscribe(bt_handler);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  
  // Prevent starting blank
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  handle_minute_tick(t, MINUTE_UNIT);
}

static void deinit() {
  window_destroy(s_main_window);

  tick_timer_service_unsubscribe();
}

int main() {
  init();
  app_event_loop();
  deinit();
}