#include <pebble.h>


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// FHEM definitions ////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

const char FHEM_URL[] = "http://mypi:8083/fhem";


static struct _Feature_Ctrl_Map
{
  const char* Room;
  const char* Description;
  const char* URL; // if used
  const char* Device;
  const char* Command;
} Feature_Ctrl_Map[] = {
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
    "Alles",
    "aus", NULL,
    "dummy_all", "off"},
};

int GetNumComs()
{
  return sizeof(Feature_Ctrl_Map) / sizeof(struct _Feature_Ctrl_Map);
}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// AppMessage //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
static const uint32_t FHEM_URL_KEY     = 0;
static const uint32_t FHEM_COM_ID_KEY  = 1;
static const uint32_t FHEM_RESP_KEY    = 2;

static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{  
  Tuple *data;

  int index = -1;
  if ((data=dict_find(iterator, FHEM_COM_ID_KEY)) != NULL) {
    index = (int)data->value->int32;
  }

  if ((data = dict_find(iterator, FHEM_RESP_KEY)) != NULL) {
    APP_LOG(APP_LOG_LEVEL_INFO, "FHEM_RESP_KEY received: %s of index %d", (char*)data->value->cstring, index);
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
  if (Feature_Ctrl_Map[index].URL)
    snprintf(URL, size, "%s?%s", FHEM_URL, Feature_Ctrl_Map[index].URL);
  else {
    snprintf(URL, size, "%s?cmd=set%%20%s%%20%s", FHEM_URL, 
             Feature_Ctrl_Map[index].Device, Feature_Ctrl_Map[index].Command);
  }
}


void SendCom(int index)
{
  char URL[1024];
  BuildFhemURL(index, URL, 1024);
  
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

#if 0
  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error creating outbound message!");
    // Error creating outbound message
    return;
  }
#endif

  DictionaryResult Res;
  if ((Res=dict_write_cstring(iter, FHEM_URL_KEY, URL)) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Dict write url error!");
    return;
  }
  
  if ((Res=dict_write_int(iter, FHEM_COM_ID_KEY, &index, sizeof(int), true)) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Dict write com id error!");
    return;
  }
  
  app_message_outbox_send();
}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// MENU ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

#define NUM_MENU_SECTIONS 2
#define NUM_MENU_ITEMS_FAVOURITES 2
#define NUM_COM 32

static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_COM];
static SimpleMenuItem s_second_menu_items[NUM_MENU_ITEMS_FAVOURITES];
// static GBitmap *s_menu_icon_image;



static void menu_select_callback(int index, void *ctx)
{
  // s_first_menu_items[index].subtitle = "You've hit select here!";

  SendCom(index);
  
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// TODO: dictate API       /////////////////////////////////////////
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
    if (strcmp(Word, Feature_Ctrl_Map[i].Room) ==0)
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

  strncpy(Description, Feature_Ctrl_Map[CmdIdx].Description, 255);
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


static bool s_special_flag = false;
static int s_hit_count = 0;

static DictationSession *s_dictation_session;
static char s_dictation_text[512];

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, 
                                       char *transcription, void *context)
{
  // Print the results of a transcription attempt                                     
  APP_LOG(APP_LOG_LEVEL_INFO, "Dictation status: %d", (int)status);
  
  SimpleMenuItem *menu_item = &s_second_menu_items[0];

  if(status == DictationSessionStatusSuccess) {
    // Display the dictated text
    snprintf(s_dictation_text, sizeof(s_dictation_text), "%s", transcription);
    menu_item->subtitle = s_dictation_text;
    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
    
    int CmdIdx = -1;
    if ((CmdIdx=ExamineText(s_dictation_text)) != -1)
      SendCom(CmdIdx);

  } else {
    // Display the reason for any error
    static char s_failed_buff[128];
    snprintf(s_failed_buff, sizeof(s_failed_buff), "Transcription failed.\n\nReason:\n%d", 
           (int)status);
    menu_item->subtitle = s_failed_buff;
    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
  }
}


static void special_select_callback(int index, void *ctx)
{
  dictation_session_start(s_dictation_session);
  
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

#if 0
  char* Words[64];
  int NumWords=ParseText("Hallo Michi. Wie gehts?", Words, 64);

    // print split words
  APP_LOG(APP_LOG_LEVEL_INFO, "Split words: %d", NumWords);
  for (int i=0; i < NumWords; i++) {
    APP_LOG(APP_LOG_LEVEL_INFO, "\tWord '%s'", Words[i]);
  }
#endif
  //  ExamineText("Finde das Wort Flur in diesem Satz");
  ExamineText("In der Küche das Licht umschalten");
  ExamineText("Im Flur das Licht aus");
  ExamineText("Im Flur das Licht orange");
  ExamineText("Im Flur das Licht rot anschalten");
  ExamineText("Im Flur das Licht anschalten");
#if 0
  APP_LOG(APP_LOG_LEVEL_INFO, ".. finished. Now Description test");
  char* DescrWords[8];
  char  Description[256];
  strncpy(Description, Feature_Ctrl_Map[0].Description, 255);
  APP_LOG(APP_LOG_LEVEL_INFO, "\t\tDescription %s", Description);
  int NumDescrWords=ParseText(Description, DescrWords, 8);
  APP_LOG(APP_LOG_LEVEL_INFO, "\t\t NumWords %d", NumDescrWords);
#endif
}
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// main window load/unload /////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

static void main_window_load(Window *window)
{
  //window_set_background_color(window, GColorYellow);

  for (int i=0; i < GetNumComs(); i++) {
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
    .num_items = GetNumComs(),
    .items = s_first_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);

  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
  
  // correct positon here?
  s_dictation_session = dictation_session_create(sizeof(s_dictation_text), 
						 dictation_session_callback, NULL);
  // Disable the confirmation screen
  dictation_session_enable_confirmation(s_dictation_session, false);
  // Disable error dialogs
  dictation_session_enable_error_dialogs(s_dictation_session, false);
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
