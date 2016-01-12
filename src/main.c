#include <pebble.h>

#include "ComsStaticDefs.h"

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

#undef DEBUG_ADD_COM
#undef DICTATE_COMS_IN_MENU
#undef REQUEST_STATES_IN_MAIN_MENU


// string memory management
static int   NumStringsAlloc=0;
static char* StringArr[256];

char* AllocStr(int size)
{
  char* RetStr = NULL;
  if (NumStringsAlloc >= 256) return NULL;
  RetStr = calloc(size, sizeof(char));
  
  if (RetStr)
    StringArr[NumStringsAlloc++] = RetStr;

  return RetStr;
}

char* AddNewStr(const char* text)
{
  // APP_LOG(APP_LOG_LEVEL_DEBUG, "Try to add new string %s", text);

  int len = strlen(text)+1;
  char* Str = AllocStr(len);
  
  if (!Str) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Cannot request str memory for %s (%d)", text, len);
    return NULL;
  }
  strncpy(Str, text, len-1);
  Str[len-1] = 0;

  return Str;
}

void FreeStr()
{
  for (int i=0; i < NumStringsAlloc; i++)
    free(StringArr[i]);
  NumStringsAlloc = 0;
}



// const char FHEM_URL[] = "http://mypi:8083/fhem";
const char FHEM_URL[] = ""; // now in javascript


void PrintCom(Coms_Map_t* PCom)
{
  if (!PCom) return;
  if (!PCom->Device) return;

  APP_LOG(APP_LOG_LEVEL_DEBUG, "\tCom: %s", PCom->Device);

  if (PCom->Room)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "\t     %s", PCom->Room);

  if (PCom->Description)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "\t     %s", PCom->Description);

  if (PCom->Command)
    APP_LOG(APP_LOG_LEVEL_DEBUG, "\t     %s", PCom->Command);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "\t     Def:%d Fav:%d State:%d", 
	  PCom->MenuDefIdx, PCom->MenuFavIdx, PCom->MenuStateIdx);
}


////////////////////////////////////////////////////////////
// Dynamic coms map
////////////////////////////////////////////////////////////
#define MaxNumCom 64
static Coms_Map_t Coms_Map_Dyn[MaxNumCom];
static int Coms_Cnt = 0;
static bool Coms_UseDyn = false;


bool AddCom(Coms_Map_t* PCom)
{
  if (Coms_Cnt >= MaxNumCom)
    return false;

  // simple copy
  memcpy((void*)&Coms_Map_Dyn[Coms_Cnt], (void*)PCom, sizeof(Coms_Map_t));
  
#ifdef DEBUG_ADD_COM
  APP_LOG(APP_LOG_LEVEL_DEBUG, "ComAdd[%d]", Coms_Cnt);
  PrintCom(&Coms_Map_Dyn[Coms_Cnt]);
#endif

  Coms_Cnt++;
  return true;
}


Coms_Map_t InitCom()
{
  Coms_Map_t NewCom = {
    .Room         = NULL,
    .Description  = NULL,
    .URL          = NULL,
    .Device       = NULL,
    .Command      = NULL,
    .MenuDefIdx   = MenuOmit,
    .MenuFavIdx   = MenuOmit,
    .MenuStateIdx = MenuOmit,
  };
  return NewCom;
}

// test function
void TestAddCom()
{
  Coms_Map_t NewCom = {
    .Room         = "Küche",
    .Description  = "Licht umschalten",
    .URL          = NULL,
    .Device       = "FS20_fr_bel",
    .Command      = "toggle",
    .MenuDefIdx   = MenuDef,
    .MenuFavIdx   = MenuFav,
    .MenuStateIdx = MenuOmit,
  };
  
  AddCom(&NewCom);
}

int GetNumComs()
{
  if (Coms_Cnt == 0 || !Coms_UseDyn)
    return sizeof(Coms_Map) / sizeof(Coms_Map_t);
  else
    return Coms_Cnt;
}

