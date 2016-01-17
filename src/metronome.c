#include <pebble.h>

#define RATE 30 // button refresh rate
#define MIN 16 // minimum tempo
#define MAX 512 // maximum tempo
#define SHORT 64 // short vibration length
#define LONG 96 // long vibration length
#define THRESHOLD 256
#define INCREMENT 2
#define INITIAL 128
#define TEMPO_KEY 0

static Window *main_window;
static TextLayer *status_layer;
static TextLayer *output_layer;
static TextLayer *bpm_layer;
static AppTimer *timer;

static int tempo;
static char tempo_string [4];
static bool state;

static const VibePattern vibe_short = {
  .durations = (uint32_t []) {SHORT},
  .num_segments = 1
};
static const VibePattern vibe_long = {
  .durations = (uint32_t []) {LONG},
  .num_segments = 1
};
static VibePattern vibe;

static void metronome_loop() {
  timer = app_timer_register(60000 / tempo, metronome_loop, NULL);
  if (state) {  
    vibes_enqueue_custom_pattern(vibe);
  }
}

static void set_tempo(int new) {
  snprintf(tempo_string, sizeof tempo_string, "%d", new);
  text_layer_set_text(output_layer, tempo_string);
  tempo = new;
  if (tempo >= THRESHOLD) {
    vibe = vibe_short;
  } else {
    vibe = vibe_long;
  }
}
static void set_state(bool new) {
  state = new;
  if (state) {
    text_layer_set_text(status_layer, "ON");
    text_layer_set_text_color(status_layer, GColorGreen); 
  } else {
    text_layer_set_text(status_layer, "OFF");
    text_layer_set_text_color(status_layer, GColorRed);
    vibes_cancel();
  } 
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (tempo < MAX) {
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

static void click_config_provider(void *context) {
  // register the click handlers
  window_single_repeating_click_subscribe(BUTTON_ID_UP, RATE, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, RATE, down_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  status_layer = text_layer_create(GRect(0, window_bounds.size.h / 2 - (42 / 2) - 28 - 5, window_bounds.size.w, 42));
  text_layer_set_font(status_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(status_layer, GColorClear);
  text_layer_set_text_alignment(status_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(status_layer));
  
  output_layer = text_layer_create(GRect(0, window_bounds.size.h / 2 - (42 / 2) - 5, window_bounds.size.w, 42));
  text_layer_set_font(output_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_background_color(output_layer, GColorClear);
  text_layer_set_text_color(output_layer, GColorWhite);
  text_layer_set_text_alignment(output_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(output_layer));
  
  bpm_layer = text_layer_create(GRect(0, window_bounds.size.h / 2 + (42 / 2) - 5, window_bounds.size.w, 42));
  text_layer_set_font(bpm_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(bpm_layer, GColorClear);
  text_layer_set_text_color(bpm_layer, GColorWhite);
  text_layer_set_text_alignment(bpm_layer, GTextAlignmentCenter);
  text_layer_set_text(bpm_layer, "BPM");
  layer_add_child(window_layer, text_layer_get_layer(bpm_layer));
  
  if (persist_exists(TEMPO_KEY)) {
    set_tempo(persist_read_int(TEMPO_KEY));
  } else {
    set_tempo(INITIAL);
  }
  
  set_state(true);
  metronome_loop();
}

static void main_window_unload(Window *window) {
  // destroy text layers
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
  window_set_click_config_provider(main_window, click_config_provider);
  window_stack_push(main_window, true);
}

static void deinit() {
  vibes_cancel();
  app_timer_cancel(timer);
  
  persist_write_int(TEMPO_KEY, tempo); // save for later
  
  window_destroy(main_window); // destroy main window
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}