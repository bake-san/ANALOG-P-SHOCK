#include "simple_analog.h"

#include "pebble.h"

#define KEY_SHOW_SECONDS_HAND          0
#define KEY_CONNECTION_LOST_VIBE       1
#define KEY_COLOR_REVERSE              2

#define NUM_CLOCK_TICKS 11

static bool s_connection_lost_vibe = true;
static bool s_show_seconds_hand    = true;
static bool s_color_reverse        = true;

// Color Setting
static GColor s_backColor;
static GColor s_hourHandColor;
static GColor s_minuteHandColor;
static GColor s_secondHandColor;
static GColor s_textColor;


static bool s_connected = false;


static Window *s_window;
static Layer *s_simple_bg_layer, *s_date_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_num_label;

static TextLayer *s_battery_layer;
static TextLayer *s_connection_layer;


static GPath *s_tick_paths[NUM_CLOCK_TICKS];
static GPath *s_minute_arrow, *s_hour_arrow;
static char s_num_buffer[4], s_day_buffer[6];


static void set_color_data(){
  if (s_color_reverse) {
    s_backColor = GColorWhite;
    s_hourHandColor = GColorDarkGray;
    s_minuteHandColor = GColorBlack;
    s_secondHandColor = GColorBlack;
    s_textColor = GColorBlack;
  } else {
    s_backColor = GColorBlack;
    s_hourHandColor = GColorDarkGray;
    s_minuteHandColor = GColorWhite;
    s_secondHandColor = GColorWhite;
    s_textColor = GColorWhite;
  }
  
}

static void set_layer_color(){
  text_layer_set_background_color(s_day_label, s_backColor);
  text_layer_set_background_color(s_num_label, s_backColor);
  text_layer_set_text_color(s_day_label, s_textColor);
  text_layer_set_text_color(s_num_label, s_textColor);
  text_layer_set_text_color(s_battery_layer, s_textColor);
  text_layer_set_text_color(s_connection_layer, s_textColor);

}
static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, s_backColor);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, s_textColor);
  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    const int x_offset = PBL_IF_ROUND_ELSE(18, 0);
    const int y_offset = PBL_IF_ROUND_ELSE(6, 0);
    gpath_move_to(s_tick_paths[i], GPoint(x_offset, y_offset));
    gpath_draw_filled(ctx, s_tick_paths[i]);
  }
}


/************************************ UI **************************************/

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  
  Tuple *show_seconds_hand_t = dict_find(iter, KEY_SHOW_SECONDS_HAND);
  Tuple *connection_lost_vibe_t = dict_find(iter, KEY_CONNECTION_LOST_VIBE);
  Tuple *color_reverse_t = dict_find(iter, KEY_COLOR_REVERSE);

  
  if (show_seconds_hand_t) {
    s_show_seconds_hand = show_seconds_hand_t->value->int8;

    persist_write_bool(KEY_SHOW_SECONDS_HAND, s_show_seconds_hand);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "s_show_seconds_hand %d", s_show_seconds_hand);
    //vibes_short_pulse();
  } 
  
  if (connection_lost_vibe_t) {
    s_connection_lost_vibe = connection_lost_vibe_t->value->int8;

    persist_write_bool(KEY_CONNECTION_LOST_VIBE, s_connection_lost_vibe);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "s_connection_lost_vibe %d", s_connection_lost_vibe);
    vibes_short_pulse();
  } 

  if (color_reverse_t) {
    s_color_reverse = color_reverse_t->value->int8;

    persist_write_bool(KEY_COLOR_REVERSE, s_color_reverse);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "s_color_reverse %d", s_color_reverse);
    set_color_data();
    set_layer_color();
  } 
}



static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);
  const int16_t second_hand_length = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, bounds.size.w / 2);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // minute/hour hand
  // 時
  graphics_context_set_stroke_color(ctx, s_backColor);

  graphics_context_set_fill_color(ctx, s_hourHandColor);
  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  // 分
  graphics_context_set_fill_color(ctx, s_backColor);
  gpath_rotate_to(s_minute_arrow, (TRIG_MAX_ANGLE * (t->tm_min * 10 + (t->tm_sec/6)) / (60 * 10)));
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_stroke_color(ctx, s_minuteHandColor);
  gpath_draw_outline(ctx, s_minute_arrow);

  // 秒
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };
if (s_show_seconds_hand){
    // second hand
    graphics_context_set_stroke_width(ctx, 3);
    graphics_context_set_stroke_color(ctx, s_secondHandColor);
    graphics_draw_line(ctx, second_hand, center);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, s_backColor);
    graphics_draw_line(ctx, second_hand, center);
  }
  // dot in the middle
  // 黒に設定
  graphics_context_set_fill_color(ctx, s_backColor);
  // 円を塗り潰し
  graphics_fill_circle(ctx, center, 7);
  // 線の色を白に設定
  graphics_context_set_stroke_color(ctx, s_secondHandColor);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, center, 7);
  graphics_draw_circle(ctx, center, 1);
  
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_day_buffer, sizeof(s_day_buffer), "%a", t);
  text_layer_set_text(s_day_label, s_day_buffer);

  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  text_layer_set_text(s_num_label, s_num_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}


