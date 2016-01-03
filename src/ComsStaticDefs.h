#ifndef _COMSSTATICDEFS_H_
#define _COMSSTATICDEFS_H_

// use this to specify in which menu the command shall appear.
typedef enum MenuIdx_ {
  MenuOmit  = -1,
  MenuDef   = -2,
  MenuFav   = -3,
  MenuState = -4,
} MenuIdx_t;

typedef enum MenuBits_ {
  MenuDefB   = 1,
  MenuFavB   = 2,
  MenuStateB = 4,
} MenuBits_t;

typedef int MapIdx_t; // type to access Coms_Map array (index for array)

// the menu indices are overwritten during creation of menu if not MenuOmit
typedef struct Coms_Map_
{
  const char* Room;
  const char* Description;
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
    MenuDef, MenuFav, MenuOmit,
  },
  {
    "Küche",
    "Licht", NULL,
    "FS20_fr_bel", "toggle",
    MenuOmit, MenuOmit, MenuState,
  },
  {
    "Küche",
    "Licht an", NULL,
    "FS20_fr_bel", "on",
    MenuOmit, MenuOmit, MenuOmit,
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

#endif
