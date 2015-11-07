#include <pebble.h>

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// FHEM definitions ////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

const char FHEM_URL[] = "http://mypi:8083/fhem";
  
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
  const char* Description;
  const char* URL; // if used
  const char* Device;
  const char* Command;
} Feature_Ctrl_Map[] = {
  { COM_KITCHEN_LIGHT_TOGGLE,
    "Küche",
    "Licht uumschalten", NULL,
    "FS20_fr_bel", "toggle"},
  { COM_LIVINGROOMLIGHT_TOGGLE,
    "Wohnzimmer",
    "Licht an/aus", NULL,
    "FS20_fz_bel", "toggle" },
  { COM_FOOR_RED,
    "Flur",
    "rot", NULL,
   /* "http://mypi:8083/fhem?cmd=set%20HueFlur1%20rgb%20FF0000&XHR=1" },*/
    "HueFlur1", "rgb%20FF0000" },
  { COM_FOOR_ORANGE,
    "Flur",
    "orange",
    "cmd=set%20HueFlur1%20rgb%20FF830A&XHR=1",
    NULL, NULL },
  { COM_FOOR_OFF,
    "Flur",
    "aus",
    "cmd=set%20HueFlur1%20off&XHR=1",
    NULL, NULL },
  { COM_SLEEPINGROOM_RED,
    "Schlafzimmer",
    "rot",
    "cmd=set%20HueSchlafzimmer1%20rgb%20FF0000&XHR=1",
    NULL, NULL },
  { COM_SLEEPINGROOM_OFF,
    "Schlafzimmer",
    "aus",
    "cmd=set%20HueSchlafzimmer1%20off&XHR=1",
    NULL, NULL },
  { COM_RADIO_ON,
    "Radio",
    "an", NULL,
    "FS20_Remote_Radio", "on"},
  { COM_ALL_OFF,
    "Alles",
    "aus", NULL,
    "dummy_all", "off"},
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// AppMessage //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{

}
static void inbox_dropped_callback(AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context)
{
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context)
{
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}




////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// MENU ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#define NUM_MENU_SECTIONS 2
#define NUM_MENU_ITEMS_FAVOURITES 2


static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_COM];
static SimpleMenuItem s_second_menu_items[NUM_MENU_ITEMS_FAVOURITES];
// static GBitmap *s_menu_icon_image;


static const uint32_t FHEM_STRING_KEY = 0xabbababe;

void BuildFhemURL(const int index, char URL[], int size)
{
  if (Feature_Ctrl_Map[index].URL)
    snprintf(URL, size, "%s?%s", FHEM_URL, Feature_Ctrl_Map[index].URL);
  else {
    snprintf(URL, size, "%s?cmd=set%%20%s%%20%s", FHEM_URL, 
             Feature_Ctrl_Map[index].Device, Feature_Ctrl_Map[index].Command);
  }
}

static void menu_select_callback(int index, void *ctx) {
  // s_first_menu_items[index].subtitle = "You've hit select here!";

  char URL[1024];
  BuildFhemURL(index, URL, 1024);
  
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  DictionaryResult Res;
  if ((Res=dict_write_cstring(iter, FHEM_STRING_KEY, URL)) != DICT_OK)
    APP_LOG(APP_LOG_LEVEL_INFO, "Dict write cstring error!");
  else
    app_message_outbox_send();

  
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// TODO: dictate API       /////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
static bool s_special_flag = false;
static int s_hit_count = 0;

static void special_select_callback(int index, void *ctx)
{
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

static void volume_select_callback(int index, void* ctx)
{
  SimpleMenuItem *menu_item = &s_second_menu_items[index];
  menu_item->subtitle = "Not yet implemented";
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// main window load/unload /////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

static void main_window_load(Window *window)
{
  //window_set_background_color(window, GColorYellow);
  for (int i=0; i < NUM_COM; i++) {
    s_first_menu_items[i] = (SimpleMenuItem) {
      .title    = Feature_Ctrl_Map[i].Room,
      .subtitle = Feature_Ctrl_Map[i].Description,
      .callback = menu_select_callback,
    };
  }


  s_second_menu_items[0] = (SimpleMenuItem) {
    .title = "Dictate",
    .callback = special_select_callback,
  };
  s_second_menu_items[1] = (SimpleMenuItem) {
    .title = "Volume",
    .callback = volume_select_callback,
  };

  s_menu_sections[0] = (SimpleMenuSection) {
    .title = "Favourites",
    .num_items = NUM_MENU_ITEMS_FAVOURITES,
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


void main_window_unload(Window *window)
{
  simple_menu_layer_destroy(s_simple_menu_layer);
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// init / deinit / main ////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

static Window *s_window;

static void init(void) {
  s_window = window_create();

  window_set_window_handlers(s_window, (WindowHandlers) {
    .load   = main_window_load,
    .unload = main_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(app_message_inbox_size_maximum(),
		   app_message_outbox_size_maximum());

}

static void deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  deinit();
}
