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

// use this to specify in which menu the command shall appear.
typedef enum MenuIdx_ {
  MenuOmit  = -1,
  MenuDef   = -2,
  MenuFav   = -3,
  MenuState = -4,
} MenuIdx_t;

// the menu indices are overwritten during creation of menu if not MenuOmit
typedef struct Coms_Map_
{
  const char* Room;
  char* Description; // workaround for state placeholder
  const char* URL; // if used
  const char* Device;
  const char* Command;
  MenuIdx_t   MenuDefIdx;    // default: either MenuDef or MenuOmit
  MenuIdx_t   MenuFavIdx;    // default: either MenuDef or MenuFav
  MenuIdx_t   MenuStateIdx;  // default: either MenuDef or MenuState
} Coms_Map_t;


static Coms_Map_t Coms_Map[] = {
  {
    "Balkon",
    "--------------------", NULL,
    "Thermo1", "",
    MenuOmit, MenuOmit, MenuState,
  },
  {
    "Wohnzimmer",
    "--------------------", NULL,
    "Thermo2", "",
    MenuOmit, MenuOmit, MenuState,
  },
  {
    "Schlafzimmer",
    "--------------------", NULL,
    "Thermo3", "",
    MenuOmit, MenuOmit, MenuState,
  },
  {
    "Küche",
    "Licht umschalten", NULL,
    "FS20_fr_bel", "toggle",
    MenuDef, MenuFav, MenuOmit,
  },
  {
    "Wohnzimmer",
    "Licht an/aus", NULL,
    "FS20_fz_bel", "toggle",
    MenuOmit, MenuFav, MenuOmit,
  },
  {
    "Flur",
    "rot", NULL,
    /* "http://mypi:8083/fhem?cmd=set%20HueFlur1%20rgb%20FF0000&XHR=1" },*/
    "HueFlur1", "rgb%20FF0000",
    MenuDef, MenuFav, MenuOmit,
  },
  {
    "Flur",
    "orange",
    "cmd=set%20HueFlur1%20rgb%20FF830A&XHR=1",
    NULL, NULL,
    MenuDef, MenuOmit, MenuOmit,
  },
  {
    "Flur",
    "aus", NULL,
    "HueFlur1", "off",
    MenuDef, MenuFav, MenuState,
  },
  {
    "Schlafzimmer",
    "rot", NULL,
    "HueSchlafzimmer1", "rgb%20FF0000",
    MenuDef, MenuFav, MenuOmit,
  },
  {
    "Schlafzimmer",
    "aus", NULL,
    "HueSchlafzimmer1", "off",
    MenuDef, MenuFav, MenuState,
  },
  {
    "Lautstärke",
    "lauter", NULL,
    "FS20_0000_VolUp", "on",
    MenuDef, MenuOmit, MenuOmit,
  },
  {
    "Lautstärke",
    "leiser", NULL,
    "FS20_0001_VolDown", "on",
    MenuDef, MenuOmit, MenuOmit,
  },
  {
    "Radio",
    "leiser",
    "cmd=%22mpc.sh+-q+-h+lora+volume+-10%22",
    NULL, NULL,
    MenuDef, MenuOmit, MenuOmit,
  },
  {
    "Radio",
    "lauter",
    "cmd=%22mpc.sh+-q+-h+lora+volume+%2B10%22",
    NULL, NULL,
    MenuDef, MenuOmit, MenuOmit,
  },
  {
    "Radio",
    "an", NULL,
    "FS20_Remote_Radio", "on",
    MenuDef, MenuFav, MenuState,
  },
  {
    "Küche",
    "Licht an", NULL,
    "FS20_fr_bel", "on",
    MenuOmit, MenuOmit, MenuState,
  },
  {
    "Küche",
    "Licht aus", NULL,
    "FS20_fr_bel", "off",
    MenuOmit, MenuOmit, MenuOmit, // used for dictate but does not appear in menu
  },
  {
    "Alles",
    "aus", NULL,
    "dummy_all", "off",
    MenuDef, MenuFav, MenuOmit,
  },
};

int GetNumComs()
{
  return sizeof(Coms_Map) / sizeof(Coms_Map_t);
}

typedef int MapIdx_t;

Coms_Map_t* GetCom(MapIdx_t Idx)
{ 
  if (Idx >= 0 && Idx < GetNumComs())
    return &Coms_Map[Idx];
  else
    return NULL;
}