Coms_Map_t* GetCom(MapIdx_t Idx)
{ 
  if (Idx >= 0 && Idx < GetNumComs()) {
    if (Coms_Cnt == 0 || !Coms_UseDyn)
      return &Coms_Map[Idx];
    else
      return &Coms_Map_Dyn[Idx];
  } else
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
  TOGGLE  = 5,
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
  MSG_ID_REQ_TYPE = 2
} MsgID_t;


///////////////////////
// Forward declarations

// index IDs for set_menu_icon/text
static int s_special_menu_id_status;
static int s_special_menu_id_ReqFHEM;

bool set_menu_icon(Menu_t Menu, int index, StatusIcon_t Status);
// todo:
bool set_menu_text(Menu_t Menu, MapIdx_t CmdIdx, int index, const char text[]);

void set_status(const char text[]);

bool SendCommand(MapIdx_t CmdIdx, bool requestStatus, MsgID_t ID);

bool SendCom(MapIdx_t CmdIdx)  { return SendCommand(CmdIdx, false, MSG_ID_DEFAULT); }
bool SendComR(MapIdx_t CmdIdx) { return SendCommand(CmdIdx, true,  MSG_ID_DEFAULT); }

void RecreateMenu();

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
// AppMessage //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
static const uint32_t FHEM_URL_KEY       = 0;
static const uint32_t FHEM_COM_ID_KEY    = 1;
static const uint32_t FHEM_RESP_KEY      = 2;
static const uint32_t FHEM_URL_GET_STATE = 3;
static const uint32_t FHEM_URL_REQ_TYPE  = 4;
static const uint32_t FHEM_MSG_ID        = 5;

// add new device:
static const uint32_t FHEM_NEW_DEV_BEG   =  6;
static const uint32_t FHEM_DEV_DEVICE    =  7;
static const uint32_t FHEM_DEV_DESCR     =  8;
static const uint32_t FHEM_DEV_STATE     =  9;
static const uint32_t FHEM_DEV_ROOM      = 10;
static const uint32_t FHEM_DEV_CHECK     = 11;
static const uint32_t FHEM_NEW_DEV_END   = 12;


// persisent storage keys
static const uint32_t FHEM_PERSIST_USEDYN           = 0; // storage key to switch between static/dynamic ComMap
static const uint32_t FHEM_PERSIST_MENU_FAV_NUM     = 1; // save number of favourite menu items to build the menu on startup without content
static const uint32_t FHEM_PERSIST_MENU_STATES_NUM  = 2; // save number of states menu items
static const uint32_t FHEM_PERSIST_MENU_SPECIAL_NUM = 3; // save number of special menu items



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
#ifdef DEBUG_ADD_COM
  APP_LOG(APP_LOG_LEVEL_DEBUG, "MsgID: %d of CmdIdx %d", MsgID, (int)index);
