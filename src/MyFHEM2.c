#include <pebble.h>

////////////////////////////////////////////////////////////////////
// Documentation ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// Vibe pulses:
//   -       command successfully sent to server via menu (no vibe)
//   short:  command successfully sent to server via dictation
//   long:   send command: failed (general error)
//           dictate: (general error)
//   double: dictate: command not found

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// FHEM definitions ////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


// Compile time features
#ifdef PBL_SDK_3
#  define ENABLE_DICTATION
#endif



const char FHEM_URL[] = "http://mypi:8083/fhem";


static struct _Coms_Map
{
  const char* Room;
  const char* Description;
  const char* URL; // if used
  const char* Device;
  const char* Command;
} Coms_Map[] = {
  {
    "Küche",
    "Licht umschalten", NULL,
    "FS20_fr_bel", "toggle"},
  {
    "Wohnzimmer",
    "Licht an/aus", NULL,
    "FS20_fz_bel", "toggle" },
  {
    "Flur",
    "rot", NULL,
   /* "http://mypi:8083/fhem?cmd=set%20HueFlur1%20rgb%20FF0000&XHR=1" },*/
    "HueFlur1", "rgb%20FF0000" },
  {
    "Flur",
    "orange",
    "cmd=set%20HueFlur1%20rgb%20FF830A&XHR=1",
    NULL, NULL },
  {
    "Flur",
    "aus",
    "cmd=set%20HueFlur1%20off&XHR=1",
    NULL, NULL },
  {
    "Schlafzimmer",
    "rot",
    "cmd=set%20HueSchlafzimmer1%20rgb%20FF0000&XHR=1",
    NULL, NULL },
  {
    "Schlafzimmer",
    "aus",
    "cmd=set%20HueSchlafzimmer1%20off&XHR=1",
    NULL, NULL },
  {
    "Lautstärke",
    "lauter", NULL,
    "FS20_0000_VolUp", "on"},
  {
    "Lautstärke",
    "leiser", NULL,
    "FS20_0001_VolDown", "on"},
  {
    "Radio",
    "leiser",
    "cmd=%22mpc.sh+-q+-h+lora+volume+-10%22",
    NULL, NULL },
  {
    "Radio",
    "lauter",
    "cmd=%22mpc.sh+-q+-h+lora+volume+%2B10%22",
    NULL, NULL },
  {
    "Radio",
    "an", NULL,
    "FS20_Remote_Radio", "on"},
  {
    "Küche",
    "Licht an", NULL,
    "FS20_fr_bel", "on"},
  {
    "Küche",
    "Licht aus", NULL,
    "FS20_fr_bel", "off"},
  {
    "Alles",
    "aus", NULL,
    "dummy_all", "off"},
};

int GetNumComs()
{
  return sizeof(Coms_Map) / sizeof(struct _Coms_Map);
}

typedef enum _StatusIconType {
  UNKNOWN = -1,
  FAILED  = 0,
  OK      = 1,
  SEND    = 2,
  OFF     = 3,
  ON      = 4,  
} StatusIconType;

typedef enum _MenuType {
  MENU_UNKNOWN     = -1,
  MENU_FAVOURITES  = 0,
  MENU_DIRECT_COMS = 1,
} MenuType;




// forward declaration
bool set_menu_icon(MenuType Menu, int index, StatusIconType Status);


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// AppMessage //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
static const uint32_t FHEM_URL_KEY       = 0;
static const uint32_t FHEM_COM_ID_KEY    = 1;
static const uint32_t FHEM_RESP_KEY      = 2;
static const uint32_t FHEM_URL_GET_STATE = 3;