//////////////////////////////////////////////////////
// Menu mapping functions Menu index <-> command index
MenuIdx_t GetDefMenuIdx(MapIdx_t MapIdx)
{
  Coms_Map_t* PCom=GetCom(MapIdx);
  if (!PCom) return MenuOmit;
  return PCom->MenuDefIdx;
}

MenuIdx_t GetFavMenuIdx(MapIdx_t MapIdx)
{
  Coms_Map_t* PCom=GetCom(MapIdx);
  if (!PCom) return MenuOmit;
  return PCom->MenuFavIdx;
}

MenuIdx_t GetStateMenuIdx(MapIdx_t MapIdx)
{
  Coms_Map_t* PCom=GetCom(MapIdx);
  if (!PCom) return MenuOmit;
  return PCom->MenuStateIdx;
}


MapIdx_t GetCmdIdxFromDefMenu(MenuIdx_t MenuIdx)
{
  for (int i=0; i < GetNumComs(); i++) {
    Coms_Map_t* PCom=GetCom(i);
    if (!PCom) continue;
    if (MenuIdx == PCom->MenuDefIdx)
      return i;
  }
  return -1;
}
MapIdx_t GetCmdIdxFromFavMenu(MenuIdx_t MenuIdx)
{
  for (int i=0; i < GetNumComs(); i++) {
    Coms_Map_t* PCom=GetCom(i);
    if (!PCom) continue;
    if (MenuIdx == PCom->MenuFavIdx)
      return i;
  }
  return -1;
}
MapIdx_t GetCmdIdxFromStateMenu(MenuIdx_t MenuIdx)
{
  for (int i=0; i < GetNumComs(); i++) {
    Coms_Map_t* PCom=GetCom(i);
    if (!PCom) continue;
    if (MenuIdx == PCom->MenuStateIdx)
      return i;
  }
  return -1;
}



typedef enum _StatusIconType {
  UNKNOWN = -1,
  FAILED  = 0,
  OK      = 1,
  SEND    = 2,
  OFF     = 3,
  ON      = 4,  
} StatusIcon_t;

typedef enum _MenuType {
  MENU_UNKNOWN    = -1,
  MENU_SPECIAL    = 0,
  MENU_DEF_COMS   = 1,
  MENU_FAV_COMS   = 2,
  MENU_STATE_COMS = 3,
} Menu_t;

typedef enum _MsgID {
  MSG_ID_UNKNOWN = -1,
  MSG_ID_DEFAULT =  0,
  MSG_ID_SEND_COM_REQ_STATE_NEXT = 1,
} MsgID_t;


///////////////////////
// Forward declarations
bool set_menu_icon(Menu_t Menu, int index, StatusIcon_t Status);
// todo:
bool set_menu_text(Menu_t Menu, MapIdx_t CmdIdx, int index, const char text[]);
  
bool SendCommand(MapIdx_t CmdIdx, bool requestStatus, MsgID_t ID);

bool SendCom(MapIdx_t CmdIdx)  { return SendCommand(CmdIdx, false, MSG_ID_DEFAULT); }
bool SendComR(MapIdx_t CmdIdx) { return SendCommand(CmdIdx, true,  MSG_ID_DEFAULT); }

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// AppMessage //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
static const uint32_t FHEM_URL_KEY       = 0;
static const uint32_t FHEM_COM_ID_KEY    = 1;
static const uint32_t FHEM_RESP_KEY      = 2;
static const uint32_t FHEM_URL_GET_STATE = 3;
static const uint32_t FHEM_MSG_ID        = 4;