#endif
  
  bool successfull = true;


  if ((data = dict_find(iterator, FHEM_URL_REQ_TYPE)) != NULL) {
    char* result = (char*)data->value->cstring;
    if (!strcmp("success", result)) {
      set_menu_icon(MENU_SPECIAL, s_special_menu_id_ReqFHEM, OK);
    } else {
      set_menu_icon(MENU_SPECIAL, s_special_menu_id_ReqFHEM, FAILED);      
    }    
  } else
  if ((data = dict_find(iterator, FHEM_RESP_KEY)) != NULL) {
    char* result = (char*)data->value->cstring;
    APP_LOG(APP_LOG_LEVEL_INFO, "FHEM_RESP_KEY received: %s of index %d", result, (int)index);
    if (!strcmp("success", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx, OK);
      // now fetch state:
      SendComR(index);

    } else if (!strcmp("off", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx,     OFF);
      set_menu_icon(MENU_FAV_COMS, MenuFavIdx,     OFF);
      set_menu_icon(MENU_STATE_COMS, MenuStateIdx, OFF);
    } else if (!strcmp("on", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx,     ON);
      set_menu_icon(MENU_FAV_COMS, MenuFavIdx,     ON);
      set_menu_icon(MENU_STATE_COMS, MenuStateIdx, ON);
    } else if (!strcmp("toggle", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx,     TOGGLE);
      set_menu_icon(MENU_FAV_COMS, MenuFavIdx,     TOGGLE);
      set_menu_icon(MENU_STATE_COMS, MenuStateIdx, TOGGLE);
    } else if (!strcmp("not connected", result)) {
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx, FAILED);
      if (MsgID != MSG_ID_SEND_COM_REQ_STATE_NEXT)
	vibes_long_pulse();
    } else {
      set_menu_text(MENU_STATE_COMS, index, MenuStateIdx, result); // todo
      set_menu_icon(MENU_DEF_COMS, MenuDefIdx,   OK);
      set_menu_icon(MENU_FAV_COMS, MenuFavIdx,   OK);
      set_menu_icon(MENU_STATE_COMS, MenuStateIdx, OK);
    } 
  } else
    if ((data = dict_find(iterator, FHEM_NEW_DEV_BEG)) != NULL) {
      Coms_Cnt = 0;
      set_status("Receiving...");

  } else
    if ((data = dict_find(iterator, FHEM_NEW_DEV_END)) != NULL) {
      Coms_UseDyn = true;
      RecreateMenu();     
      set_status("Devs received.");
      // request all states now
      SendCommand(0, true, MSG_ID_SEND_COM_REQ_STATE_NEXT);
  } else
    if ((data = dict_find(iterator, FHEM_DEV_DEVICE)) != NULL) {
      
      Coms_Map_t NewCom = InitCom();
      
      /* Coms data structure and its members:
    	Coms_Map_t NewCom = {
    	  .Room         = "Küche",
    	  .Description  = "Licht umschalten",
    	  .URL          = NULL,
    	  .Device       = "FS20_fr_bel",
    	  .Command      = "toggle",
    	  .MenuDefIdx   = MenuDef,
    	  .MenuFavIdx   = MenuFav,
    	  .MenuStateIdx = MenuOmit,
      */
#ifdef DEBUG_ADD_COM
      APP_LOG(APP_LOG_LEVEL_INFO, "FHEM_DEV_DEVICE received: %s", (char*)data->value->cstring);
#endif
      
      NewCom.Device = AddNewStr((char*)data->value->cstring);
      if ((data = dict_find(iterator, FHEM_DEV_DESCR)) != NULL) {
      	NewCom.Description = AddNewStr((char*)data->value->cstring);
#ifdef DEBUG_ADD_COM
	APP_LOG(APP_LOG_LEVEL_INFO, "FHEM_DEV_DESCR received: %s", NewCom.Description);
#endif
      }
      // todo: cleanup state and command
      if ((data = dict_find(iterator, FHEM_DEV_STATE)) != NULL) {
	NewCom.Command = AddNewStr((char*)data->value->cstring);
#ifdef DEBUG_ADD_COM
	APP_LOG(APP_LOG_LEVEL_INFO, "FHEM_DEV_STATE received: %s", NewCom.Command);
#endif
      }
      if ((data = dict_find(iterator, FHEM_DEV_ROOM)) != NULL) {
	NewCom.Room = AddNewStr((char*)data->value->cstring);
#ifdef DEBUG_ADD_COM
	APP_LOG(APP_LOG_LEVEL_INFO, "FHEM_DEV_ROOM received: %s", NewCom.Room);
#endif
      }
      if ((data = dict_find(iterator, FHEM_DEV_CHECK)) != NULL) {
      	MenuBits_t MenuBits = (MenuBits_t)data->value->int32;
      	NewCom.MenuDefIdx   = (MenuBits & MenuDefB)   ? MenuDef   : MenuOmit;
      	NewCom.MenuFavIdx   = (MenuBits & MenuFavB)   ? MenuFav   : MenuOmit;
      	NewCom.MenuStateIdx = (MenuBits & MenuStateB) ? MenuState : MenuOmit;
#ifdef DEBUG_ADD_COM
      	APP_LOG(APP_LOG_LEVEL_INFO, "FHEM_DEV_CHECK received: %d", MenuBits);
#endif
      }
      NewCom.MenuDefIdx = MenuDef; // todo
      
      AddCom(&NewCom);
      
#ifdef DEBUG_ADD_COM
      APP_LOG(APP_LOG_LEVEL_DEBUG, "... added new com. Now we have %d coms",
	      GetNumComs());
#endif

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
  snprintf(URL, size, "%s?cmd=jsonlist2%%20%s&XHR=1", FHEM_URL, 
	   PCom->Device);
  return true;
}

