#include <pebble.h>

#define PERSIST_KEY_TIMES_OPENED 0

static Window *s_window;
static TextLayer *s_text_layer;

static int s_opened = 0;
static char s_message[32];

static void prv_update_launch_data() {
  // Read and increment the number of times we've opened the app
  s_opened = persist_read_int(PERSIST_KEY_TIMES_OPENED) + 1;

  // Write the new value back to the persistent storage
  persist_write_int(PERSIST_KEY_TIMES_OPENED, s_opened);

  // Generate the display string
  snprintf(s_message, 32, "Opened app %d time%c", s_opened, s_opened == 1 ? '\0' : 's');
}

static void prv_update_app_glance(AppGlanceReloadSession *session, size_t limit,
                                                                  void *context) {
  // This should never happen, but developers should always ensure
  // they’re not adding more app glance slices than the limit
  if (limit < 1) return;

  // Cast the context object to a string
  const char *message = (char *)context;

  // Create the AppGlanceSlice
  // When layout.icon_resource_id is not set, the app's default icon is used
  const AppGlanceSlice entry = (AppGlanceSlice) {
    .layout = {
      .icon_resource_id = APP_GLANCE_SLICE_DEFAULT_ICON,
      .template_string = message
    },
    // TODO: Change to APP_GLANCE_SLICE_NO_EXPIRATION in SDK 4-dp2
    .expiration_time = time(NULL)+3600
  };

  // Add the slice, and check the result
  const AppGlanceResult result = app_glance_add_slice(session, entry);
  if (result != APP_GLANCE_RESULT_SUCCESS) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "AppGlance Error: %d", result);
  }
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Center the text layer, and display how many times we've opened the app
  s_text_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 10, bounds.size.w, 20));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_text_layer, s_message);

  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void prv_init(void) {
  prv_update_launch_data();

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });

  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  window_destroy(s_window);

  // As a best practice, app_glance_reload should typically be one
  // of the last things an app does before exiting.
  app_glance_reload(prv_update_app_glance, s_message);
}

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