static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{  
  Tuple *data;

  int index = -1;
  if ((data=dict_find(iterator, FHEM_COM_ID_KEY)) != NULL) {
    index = (int)data->value->int32;
  }

  if ((data = dict_find(iterator, FHEM_RESP_KEY)) != NULL) {
    char* result = (char*)data->value->cstring;
    APP_LOG(APP_LOG_LEVEL_INFO, "FHEM_RESP_KEY received: %s of index %d", result, index);
    if (!strcmp("success", result)) {
      set_menu_icon(MENU_DIRECT_COMS, index, OK);
    } else if (!strcmp("off", result)) {
      set_menu_icon(MENU_DIRECT_COMS, index, OFF);
      vibes_long_pulse();
    } else if (!strcmp("on", result)) {
      set_menu_icon(MENU_DIRECT_COMS, index, ON);
      vibes_long_pulse();
    } else {
      set_menu_icon(MENU_DIRECT_COMS, index, FAILED);
      vibes_long_pulse();
    } 
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "FHEM_RESP_KEY not received.");
  }
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
// FHEM ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void BuildFhemURL(const int index, char URL[], int size)
{
  if (Coms_Map[index].URL)
    snprintf(URL, size, "%s?%s", FHEM_URL, Coms_Map[index].URL);
  else {
    snprintf(URL, size, "%s?cmd=set%%20%s%%20%s", FHEM_URL, 
             Coms_Map[index].Device, Coms_Map[index].Command);
  }
}

void BuildFhemStatusURL(const int index, char URL[], int size)
{
  snprintf(URL, size, "%s?cmd=jsonlist%%20%s", FHEM_URL, 
	   Coms_Map[index].Device);
}


bool SendCom(int index, bool requestStatus)
{
  char URL[1024];
  
  AppMessageResult Res = APP_MSG_OK;
    
  DictionaryIterator *iter;
  if ((Res=app_message_outbox_begin(&iter)) != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_outbox_begin: %d", Res);   
    return false;
  }

  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error creating outbound message!");
    return false;
  }

  DictionaryResult ResDict;
  if (!requestStatus) {
    BuildFhemURL(index, URL, 1024);
    
    if ((ResDict=dict_write_cstring(iter, FHEM_URL_KEY, URL)) != DICT_OK) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Dict write url error: %d!", ResDict);
      return false;
    }
  } else {
    BuildFhemStatusURL(index, URL, 1024);
    
    if ((ResDict=dict_write_cstring(iter, FHEM_URL_GET_STATE, URL)) != DICT_OK) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Dict write status url error: %d!", ResDict);
      return false;
    }
  }
  
  
  if ((ResDict=dict_write_int(iter, FHEM_COM_ID_KEY, &index, sizeof(int), true)) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Dict write com id error: %d", ResDict);
    return false;
  }
  
  if (app_message_outbox_send() != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_outbox_send: %d", Res);   
    return false;
  }
  return true;
}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// MENU ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


// icons moved from here
// here ok?
#define NUM_MENU_SECTIONS 2
#define NUM_MENU_ITEMS_FAVOURITES 2
#define NUM_COM 32

static GBitmap *s_menu_icon_image_ok;
static GBitmap *s_menu_icon_image_failed;
static GBitmap *s_menu_icon_image_send;
static GBitmap *s_menu_icon_image_off;
static GBitmap *s_menu_icon_image_on;
static SimpleMenuItem s_first_menu_items[NUM_COM];
static SimpleMenuItem s_favourites_menu_items[NUM_MENU_ITEMS_FAVOURITES];
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuLayer *s_simple_menu_layer;


// forward declaration
bool set_menu_icon(MenuType Menu, int index, StatusIconType Status)
{
  SimpleMenuItem* pMenu = NULL;
  
  switch(Menu) {
    default:
    case MENU_UNKNOWN:
      return false;
    case MENU_FAVOURITES:
      pMenu = &s_favourites_menu_items[index];
      break;
    case MENU_DIRECT_COMS:
      pMenu = &s_first_menu_items[index];
      break;   
  };

  if (!pMenu)
    return false;
  
  switch(Status) {
    default:
    case UNKNOWN:
      return false;
    case FAILED:
      pMenu->icon = PBL_IF_RECT_ELSE(s_menu_icon_image_failed, NULL);
      break;
    case OK:
      pMenu->icon = PBL_IF_RECT_ELSE(s_menu_icon_image_ok, NULL);
      break;
    case SEND:
      pMenu->icon = PBL_IF_RECT_ELSE(s_menu_icon_image_send, NULL);
      break;
    case OFF:
      pMenu->icon = PBL_IF_RECT_ELSE(s_menu_icon_image_off, NULL);
      break;
    case ON:
      pMenu->icon = PBL_IF_RECT_ELSE(s_menu_icon_image_on, NULL);
      break;
  }
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
 
  return true;
}