bool BuildFhemTypeURL(const MapIdx_t index, const char Type[], char URL[], int size)
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
  snprintf(URL, size, "%s?cmd=jsonlist2%%20TYPE=%s&XHR=1", FHEM_URL, Type);
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

  APP_LOG(APP_LOG_LEVEL_DEBUG, "SendCommand (Index: %d. MsgID %d). URL: %s", 
	  index, MsgID, URL);   

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
    // todo: something buggy here. sometimes the URL is not parseable from JSON
    // maybe move creation of complete URL to Pebble.js
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

// Todo: do we need this?
static Window *s_window;


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
static GBitmap *s_menu_icon_image_toggle;

#ifdef DICTATE_COMS_IN_MENU
static SimpleMenuItem s_def_menu_items[NUM_COM];
#endif

static SimpleMenuItem s_fav_menu_items[NUM_COM];
static int s_menu_fav_NumItems = 0;
static SimpleMenuItem s_states_menu_items[NUM_COM];
static int s_menu_states_NumItems = 0;
static SimpleMenuItem s_special_menu_items[MAX_NUM_MENU_ITEMS_FAVOURITES];
static int s_menu_special_NumItems = 0;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static int s_menu_sections_NumItems = 0;
static SimpleMenuLayer* s_simple_menu_layer;

// Create and destroy menu
bool CreateMenu(bool InitialEmpty);
void DestroyMenu(SimpleMenuLayer* Layer);

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
#ifdef DICTATE_COMS_IN_MENU
    case MENU_DEF_COMS:
      pMenu = &s_def_menu_items[index];
      break;   
#endif
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
    case TOGGLE:
      pMenu->icon = s_menu_icon_image_toggle;
      break;
  }
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
 
  return true;
}