static void inbox_received_callback(DictionaryIterator *iterator, void *context)
{  
  Tuple *data;

  MapIdx_t index = -1;
  if ((data=dict_find(iterator, FHEM_COM_ID_KEY)) != NULL) {
    index = (MapIdx_t)data->value->int32;
  }

  MenuIdx_t MenuDefIdx   = GetDefMenuIdx(index);
  MenuIdx_t MenuFavIdx   = GetFavMenuIdx(index);
  MenuIdx_t MenuStateIdx = GetStateMenuIdx(index);


  MsgID_t MsgID = MSG_ID_UNKNOWN;
  if ((data=dict_find(iterator, FHEM_MSG_ID)) != NULL) {
    MsgID = (MsgID_t)data->value->int32;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "MsgID: %d of CmdIdx %d", MsgID, (int)index);
  
  bool successfull = true;

  if ((data = dict_find(iterator, FHEM_RESP_KEY)) != NULL) {
    char* result = (char*)data->value->cstring;
    APP_LOG(APP_LOG_LEVEL_INFO, "FHEM_RESP_KEY received: %s of index %d", result, (int)index);
    if (!strcmp("success", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx, OK);
      // now fetch state:
      SendComR(index);

    } else if (!strcmp("off", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx,   OFF);
      set_menu_icon(MENU_FAV_COMS, MenuFavIdx,   OFF);
      set_menu_icon(MENU_STATE_COMS, MenuStateIdx, OFF);
    } else if (!strcmp("on", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx,   ON);
      set_menu_icon(MENU_FAV_COMS, MenuFavIdx,   ON);
      set_menu_icon(MENU_STATE_COMS, MenuStateIdx, ON);
    } else if (!strcmp("toggle", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx,   OK);
      set_menu_icon(MENU_FAV_COMS, MenuFavIdx,   OK);
      set_menu_icon(MENU_STATE_COMS, MenuStateIdx, OK);
    } else if (!strcmp("not connected", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx, FAILED);
      if (MsgID != MSG_ID_SEND_COM_REQ_STATE_NEXT)
	vibes_long_pulse();
    } else {
      set_menu_text(MENU_STATE_COMS, index, MenuStateIdx,   result); // todo
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx,   OK);
      set_menu_icon(MENU_FAV_COMS, MenuFavIdx,   OK);
      set_menu_icon(MENU_STATE_COMS, MenuStateIdx, OK);
    } 
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "FHEM_RESP_KEY not received.");
    successfull = false;
  }

  if (successfull) {
    if (MsgID == MSG_ID_SEND_COM_REQ_STATE_NEXT) {
      bool fetch_next = true;
      while (fetch_next) {
	index++;
	Coms_Map_t* PCom = GetCom(index);
	if (!PCom)
	  fetch_next = false; // index not valid anymore
	else {
	  if (PCom->Device) {
	    APP_LOG(APP_LOG_LEVEL_DEBUG, "Request next state from %s",
		    GetCom(index)->Device);
	    if (SendCommand(index, true, MSG_ID_SEND_COM_REQ_STATE_NEXT)) {
	      fetch_next = false; // command fired successfully, waiting for next msg
	    }
	  }
	}
      }
    }
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
void BuildFhemURL(const MapIdx_t index, char URL[], int size)
{
  Coms_Map_t* PCom = GetCom(index);
  if (!PCom) return;

  if (GetCom(index)->URL)
    snprintf(URL, size, "%s?%s", FHEM_URL, PCom->URL);
  else {
    snprintf(URL, size, "%s?cmd=set%%20%s%%20%s&XHR=1", FHEM_URL, 
             PCom->Device, PCom->Command);
  }
}

bool BuildFhemStatusURL(const MapIdx_t index, char URL[], int size)
{
  Coms_Map_t* PCom = GetCom(index);
  if (!PCom) {
    *URL = 0;
    return false;
  }

  if (!PCom->Device) {
    *URL = 0;
    return false;
  }
  snprintf(URL, size, "%s?cmd=jsonlist%%20%s&XHR=1", FHEM_URL, 
	   PCom->Device);
  return true;
}


bool SendCommand(MapIdx_t index, bool requestStatus, MsgID_t MsgID)
{
  char URL[1024];

  if (!requestStatus) {
    BuildFhemURL(index, URL, 1024);
  } else {
    if (!BuildFhemStatusURL(index, URL, 1024))
      return false; // device is not specified (just URL)
  }
  if (strlen(URL) == 0)
    return false;

  AppMessageResult Res = APP_MSG_OK;
    
  DictionaryIterator *iter;
  if ((Res=app_message_outbox_begin(&iter)) != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "ERROR: app_message_outbox_begin: %d", Res);   
    return false;
  }

  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error creating outbound message!");
    return false;
  }
  
  DictionaryResult ResDict;
  if (!requestStatus) {
    if ((ResDict=dict_write_cstring(iter, FHEM_URL_KEY, URL)) != DICT_OK) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Dict write url error: %d!", ResDict);
      return false;
    }
  } else {
    if ((ResDict=dict_write_cstring(iter, FHEM_URL_GET_STATE, URL)) != DICT_OK) {
      APP_LOG(APP_LOG_LEVEL_ERROR, "Dict write status url error: %d!", ResDict);
      return false;
    }
  }
  
  
  if ((ResDict=dict_write_int(iter, FHEM_COM_ID_KEY, (int*)&index, sizeof(int), true)) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dict write com id error: %d", ResDict);
    return false;
  }

  if ((ResDict=dict_write_int(iter, FHEM_MSG_ID, &MsgID, sizeof(int), true)) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dict write MsgID error: %d", ResDict);
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
#define NUM_MENU_SECTIONS 5
#define MAX_NUM_MENU_ITEMS_FAVOURITES 10
#define NUM_COM 32

static GBitmap *s_menu_icon_image_ok;
static GBitmap *s_menu_icon_image_failed;
static GBitmap *s_menu_icon_image_send;
static GBitmap *s_menu_icon_image_off;
static GBitmap *s_menu_icon_image_on;
static SimpleMenuItem s_def_menu_items[NUM_COM];
static SimpleMenuItem s_fav_menu_items[NUM_COM];
static SimpleMenuItem s_states_menu_items[NUM_COM];
static SimpleMenuItem s_special_menu_items[MAX_NUM_MENU_ITEMS_FAVOURITES];
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuLayer *s_simple_menu_layer;


// string memory management
static int   NumStringsAlloc=0;
static char* StringArr[128];

char* AllocStr(int size)
{
  char* RetStr = NULL;
  if (NumStringsAlloc >= 128) return NULL;
  RetStr = calloc(size, sizeof(char));
  
  if (RetStr)
    StringArr[NumStringsAlloc++] = RetStr;

  return RetStr;
}

void FreeStr()
{
  for (int i=0; i < NumStringsAlloc; i++)
    free(StringArr[i]);
  NumStringsAlloc = 0;
}



bool set_menu_icon(Menu_t Menu, int index, StatusIcon_t Status)
{
  SimpleMenuItem* pMenu = NULL;

  if (index < 0) return false; // either omit or not set yet

  switch(Menu) {
    default:
    case MENU_UNKNOWN:
      return false;
    case MENU_SPECIAL:
      pMenu = &s_special_menu_items[index];
      break;
    case MENU_DEF_COMS:
      pMenu = &s_def_menu_items[index];
      break;   
    case MENU_FAV_COMS:
      pMenu = &s_fav_menu_items[index];
      break;   
    case MENU_STATE_COMS:
      pMenu = &s_states_menu_items[index];
      break;   
  };

  if (!pMenu)
    return false;
  
  switch(Status) {
    default:
    case UNKNOWN:
      return false;
    case FAILED:
      pMenu->icon = s_menu_icon_image_failed;
      break;
    case OK:
      pMenu->icon = s_menu_icon_image_ok;
      break;
    case SEND:
      pMenu->icon = s_menu_icon_image_send;
      break;
    case OFF:
      pMenu->icon = s_menu_icon_image_off;
      break;
    case ON:
      pMenu->icon = s_menu_icon_image_on;
      break;
  }
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
 
  return true;
}


bool set_menu_text(Menu_t Menu, MapIdx_t CmdIdx, int index, const char text[])
{
  SimpleMenuItem* pMenu = NULL;
  
  if (index < 0) return false; // either omit or not set yet

  //  Coms_Map_t* PCom=GetCom(CmdIdx);
  //  if (!PCom) return false;
  
  switch(Menu) {
    default:
    case MENU_UNKNOWN:
      return false;
    case MENU_SPECIAL:
      pMenu = &s_special_menu_items[index];
      break;
    case MENU_DEF_COMS:
      pMenu = &s_def_menu_items[index];
      break;   
    case MENU_FAV_COMS:
      pMenu = &s_fav_menu_items[index];
      break;   
    case MENU_STATE_COMS:
      pMenu = &s_states_menu_items[index];
      break;   
  };

  if (!pMenu)
    return false;

  int len = strlen(text);
  char* SubTitle = AllocStr(len);

  if (!SubTitle) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Cannot request string memory of length %d", len);
    return false;
  }

  strncpy(SubTitle, text, len);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Changing menu text of command %d from %s to %s", 
	  CmdIdx, pMenu->subtitle, SubTitle);
  
  pMenu->subtitle = SubTitle;
  
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));

  return true;
}