static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "CHG");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
  }
  text_layer_set_text(s_battery_layer, battery_text);
}


static void handle_bluetooth(bool connected) {
  if (connected) {
    s_connected = true;
  } else {
    if (s_connected) {
      if (s_connection_lost_vibe){
        vibes_short_pulse();
      }
    }
    s_connected = false;
  }
  text_layer_set_text(s_connection_layer, connected ? "OK" : "NG");
}



static void window_load(Window *window) {
  
  set_color_data();
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "---- window_load ----");

  // Config
 //if (persist_read_bool(KEY_CONNECTION_LOST_VIBE)) {
    s_connection_lost_vibe = persist_read_bool(KEY_CONNECTION_LOST_VIBE);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "s_connection_lost_vibe %d", s_connection_lost_vibe);
 /// }
 //if (persist_read_bool(KEY_SHOW_SECONDS_HAND)) {
    s_show_seconds_hand = persist_read_bool(KEY_SHOW_SECONDS_HAND);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "s_show_seconds_hand %d", s_show_seconds_hand);
 // }
 //if (persist_read_bool(KEY_COLOR_REVERSE)) {
    s_color_reverse = persist_read_bool(KEY_COLOR_REVERSE);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "s_color_reverse %d", s_color_reverse);
    set_color_data();
  //}
  
  

  
  s_simple_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_simple_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_simple_bg_layer);

  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);
  layer_add_child(window_layer, s_date_layer);

  //s_day_label = text_layer_create(PBL_IF_ROUND_ELSE(
  //  GRect(63, 114, 27, 20),
  //  GRect(46, 114, 27, 20)));
  s_day_label = text_layer_create(
    GRect((int)(bounds.size.w / 2) + 20, bounds.size.h -98, 27,20));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, s_backColor);
  text_layer_set_text_color(s_day_label, s_textColor);
  text_layer_set_font(s_day_label, fonts_get_system_font(FONT_KEY_GOTHIC_18));

  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));

  //s_num_label = text_layer_create(PBL_IF_ROUND_ELSE(
  //  GRect(90, 114, 18, 20),
  //  GRect(73, 114, 18, 20)));
  s_num_label = text_layer_create(GRect((int)(bounds.size.w / 2) + 47, bounds.size.h -98, 18,20));
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_background_color(s_num_label, s_backColor);
  text_layer_set_text_color(s_num_label, s_textColor);
  text_layer_set_font(s_num_label, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  
  // BT OK/NG
  s_connection_layer = text_layer_create(GRect((int)(bounds.size.w / 2) - 27,114, 20, 20));
  text_layer_set_text_color(s_connection_layer, s_textColor);
  text_layer_set_background_color(s_connection_layer, GColorClear);
  text_layer_set_font(s_connection_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_connection_layer, GTextAlignmentLeft);
  handle_bluetooth(connection_service_peek_pebble_app_connection());

  // Battery
  s_battery_layer = text_layer_create(GRect((int)(bounds.size.w / 2) , 114,40,20));
  text_layer_set_text_color(s_battery_layer, s_textColor);
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentLeft);
  text_layer_set_text(s_battery_layer, "100%");

  battery_state_service_subscribe(handle_battery);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });

  
  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));

  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  layer_add_child(window_layer, text_layer_get_layer(s_connection_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));

  handle_battery(battery_state_service_peek());

}

static void window_unload(Window *window) {

  battery_state_service_unsubscribe();
  connection_service_unsubscribe();

  layer_destroy(s_simple_bg_layer);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);
  text_layer_destroy(s_connection_layer);
  text_layer_destroy(s_battery_layer);

  layer_destroy(s_hands_layer);
}

static void init() {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  s_day_buffer[0] = '\0';
  s_num_buffer[0] = '\0';

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);

  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    s_tick_paths[i] = gpath_create(&ANALOG_BG_POINTS[i]);
  }

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);

  for (int i = 0; i < NUM_CLOCK_TICKS; ++i) {
    gpath_destroy(s_tick_paths[i]);
  }

  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