// todo: for tests
static bool s_special_flag = false;

static void menu_select_callback(int index, void *ctx)
{
  // s_first_menu_items[index].subtitle = "You've hit select here!";

#if 0
  GRect invisible_rect = GRect(0, 0, 5, 5);
  GRect visible_rect = GRect(10, 0, 5, 5);

  // test
  // .a = 1
  GColor invisible = (GColor){
    .a = 0b01,
    .r = 0b11,
    .g = 0b10,
    .b = 0b00
  };

  // .a = 2
  GColor visible = (GColor){ .argb = 0b10111000 };

  // Fill with these colors
  graphics_context_set_fill_color(ctx, invisible);
  graphics_fill_rect(ctx, invisible_rect, GCornerNone, 0);
  graphics_context_set_fill_color(ctx, visible);
  graphics_fill_rect(ctx, visible_rect, GCornerNone, 0);

  // Draw outlines
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_rect(ctx, invisible_rect);
  graphics_draw_rect(ctx, visible_rect);
#endif
  

  if (SendCom(index, s_special_flag))
    set_menu_icon(MENU_DIRECT_COMS, index, SEND);
  else
    set_menu_icon(MENU_DIRECT_COMS, index, FAILED);
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// dictate API             /////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

int ParseText(char DictationText[], char* Words[], int MaxNumWords)
{
  char* p, *pb = 0;
  
  // char* Words[64];
  int NumWords=0;
  
  // split text
  for (p=pb=DictationText; *p != 0 && NumWords < MaxNumWords; p++) {
    if (*p == ' ') {
      // found separator
      Words[NumWords++] = pb;
      pb=p+1;
      *p=0;
    }
  }
  if (*p == 0 && pb != p) {
      Words[NumWords++] = pb;
  }
  
  // print split words
#if 0
  APP_LOG(APP_LOG_LEVEL_INFO, "Split words: %d", NumWords);
  for (int i=0; i < NumWords; i++) {
    APP_LOG(APP_LOG_LEVEL_INFO, "\tWord '%s'", Words[i]);
  }
#endif
  return NumWords;
}

int FindRoom(const char Word[], int StartIdx)
{
  for (int i=StartIdx; i < GetNumComs(); i++) {
    if (strcmp(Word, Coms_Map[i].Room) ==0)
      return i;
  }
  return -1;
}

// returns current word index in Words
// Example:
//   CmdIdx     = 0
//   Words      = Turn Kitchen Light on
//   CurWordIdx = 2 (Kitchen pos +1)
//   NumWords   = 4
int MatchRoomWords(int CmdIdx, char* Words[], int CurWordIdx, int NumWords)
{
  if (CurWordIdx >= NumWords)
    return NumWords;
  char* DescrWords[8];
  char Description[256];

  strncpy(Description, Coms_Map[CmdIdx].Description, 255);
  APP_LOG(APP_LOG_LEVEL_INFO, "\t\tDescription %s", Description);

  int NumDescrWords=ParseText(Description, DescrWords, 8);
  APP_LOG(APP_LOG_LEVEL_INFO, "\t\t NumWords %d", NumDescrWords);
  // DescrWords = Light on, NumDescrWords = 2
  
  int j = 0;
  for (int i=CurWordIdx; i < NumWords; i++){
    if (strcmp(Words[i], DescrWords[j]) == 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "\t\t[%d/%d]Match description word: %s at %d", j, NumDescrWords, DescrWords[j], i);
      j++;
      if (j == NumDescrWords) {
        // perfect match
        return i;
      }
    }
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "\t\tNo final match j=%d", j);
  return NumWords;
}