// todo: for tests
static bool s_special_flag = false;

static void def_menu_select_callback(int index, void *ctx)
{
  MapIdx_t CmdIdx = GetCmdIdxFromDefMenu(index);
  if (CmdIdx < 0) return;  

  if (SendCommand(CmdIdx, s_special_flag, MSG_ID_DEFAULT))
    set_menu_icon(MENU_DEF_COMS, index, SEND);
  else
    set_menu_icon(MENU_DEF_COMS, index, FAILED);
}


static void fav_menu_select_callback(int index, void *ctx)
{
  MapIdx_t CmdIdx = GetCmdIdxFromFavMenu(index);
  if (CmdIdx < 0) return;  

  if (SendCommand(CmdIdx, s_special_flag, MSG_ID_DEFAULT))
    set_menu_icon(MENU_FAV_COMS, index, SEND);
  else
    set_menu_icon(MENU_FAV_COMS, index, FAILED);
}


static void states_menu_select_callback(int index, void *ctx)
{
  MapIdx_t CmdIdx = GetCmdIdxFromStateMenu(index);
  if (CmdIdx < 0) return;  

  if (SendCommand(CmdIdx, true, MSG_ID_DEFAULT))
    set_menu_icon(MENU_STATE_COMS, index, SEND);
  else
    set_menu_icon(MENU_STATE_COMS, index, FAILED);
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
    Coms_Map_t* PCom = GetCom(i);
    if (!PCom) return -1;
    if (strcmp(Word, PCom->Room) == 0)
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

  Coms_Map_t* PCom = GetCom(CmdIdx);
  if (!PCom) return NumWords;

  strncpy(Description, PCom->Description, 255);
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
MapIdx_t ExamineText(const char Text2Examine[])
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
      MapIdx_t CmdIdx = FindRoom(Words[i], j);
      if (CmdIdx >= 0) {
	// Flur: i = 1, CmdIdx = 2 (we have to find 4)
	APP_LOG(APP_LOG_LEVEL_INFO, "\t%s at %d", Words[i], (int)CmdIdx);
	int CurWordIdx = MatchRoomWords(CmdIdx, Words, i+1, NumWords);
	if (CurWordIdx == NumWords) { // not found, continue	
	  continue;
	}
	APP_LOG(APP_LOG_LEVEL_INFO, "\tFound room idx %d. Last word: %s", (int)CmdIdx, Words[CurWordIdx]);
	return CmdIdx;
      }
    }
  }
  return -1;
}