void set_status(const char text[])
{
  s_special_menu_items[s_special_menu_id_status].subtitle = text;
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
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
#ifdef DICTATE_COMS_IN_MENU
    case MENU_DEF_COMS:
      pMenu = &s_def_menu_items[index];
      break;   
#endif
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

#ifdef DICTATE_COMS_IN_MENU
static void def_menu_select_callback(int index, void *ctx)
{
  MapIdx_t CmdIdx = GetCmdIdxFromDefMenu(index);
  if (CmdIdx < 0) return;  

  if (SendCommand(CmdIdx, s_special_flag, MSG_ID_DEFAULT))
    set_menu_icon(MENU_DEF_COMS, index, SEND);
  else
    set_menu_icon(MENU_DEF_COMS, index, FAILED);
}
#endif

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

#ifdef DICTATE_COMS_IN_MENU
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
#endif


static void switch_stat_dyn_callback(int index, void* ctx)
{
  Coms_UseDyn = !Coms_UseDyn;

  RecreateMenu();

  // for debugging
  for (int i=0; i < GetNumComs(); i++) {
    PrintCom(GetCom(i));
  }

  // simple_menu_layer_set_selected_index(s_simple_menu_layer, index, false);
}


static void request_fs20_devices_callback(int index, void* ctx)
{
  char URL[1024];

  if (!BuildFhemTypeURL(index, "FS20", URL, 1024))
    return;

  if (strlen(URL) == 0)
    return;

  MsgID_t MsgID=MSG_ID_REQ_TYPE;

  AppMessageResult Res = APP_MSG_OK;
    
  DictionaryIterator *iter;
  if ((Res=app_message_outbox_begin(&iter)) != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "ERROR: app_message_outbox_begin: %d", Res);   
    return;
  }

  if (!iter) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error creating outbound message!");
    return;
  }
  
  DictionaryResult ResDict;
  if ((ResDict=dict_write_cstring(iter, FHEM_URL_REQ_TYPE, URL)) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Dict write url error: %d!", ResDict);
    return;
  }
  
  if ((ResDict=dict_write_int(iter, FHEM_COM_ID_KEY, (int*)&index, sizeof(int), true)) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dict write com id error: %d", ResDict);
    return;
  }

  if ((ResDict=dict_write_int(iter, FHEM_MSG_ID, &MsgID, sizeof(int), true)) != DICT_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Dict write MsgID error: %d", ResDict);
    return;
  }
  
  if (app_message_outbox_send() != APP_MSG_OK) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "app_message_outbox_send: %d", Res);   
    return;
  }

  set_menu_icon(MENU_SPECIAL, s_special_menu_id_ReqFHEM, SEND);
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

  s_special_menu_items[MenuCnt] = (SimpleMenuItem) {
    .title    = "Request",
    .subtitle = "FHEM Devices",
    .callback = request_fs20_devices_callback,
  };
  s_special_menu_id_ReqFHEM = MenuCnt++;

  s_special_menu_items[MenuCnt] = (SimpleMenuItem) {
    .title = "Switch coms list",
    .callback = switch_stat_dyn_callback,
    .subtitle = Coms_UseDyn ? "dynamic" : "static",
  };
  s_special_menu_id_status = MenuCnt++;
  // s_special_status_item=&s_special_menu_items[];
  
  return MenuCnt;
}

#ifdef DICTATE_COMS_IN_MENU
int create_default_menu()
{
  int MenuCnt=0;

  /*
  s_def_menu_items[MenuCnt++] = (SimpleMenuItem) {
    .title    = "Send/Receive Mode",
    .callback = volume_select_callback,
    .icon     = s_menu_icon_image_send,
  };
  */

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
#endif


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

#ifdef REQUEST_STATES_IN_MAIN_MENU
  s_states_menu_items[MenuCnt++] = (SimpleMenuItem) {
    .title = "Request",
    .subtitle = "States",
    .callback = request_states_select_callback,
    .icon = s_menu_icon_image_send,
  };
#endif

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

void RecreateMenu()
{
  if (CreateMenu(false)) {
    // new menu has same items as previous. So we dont need to destroy the menu and rebuild
    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "RecreateMenu: No rebuild");
    return;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "RecreateMenu: Doing a rebuild!!!!!");
  
  DestroyMenu(s_simple_menu_layer);
  
  Layer *window_layer = window_get_root_layer(s_window);
  GRect bounds = layer_get_frame(window_layer);
  
  s_simple_menu_layer = simple_menu_layer_create(bounds, s_window, 
						 s_menu_sections,
						 s_menu_sections_NumItems,
						 NULL);
  
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}