// returns found command index
int ExamineText(const char Text2Examine[])
{
  char Text[128];

  strncpy(Text, Text2Examine, 127);

  APP_LOG(APP_LOG_LEVEL_INFO, "Examine words in %s", Text);
  char* Words[64];
  int NumWords=ParseText(Text, Words, 64);

  APP_LOG(APP_LOG_LEVEL_INFO, "\t NumWords %d", NumWords);
  
  // example: "Im Flur Licht aus"
  for (int i=0; i < NumWords; i++) {
    for (int j=0; j < GetNumComs(); j++) {
      int CmdIdx = FindRoom(Words[i], j);
      if (CmdIdx >= 0) {
	// Flur: i = 1, CmdIdx = 2 (we have to find 4)
	APP_LOG(APP_LOG_LEVEL_INFO, "\t%s at %d", Words[i], CmdIdx);
	int CurWordIdx = MatchRoomWords(CmdIdx, Words, i+1, NumWords);
	if (CurWordIdx == NumWords) { // not found, continue	
	  continue;
	}
	APP_LOG(APP_LOG_LEVEL_INFO, "\tFound room idx %d. Last word: %s", CmdIdx, Words[CurWordIdx]);
	return CmdIdx;
      }
    }
  }
  return -1;
}


static int s_hit_count = 0;

#ifdef ENABLE_DICTATION
static DictationSession *s_dictation_session;
static char s_dictation_text[512];

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, 
                                       char *transcription, void *context)
{
  // Print the results of a transcription attempt                                     
  APP_LOG(APP_LOG_LEVEL_INFO, "Dictation status: %d", (int)status);
  
  SimpleMenuItem *menu_item = &s_favourites_menu_items[0];

  if(status == DictationSessionStatusSuccess) {
    // Display the dictated text
    snprintf(s_dictation_text, sizeof(s_dictation_text), "%s", transcription);
    menu_item->subtitle = s_dictation_text;
    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
    
    int CmdIdx = -1;
    if ((CmdIdx=ExamineText(s_dictation_text)) != -1) {
      if (SendCom(CmdIdx, false)) {
        set_menu_icon(MENU_FAVOURITES, 0, OK);
	      vibes_short_pulse(); // OK
      } else {
        set_menu_icon(MENU_FAVOURITES, 0, FAILED);
	      vibes_long_pulse();
      }
    } else {
      set_menu_icon(MENU_FAVOURITES, 0, FAILED);
      vibes_double_pulse();
    }

  } else {
    vibes_long_pulse();
    // Display the reason for any error
    static char s_failed_buff[128];
    snprintf(s_failed_buff, sizeof(s_failed_buff), "Transcription failed.\n\nReason:\n%d", 
           (int)status);
    menu_item->subtitle = s_failed_buff;
    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
    set_menu_icon(MENU_FAVOURITES, 0, FAILED);  
  }
}
#endif // ENABLE_DICTATION

static void special_select_callback(int index, void *ctx)
{
#ifdef ENABLE_DICTATION
  dictation_session_start(s_dictation_session);
#endif
  // layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void volume_select_callback(int index, void* ctx)
{
#if 0
  char* Words[64];
  int NumWords=ParseText("Hallo Michi. Wie gehts?", Words, 64);

    // print split words
  APP_LOG(APP_LOG_LEVEL_INFO, "Split words: %d", NumWords);
  for (int i=0; i < NumWords; i++) {
    APP_LOG(APP_LOG_LEVEL_INFO, "\tWord '%s'", Words[i]);
  }
#endif
#ifdef test
  //  ExamineText("Finde das Wort Flur in diesem Satz");
  ExamineText("In der Küche das Licht umschalten");
  ExamineText("Im Flur das Licht aus");
  ExamineText("Im Flur das Licht orange");
  ExamineText("Im Flur das Licht rot anschalten");
  ExamineText("Im Flur das Licht anschalten");
#endif
#if 0
  APP_LOG(APP_LOG_LEVEL_INFO, ".. finished. Now Description test");
  char* DescrWords[8];
  char  Description[256];
  strncpy(Description, Coms_Map[0].Description, 255);
  APP_LOG(APP_LOG_LEVEL_INFO, "\t\tDescription %s", Description);
  int NumDescrWords=ParseText(Description, DescrWords, 8);
  APP_LOG(APP_LOG_LEVEL_INFO, "\t\t NumWords %d", NumDescrWords);
#endif

  s_special_flag = !s_special_flag;

  SimpleMenuItem *menu_item = &s_favourites_menu_items[index];
  // menu_item->subtitle = "Not yet implemented";
  // layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
  if (s_special_flag) {
    menu_item->subtitle = "Receive Status Mode";
  } else {
    menu_item->subtitle = "Send Command Mode";
  }
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// Menu click callbacks    /////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void down_click_handler(ClickRecognizerRef recognizer, void *context)
{
  menu_layer_set_selected_next((MenuLayer *)s_simple_menu_layer, false, 
			       MenuRowAlignCenter, true);
}

void up_click_handler(ClickRecognizerRef recognizer, void *context)
{
  menu_layer_set_selected_next((MenuLayer *)s_simple_menu_layer, true, 
			       MenuRowAlignCenter, true);
}

#if 0
void select_click_handler(ClickRecognizerRef recognizer, void *context)
{
  int index=simple_menu_layer_get_selected_index(s_simple_menu_layer);
  
}
#endif

void select_long_click_handler(ClickRecognizerRef recognizer, void *context)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "long Select pressed!");
}

