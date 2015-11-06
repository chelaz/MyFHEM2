#include <pebble.h>


typedef enum {
  COM_KITCHEN_LIGHT_TOGGLE   =0,
  COM_LIVINGROOMLIGHT_TOGGLE,
  COM_FOOR_RED,
  COM_FOOR_ORANGE,
  COM_FOOR_OFF,
  COM_SLEEPINGROOM_RED,
  COM_SLEEPINGROOM_OFF,
  COM_RADIO_ON,
  COM_ALL_OFF,
  NUM_COM
} Commands_type;


static struct _Feature_Ctrl_Map
{
  long        ID;
  const char* Room;
  const char* Device;
  const char* URL;
} Feature_Ctrl_Map[] = {
  { COM_KITCHEN_LIGHT_TOGGLE,
    "Küche",
    "Licht an/aus",
    "http://mypi:8083/fhem?cmd=set%20FS20_fr_bel%20toggle" },
  { COM_LIVINGROOMLIGHT_TOGGLE,
    "Wohnzimmer",
    "Licht an/aus",
    "http://mypi:8083/fhem?cmd=set%20FS20_fz_bel%20toggle" },
  { COM_FOOR_RED,
    "Flur",
    "rot",
    "http://mypi:8083/fhem?cmd=set%20HueFlur1%20rgb%20FF0000&XHR=1" },
  { COM_FOOR_ORANGE,
    "Flur",
    "orange",
    "http://mypi:8083/fhem?cmd=set%20HueFlur1%20rgb%20FF830A&XHR=1" },
  { COM_FOOR_OFF,
    "Flur",
    "aus",
    "http://mypi:8083/fhem?cmd=set%20HueFlur1%20off&XHR=1" },
  { COM_SLEEPINGROOM_RED,
    "Schlafzimmer",
    "rot",
    "http://mypi:8083/fhem?cmd=set%20HueSchlafzimmer1%20rgb%20FF0000&XHR=1" },
  { COM_SLEEPINGROOM_OFF,
    "Schlafzimmer",
    "aus",
    "http://mypi:8083/fhem?cmd=set%20HueSchlafzimmer1%20off&XHR=1" },
  { COM_RADIO_ON,
    "Radio",
    "an",
    "http://mypi:8083/fhem?cmd=set%20dummy_WebRadio%20on" },
  { COM_ALL_OFF,
    "Alles",
    "aus",
    "http://mypi:8083/fhem?cmd=set%20dummy_all%20off" },
};


static Window *window;
//static TextLayer *text_layer;

/*
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}
*/

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// AppMessage ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {

}
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}




////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// MENU ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#define NUM_MENU_SECTIONS 2
#define NUM_VOICE_MENU_ITEMS 1


// static Window *s_main_window;
static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_COM];
static SimpleMenuItem s_second_menu_items[NUM_VOICE_MENU_ITEMS];
// static GBitmap *s_menu_icon_image;

static bool s_special_flag = false;
static int s_hit_count = 0;

static const uint32_t FHEM_STRING_KEY = 0xabbababe;
static const char *URL = "http://mypi:8083/fhem?cmd=set%20FS20_fr_bel%20toggle";

static void menu_select_callback(int index, void *ctx) {
  // s_first_menu_items[index].subtitle = "You've hit select here!";

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  DictionaryResult Res;
  if ((Res=dict_write_cstring(iter, FHEM_STRING_KEY, Feature_Ctrl_Map[index].URL)) != DICT_OK)
    APP_LOG(APP_LOG_LEVEL_INFO, "Dict write cstring error!");
  else
    app_message_outbox_send();

  return;

  switch(index) {
  case COM_KITCHEN_LIGHT_TOGGLE:
    s_first_menu_items[index].subtitle = "Kitchen toggle";
    APP_LOG(APP_LOG_LEVEL_INFO, "Kitchen toggle");

    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    // dict_write_uint8(iter, 0, 0);

    DictionaryResult Res;
    if ((Res=dict_write_cstring(iter, FHEM_STRING_KEY, URL)) != DICT_OK)
      APP_LOG(APP_LOG_LEVEL_INFO, "Dict write cstring error!");
    else
      app_message_outbox_send();

    break;
  case COM_LIVINGROOMLIGHT_TOGGLE:
    s_first_menu_items[index].subtitle = "Living room toggle";
    break;
  default:
    s_first_menu_items[index].subtitle = "???";
  }

  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void special_select_callback(int index, void *ctx) {
  // Of course, you can do more complicated things in a menu item select callback
  // Here, we have a simple toggle
  s_special_flag = !s_special_flag;

  SimpleMenuItem *menu_item = &s_second_menu_items[index];

  if (s_special_flag) {
    menu_item->subtitle = "Okay, it's not so special.";
  } else {
    menu_item->subtitle = "Well, maybe a little.";
  }

  if (++s_hit_count > 5) {
    menu_item->title = "Very Special Item";
  }

  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void main_window_load(Window *window) {

  for (int i=0; i < NUM_COM; i++) {
    s_first_menu_items[i] = (SimpleMenuItem) {
      .title    = Feature_Ctrl_Map[i].Room,
      .subtitle = Feature_Ctrl_Map[i].Device,
      .callback = menu_select_callback,
    };
  }

  /*
  s_first_menu_items[COM_KITCHEN_LIGHT_TOGGLE] = (SimpleMenuItem) {
    .title    = "Küche",
    .subtitle = "Licht an/aus",
    .callback = menu_select_callback,

  };
  s_first_menu_items[COM_LIVINGROOMLIGHT_TOGGLE] = (SimpleMenuItem) {
    .title    = "Wohnzimmer",
    .subtitle = "Licht an/aus",
    .callback = menu_select_callback,
  };
  s_first_menu_items[COM_FOOR_RED] = (SimpleMenuItem) {
    .title = "Flur",
    .subtitle = "rot",
    .callback = menu_select_callback,
  };
  s_first_menu_items[COM_FOOR_OFF] = (SimpleMenuItem) {
    .title = "Flur",
    .subtitle = "aus",
    .callback = menu_select_callback,
  };
  */

  s_second_menu_items[0] = (SimpleMenuItem) {
    .title = "Dictate",
    .callback = special_select_callback,
  };

  s_menu_sections[0] = (SimpleMenuSection) {
    .title = "Voice Commands",
    .num_items = NUM_VOICE_MENU_ITEMS,
    .items = s_second_menu_items,
  };
  s_menu_sections[1] = (SimpleMenuSection) {
    .title = "Direct Commands",
    .num_items = NUM_COM,
    .items = s_first_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}
////////////////////////////////////////////////////////////////////





/*
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(text_layer, "Press a button");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}
*/

void main_window_unload(Window *window)
{
  simple_menu_layer_destroy(s_simple_menu_layer);
  // gbitmap_destroy(s_menu_icon_image);
}

/*
static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}
*/

static void init(void) {
  window = window_create();
  // window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(app_message_inbox_size_maximum(),
		   app_message_outbox_size_maximum());

}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
