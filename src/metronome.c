#include <pebble.h>

#define RATE 30 // button refresh rate
#define MIN 16 // minimum tempo
#define MAX 512 // maximum tempo
#define INCREMENT 2 // how much tempo changes on button press

#define INITIAL_TEMPO 128
#define INITIAL_VIBE_DURATION 48

#define KEY_TEMPO 0 // for persistent storage and app_message
#define KEY_VIBE_DURATION 1

#define FORCE_COLOR false

static Window *main_window; // ouch! these right here sure are pointy
static TextLayer *status_layer;
static TextLayer *output_layer;
static TextLayer *bpm_layer;
static AppTimer *timer;

static int tempo; // variables and things
static char tempo_string [4];
static bool state;
static bool color;

static VibePattern vibe; // variables for vibrations
static uint32_t vibe_segments [1];
static int vibe_duration;

static void metronome_loop() { // runs every beat
  timer = app_timer_register(60000 / tempo, metronome_loop, NULL);
  if(state) {
    vibes_enqueue_custom_pattern(vibe);
  }
}

static bool get_color() { // is color supported?
  if (FORCE_COLOR) {
    return true;
  } else {
    switch(watch_info_get_model()) {
      case WATCH_INFO_MODEL_PEBBLE_ORIGINAL:
        return false;
      case WATCH_INFO_MODEL_PEBBLE_STEEL:
        return false;
      case WATCH_INFO_MODEL_UNKNOWN:
        return false;
      default:
        return true;
    }
  }
}

static void set_vibe(int vibe_duration) {
  vibe_segments[0] = (uint32_t) {vibe_duration};
  vibe = (VibePattern) {
    .durations = vibe_segments,
    .num_segments = 1
  };
}
static void set_tempo(int new) {
  snprintf(tempo_string, sizeof tempo_string, "%d", new);
  text_layer_set_text(output_layer, tempo_string);
  tempo = new;
}
static void set_state(bool new) {
  state = new;
  text_layer_set_text(status_layer, (state ? "ON" : "OFF"));
  if(color) {
    text_layer_set_text_color(status_layer, (state ? GColorGreen : GColorRed));
  } else {
    text_layer_set_text_color(status_layer, GColorWhite);
  }
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) { // i have no idea what i'm doing here, please fork
  Tuple *tuple_vibe_duration = dict_find(iterator, KEY_VIBE_DURATION);
  if(tuple_vibe_duration) {
    vibe_duration = atoi(tuple_vibe_duration->value->cstring);
  }
  set_vibe(vibe_duration);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) { // click handlers
  if(tempo < MAX) {
    set_tempo(tempo + INCREMENT);
  }
}
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  set_state(!state);
}
static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (tempo > MIN) {
    set_tempo(tempo - INCREMENT);
  }
}

static void click_config_provider(void *context) { // register the click handlers
  window_single_repeating_click_subscribe(BUTTON_ID_UP, RATE, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, RATE, down_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // status layer
  status_layer = text_layer_create(GRect(0, window_bounds.size.h / 2 - (42 / 2) - 28 - 5, window_bounds.size.w, 42));
  text_layer_set_font(status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(status_layer, GColorClear);
  text_layer_set_text_alignment(status_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(status_layer));
  
  // output layer
  output_layer = text_layer_create(GRect(0, window_bounds.size.h / 2 - (42 / 2) - 5, window_bounds.size.w, 42));
  text_layer_set_font(output_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_background_color(output_layer, GColorClear);
  text_layer_set_text_color(output_layer, GColorWhite);
  text_layer_set_text_alignment(output_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(output_layer));
  
  // bpm layer
  bpm_layer = text_layer_create(GRect(0, window_bounds.size.h / 2 + (42 / 2) - 5, window_bounds.size.w, 42));
  text_layer_set_font(bpm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(bpm_layer, GColorClear);
  text_layer_set_text_color(bpm_layer, GColorWhite);
  text_layer_set_text_alignment(bpm_layer, GTextAlignmentCenter);
  text_layer_set_text(bpm_layer, "BPM");
  layer_add_child(window_layer, text_layer_get_layer(bpm_layer));

  // read stuff
  vibe_duration = (persist_exists(KEY_VIBE_DURATION) ? persist_read_int(KEY_VIBE_DURATION) : INITIAL_VIBE_DURATION);
  set_vibe(vibe_duration);
  set_tempo(persist_exists(KEY_TEMPO) ? persist_read_int(KEY_TEMPO) : INITIAL_TEMPO);
  
  color = get_color();
  set_state(true);
  metronome_loop();
}

static void main_window_unload(Window *window) { // destroy text layers
  text_layer_destroy(output_layer);
  text_layer_destroy(bpm_layer);
  text_layer_destroy(status_layer);
}

static void init() {
  // create main window
  main_window = window_create();
  window_set_background_color(main_window, GColorBlack);
  window_set_window_handlers(main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  window_set_click_config_provider(main_window, click_config_provider);
  window_stack_push(main_window, true);
}

static void deinit() {
  vibes_cancel();
  app_timer_cancel(timer);
  
  persist_write_int(KEY_TEMPO, tempo); // save for later
  persist_write_int(KEY_VIBE_DURATION, vibe_duration);
  
  window_destroy(main_window); // destroy main window
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}