// InitialEmpty: true: the contents of the menu items are not filled
bool CreateMenu(bool InitialEmpty)
{
  bool NumItemsChanged=false;

  // create menu sections
  int NumItems=0;
  int NumSections=0;

  // special menu

#if 0
  // we always can create the special menu since the menu items dont change
  if (InitialEmpty && s_menu_special_NumItems != 0)
    NumItems = s_menu_special_NumItems;
  else 
#endif
  {
    NumItems = create_special_menu();
    
    if (s_menu_special_NumItems != NumItems) {
      NumItemsChanged = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "CreateMenu: special old and new: %d <> %d",
	      s_menu_special_NumItems, NumItems);
      s_menu_special_NumItems = NumItems;
    }
  }
  if (NumItems > 0) {
    s_menu_sections[NumSections++] = (SimpleMenuSection) {
      .title = "Special",
      .num_items = NumItems,
      .items = s_special_menu_items,
    };
  }

  // favourite menu
  if (InitialEmpty && s_menu_fav_NumItems != 0)
    NumItems = s_menu_fav_NumItems;
  else {
    NumItems = create_favourites_menu();
    
    if (s_menu_fav_NumItems != NumItems) {
      NumItemsChanged = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "CreateMenu: fav old and new: %d <> %d",
	      s_menu_fav_NumItems, NumItems);
      s_menu_fav_NumItems = NumItems;
    }
  }
  s_menu_sections[NumSections++] = (SimpleMenuSection) {
    .title = "Favourites",
    .num_items = NumItems,
    .items = s_fav_menu_items,
  };

  // states menu
  if (InitialEmpty && s_menu_states_NumItems != 0)
    NumItems = s_menu_states_NumItems;
  else {
    NumItems = create_states_menu();
    if (s_menu_states_NumItems != NumItems) {
      NumItemsChanged = true;
      APP_LOG(APP_LOG_LEVEL_DEBUG, "CreateMenu: States old and new: %d <> %d",
	      s_menu_states_NumItems, NumItems);
      s_menu_states_NumItems = NumItems;
    }
  }
  s_menu_sections[NumSections++] = (SimpleMenuSection) {
    .title = "States",
    .num_items = NumItems,
    .items = s_states_menu_items,
  };

#ifdef DICTATE_COMS_IN_MENU
  // default menu
  NumItems=create_default_menu();
  s_menu_sections[NumSections++] = (SimpleMenuSection) {
    .title = "Default Commands",
    .num_items = NumItems,
    .items = s_def_menu_items,
  };
#endif

  s_menu_sections_NumItems = NumSections;

  if (!NumItemsChanged) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "CreateMenu: number of items are the same. No rebuild");
    return true;
  }

  return false;
}


void DestroyMenu(SimpleMenuLayer* Layer)
{
  simple_menu_layer_destroy(Layer);
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
  s_menu_icon_image_toggle = gbitmap_create_with_resource(RESOURCE_ID_STATUS_TOGGLE);

  //window_set_background_color(window, GColorYellow);

  // test dynamic com_map
  // TestAddCom();

  CreateMenu(true);
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  s_simple_menu_layer = simple_menu_layer_create(bounds, window, 
						 s_menu_sections, 
						 s_menu_sections_NumItems,
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
  DestroyMenu(s_simple_menu_layer);
  gbitmap_destroy(s_menu_icon_image_toggle);
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

static void init(void)
{

  if (persist_exists(FHEM_PERSIST_USEDYN)) {
    Coms_UseDyn = persist_read_bool(FHEM_PERSIST_USEDYN);
  }

  if (persist_exists(FHEM_PERSIST_MENU_FAV_NUM)) {
    s_menu_fav_NumItems = persist_read_int(FHEM_PERSIST_MENU_FAV_NUM);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Read storage: fav: %d", s_menu_fav_NumItems);
  } else
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Read storage: fav: -");

  
  if (persist_exists(FHEM_PERSIST_MENU_STATES_NUM)) {
    s_menu_states_NumItems = persist_read_int(FHEM_PERSIST_MENU_STATES_NUM);
  }

  if (persist_exists(FHEM_PERSIST_MENU_SPECIAL_NUM)) {
    s_menu_special_NumItems = persist_read_int(FHEM_PERSIST_MENU_SPECIAL_NUM);
  }

  
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
		   app_message_outbox_size_maximum());
  */
  app_message_open(512, 512);

}

static void deinit(void)
{
  persist_write_int(FHEM_PERSIST_MENU_SPECIAL_NUM, s_menu_special_NumItems);
  persist_write_int(FHEM_PERSIST_MENU_STATES_NUM, s_menu_states_NumItems);
  persist_write_int(FHEM_PERSIST_MENU_FAV_NUM,    s_menu_fav_NumItems);
  
  persist_write_bool(FHEM_PERSIST_USEDYN, Coms_UseDyn);
  FreeStr();
  window_destroy(s_window);
}


int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);

  app_event_loop();
  deinit();
}