#if 0
static void click_config_provider(void *context) {
  // Register the ClickHandlers
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  // window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, NULL, select_long_click_handler);
}
#endif

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// main window load/unload /////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

static void main_window_load(Window *window)
{
  s_menu_icon_image_ok     = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON_OK);
  s_menu_icon_image_failed = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON_FAILED);
  s_menu_icon_image_send   = gbitmap_create_with_resource(RESOURCE_ID_MENU_ICON_SEND);
  s_menu_icon_image_off    = gbitmap_create_with_resource(RESOURCE_ID_STATUS_OFF);
  s_menu_icon_image_on     = gbitmap_create_with_resource(RESOURCE_ID_STATUS_ON);

  //window_set_background_color(window, GColorYellow);

  for (int i=0; i < GetNumComs(); i++) {
    s_first_menu_items[i] = (SimpleMenuItem) {
      .title    = Coms_Map[i].Room,
      .subtitle = Coms_Map[i].Description,
      .callback = menu_select_callback,
      // .icon     = PBL_IF_RECT_ELSE(s_menu_icon_image_ok, NULL),
    };
  }

  int itemCnt=0;
#ifdef ENABLE_DICTATION
  s_favourites_menu_items[itemCnt++] = (SimpleMenuItem) {
    .title = "Dictate",
    .callback = special_select_callback,
  };
#endif
  s_favourites_menu_items[itemCnt++] = (SimpleMenuItem) {
    .title = "Send/Receive Mode",
    .callback = volume_select_callback,
  };
  s_menu_sections[0] = (SimpleMenuSection) {
    .title = "Favourites",
    .num_items = itemCnt,
    .items = s_favourites_menu_items,
  };

  s_menu_sections[1] = (SimpleMenuSection) {
    .title = "Direct Commands",
    .num_items = GetNumComs(),
    .items = s_first_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
  
#ifdef ENABLE_DICTATION  
  // correct positon here?
  s_dictation_session = dictation_session_create(sizeof(s_dictation_text), 
						 dictation_session_callback, NULL);
  // Disable the confirmation screen
  dictation_session_enable_confirmation(s_dictation_session, false);
  // Disable error dialogs
  dictation_session_enable_error_dialogs(s_dictation_session, false);
#endif

#if 0
  // ClickConfigProvider menu_click_conf_prov=window_get_click_config_provider(window);
  //  window_set_click_config_provider(window, click_config_provider);

  menu_layer_set_callbacks((MenuLayer*)s_simple_menu_layer, NULL,
			   (MenuLayerCallbacks) {
			     .select_long_click = select_long_click_cb,
			    });
  
  menu_layer_set_click_config_onto_window((MenuLayer*)s_simple_menu_layer, window);
#endif
}
////////////////////////////////////////////////////////////////////


void main_window_unload(Window *window)
{
  simple_menu_layer_destroy(s_simple_menu_layer);
  gbitmap_destroy(s_menu_icon_image_on);
  gbitmap_destroy(s_menu_icon_image_off);
  gbitmap_destroy(s_menu_icon_image_send);
  gbitmap_destroy(s_menu_icon_image_failed);
  gbitmap_destroy(s_menu_icon_image_ok);
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

  /*  app_message_open(app_message_inbox_size_maximum(),
      app_message_outbox_size_maximum()); */
  app_message_open(128, 128);

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