#ifdef ENABLE_DICTATION
static DictationSession *s_dictation_session;
static char s_dictation_text[512];

static void dictation_session_callback(DictationSession *session, DictationSessionStatus status, 
                                       char *transcription, void *context)
{
  // Print the results of a transcription attempt                                     
  APP_LOG(APP_LOG_LEVEL_INFO, "Dictation status: %d", (int)status);
  
  SimpleMenuItem *menu_item = &s_special_menu_items[0];

  if(status == DictationSessionStatusSuccess) {
    // Display the dictated text
    snprintf(s_dictation_text, sizeof(s_dictation_text), "%s", transcription);
    menu_item->subtitle = s_dictation_text;
    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
    
    MapIdx_t CmdIdx = -1;
    if ((CmdIdx=ExamineText(s_dictation_text)) != -1) {
      if (SendCom(CmdIdx)) {
        set_menu_icon(MENU_SPECIAL, 0, OK);
	      vibes_short_pulse(); // OK
      } else {
        set_menu_icon(MENU_SPECIAL, 0, FAILED);
	      vibes_long_pulse();
      }
    } else {
      set_menu_icon(MENU_SPECIAL, 0, FAILED);
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
    set_menu_icon(MENU_SPECIAL, 0, FAILED);  
  }
}
#endif // ENABLE_DICTATION

#ifdef ENABLE_DICTATION
static void special_select_callback(int index, void *ctx)
{
  dictation_session_start(s_dictation_session);
  // layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}
#endif


static void request_states_select_callback(int index, void* ctx)
{
  SendCommand(0, true, MSG_ID_SEND_COM_REQ_STATE_NEXT);

/*
    set_menu_icon(MENU_DEF_COMS, index, SEND);
  else
    set_menu_icon(MENU_DEF_COMS, index, FAILED);
*/
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
  strncpy(Description, GetCom(0)->Description, 255);
  APP_LOG(APP_LOG_LEVEL_INFO, "\t\tDescription %s", Description);
  int NumDescrWords=ParseText(Description, DescrWords, 8);
  APP_LOG(APP_LOG_LEVEL_INFO, "\t\t NumWords %d", NumDescrWords);
#endif

  s_special_flag = !s_special_flag;

  SimpleMenuItem *menu_item = &s_def_menu_items[index];
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
// Menus                   /////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
int create_special_menu()
{
  int MenuCnt=0;
#ifdef ENABLE_DICTATION
  s_special_menu_items[MenuCnt++] = (SimpleMenuItem) {
    .title = "Dictate",
    .callback = special_select_callback,
  };
#endif
  /*
  s_special_menu_items[MenuCnt++] = (SimpleMenuItem) {
    .title = "Send/Receive Mode",
    .callback = volume_select_callback,
  };
  */
  /*  s_special_menu_items[MenuCnt++] = (SimpleMenuItem) {
    .title = "Request States",
    .callback = request_states_select_callback,
  };
  */
  return MenuCnt;
}


int create_default_menu()
{
  int MenuCnt=0;

  s_def_menu_items[MenuCnt++] = (SimpleMenuItem) {
    .title    = "Send/Receive Mode",
    .callback = volume_select_callback,
    .icon     = s_menu_icon_image_send,
  };

  for (int i=0; i < GetNumComs(); i++) {
    Coms_Map_t* PCom = GetCom(i);
    if (!PCom) continue;
    
    if (PCom->MenuDefIdx != MenuOmit) {
      s_def_menu_items[MenuCnt] = (SimpleMenuItem) {
	.title    = PCom->Room,
	.subtitle = PCom->Description,
	.callback = def_menu_select_callback,
	// .icon     = PBL_IF_RECT_ELSE(s_menu_icon_image_ok, NULL),
      };
      PCom->MenuDefIdx = MenuCnt;
      MenuCnt++;
    }
  }
  return MenuCnt;
}


int create_favourites_menu()
{
  int MenuCnt=0;
  for (int i=0; i < GetNumComs(); i++) {
    Coms_Map_t* PCom = GetCom(i);
    if (!PCom) continue;
    
    if (PCom->MenuFavIdx != MenuOmit) {
      s_fav_menu_items[MenuCnt] = (SimpleMenuItem) {
	.title    = PCom->Room,
	.subtitle = PCom->Description,
	.callback = fav_menu_select_callback,
	// .icon     = PBL_IF_RECT_ELSE(s_menu_icon_image_ok, NULL),
      };
      PCom->MenuFavIdx = MenuCnt;
      MenuCnt++;
    }
  }
  return MenuCnt;
}


int create_states_menu()
{
  int MenuCnt=0;

  s_states_menu_items[MenuCnt++] = (SimpleMenuItem) {
    .title = "Request",
    .subtitle = "States",
    .callback = request_states_select_callback,
    .icon = s_menu_icon_image_send,
  };

  for (int i=0; i < GetNumComs(); i++) {
    Coms_Map_t* PCom = GetCom(i);
    if (!PCom) continue;
    
    if (PCom->MenuStateIdx != MenuOmit) {
      s_states_menu_items[MenuCnt] = (SimpleMenuItem) {
	.title    = PCom->Room,
	.subtitle = PCom->Description,
	.callback = states_menu_select_callback,
	// .icon     = PBL_IF_RECT_ELSE(s_menu_icon_image_ok, NULL),
      };
      PCom->MenuStateIdx = MenuCnt;
      MenuCnt++;
    }
  }
  return MenuCnt;
}


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

  ///////////////////////
  // create menu sections

  int NumItems=0;
  int NumSections=0;

  // special menu
  NumItems=create_special_menu();
  if (NumItems > 0) {
    s_menu_sections[NumSections++] = (SimpleMenuSection) {
      .title = "Special",
      .num_items = NumItems,
      .items = s_special_menu_items,
    };
  }

  // favourite menu
  NumItems=create_favourites_menu();
  s_menu_sections[NumSections++] = (SimpleMenuSection) {
    .title = "Favourites",
    .num_items = NumItems,
    .items = s_fav_menu_items,
  };

  // states menu
  NumItems=create_states_menu();
  s_menu_sections[NumSections++] = (SimpleMenuSection) {
    .title = "States",
    .num_items = NumItems,
    .items = s_states_menu_items,
  };

  // default menu
  NumItems=create_default_menu();
  s_menu_sections[NumSections++] = (SimpleMenuSection) {
    .title = "Default Commands",
    .num_items = NumItems,
    .items = s_def_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_simple_menu_layer = simple_menu_layer_create(bounds, window, 
						 s_menu_sections, NumSections,
						 NULL);

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

  // tests:
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
  app_message_open(256, 256);

}

static void deinit(void) {
  FreeStr();
  window_destroy(s_window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  deinit();
}
