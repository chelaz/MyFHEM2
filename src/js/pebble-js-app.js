// https://developer.getpebble.com/guides/pebble-apps/pebblekit-js/js-app-comm/
// https://developer.getpebble.com/guides/pebble-apps/communications/appmessage/
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/JSON
// pass options to the settings page
// https://stackoverflow.com/questions/30559207/is-it-possible-to-display-a-settings-page-and-communicate-with-a-phone-using-peb


///////////////////////////////
// IDs:
///////////////////////////////
// localStorage:
//   FHEM_SERVER_URL:   server URL eg. "http://mypi:8083/fhem"
//   FHEM_URL_REQ_TYPE: received devices from FHEM
//   FHEM_DEVS_CONFIG:  received device configuration from settings page
//
///////////////////////////////
// appKeys (see also appinfo.json)
//   FHEM_URL_KEY       (R) special url from watch (SendCom)
//   FHEM_URL_GET_STATE (R) special url from watch (GetState)
//   FHEM_URL_REQ_TYPE  (S/R) special url from watch (RequestType from FHEM)
//   FHEM_RESP_KEY      (S) send command/get state response -> watch
//   FHEM_COM_ID_KEY    (S/R) given ID from watch
//   FHEM_MSG_ID        (S/R) given message ID from watch
//   FHEM_NEW_DEV_BEG   (S) Indicates that new devices are sent
//   FHEM_DEV_DEVICE    (S) FHEMDevice.Device
//   FHEM_DEV_DESCR     (S) FHEMDevice.Descr
//   FHEM_DEV_STATE     (S) FHEMDevice.State
//   FHEM_DEV_ROOM      (S) FHEMDevice.Room
//   FHEM_DEV_CHECK     (S) FHEMDevice.Check
//   FHEM_NEW_DEV_END   (S) Indicates the end of sending new devices
//
///////////////////////////////
// global variables
//   Cfg_Devices_buf: buffer array of devices to be sent to watch (finally empty)
//   SendBusy:        locking mechanism for Cfg_Devices_buf






function GetServerURL(URL_Args)
{
  var URL="";
  //  if(localStorage['FHEM_SERVER_URL'])
  var storageURL = localStorage.getItem('FHEM_SERVER_URL');
  if(storageURL !== null)
    URL = storageURL;
  
  return URL + URL_Args;
}

function HTTPGET(url)
{
    var req = new XMLHttpRequest();
    req.open("GET", url, false);
    req.send(null);
    return req.responseText;
}


function SendCom(ComID, MsgID, URL)
{
  console.log('SendCom: ' + URL);
  var response = HTTPGET(URL);

  var dict;
  if (response !== null) {
    dict = { 'FHEM_RESP_KEY'  :  'success',
             'FHEM_COM_ID_KEY' :  ComID,
             'FHEM_MSG_ID'     :  MsgID };
  } else {
    dict = { 'FHEM_RESP_KEY'  :  'not connected',
             'FHEM_COM_ID_KEY' :  ComID,
             'FHEM_MSG_ID'     :  MsgID };
  }

  Pebble.sendAppMessage(dict,
                        function(e) {
                          console.log('AppMsg: SendCom successful.');
                        },
                        function(e) {
                          console.log('AppMsg: SendCom failed!');
                        }
                       );
  return;
}

function GetState(ComID, MsgID, URL)
{
  console.log('GetState: ' + URL);
  var response = HTTPGET(URL);

  // tests:
  /* // JSONLIST2:
    response = JSON.stringify({
	  { 
	    "Arg":"FS20_fr_bel", 
	      "Results": [ 
			  { 
			    "Name":"FS20_fr_bel", 
			    "Internals": { 
			      "DEF": "3bcd 11", 
			      "NAME": "FS20_fr_bel", 
			      "STATE": "toggle", 
			      "TYPE": "FS20", 
			    }, 
			    "Readings": {
			      "state": { "Value":"toggle" }
			    },
			    "Attributes": { 
			      "IODev": "FHZ_0", 
			      "alias": "Beleuchtung Kueche", 
			      "room": "Kueche"
			    }
			  }
			 ], 
	      "totalResultsReturned":1 
	      }
	});
  */


  var dict;
  if (response !== null) {
    var DevJSON = JSON.parse(response);

    // jsonlist2
    var State = "unknown";
    if (DevJSON.totalResultsReturned == 1) {
      State = DevJSON.Results[0].Internals.STATE;
      // console.log('Received Internals: ' + JSON.stringify(DevJSON.Results[0].Internals));
    }

    dict = { 'FHEM_RESP_KEY'   :  State,
             'FHEM_COM_ID_KEY' :  ComID,
             'FHEM_MSG_ID'     :  MsgID };
  } else {
    dict = { 'FHEM_RESP_KEY'   :  'not connected',
             'FHEM_COM_ID_KEY' :  ComID,
             'FHEM_MSG_ID'     :  MsgID };
  }

  Pebble.sendAppMessage(dict,
                        function(e) {
                          console.log('AppMsg: GetState successful.');
                        },
                        function(e) {
                          console.log('AppMsg: GetState failed!');
                        }
                       );

}

function RequestTypes(URL, DeviceType, RoomFilter)
{
  console.log('RequestTypes: ' + URL + "of types: " + DeviceType);
  var response = HTTPGET(URL);

  // jsonlist2 TYPE=FS20
  /*
      response = JSON.stringify({
	  "Arg":"TYPE=FS20",
	    "Results": [
			{
			  "Name":"FS20_fr_bel",
			    "PossibleSets":"dim06% dim100% dim12% dim18% dim25% dim31% dim37% dim43% dim50% dim56% dim62% dim68% dim75% dim81% dim87% dim93% dimdown dimup dimupdown off off-for-timer on on-100-for-timer-prev on-for-timer on-old-for-timer on-old-for-timer-prev ramp-off-time ramp-on-time reset sendstate timer toggle dim:slider,0,6.25,100 blink intervals off-till on-till",
			    "PossibleAttrs":"verbose:0,1,2,3,4,5 room group comment alias eventMap userReadings IODev follow-on-for-timer:1,0 follow-on-timer do_not_notify:1,0 ignore:1,0 dummy:1,0 showtime:1,0 event-on-change-reading event-on-update-reading event-aggregator event-min-interval stateFormat model:dummyDimmer,dummySender,dummySimple,fs20as1,fs20as4,fs20bf,fs20bs,fs20di,fs20di10,fs20du,fs20fms,fs20hgs,fs20irl,fs20kse,fs20ls,fs20ms2,fs20pira,fs20piri,fs20piru,fs20rgbsa,fs20rst,fs20rsu,fs20s16,fs20s20,fs20s4,fs20s4a,fs20s4m,fs20s4u,fs20s4ub,fs20s8,fs20s8m,fs20sa,fs20sd,fs20si3,fs20sig,fs20sm4,fs20sm8,fs20sn,fs20sr,fs20ss,fs20st,fs20st2,fs20str,fs20su,fs20sv,fs20tc1,fs20tc6,fs20tfk,fs20tk,fs20ue1,fs20usr,fs20uts,fs20ws1,fs20ze cmdIcon devStateIcon devStateStyle icon sortby webCmd widgetOverride userattr",
			    "Internals": {
			    "BTN": "11",
			      "DEF": "3bcd 11",
			      "NAME": "FS20_fr_bel",
			      "NR": "335",
			      "STATE": "toggle",
			      "TYPE": "FS20",
			      "XMIT": "3bcd"
			      },
			    "Readings": {      "state": { "Value":"toggle", "Time":"2015-11-21 17:25:16" }    },
			    "Attributes": {
			    "IODev": "FHZ_0",
			      "alias": "Beleuchtung Kueche",
			      "room": "Kueche"
			      }
			},
			],
	    "totalResultsReturned":26
	    });
      */

  // var FHEM_Types = { DeviceType : [], "num": 0 };
  var FHEM_Types = {};
  FHEM_Types[DeviceType] = [];
  FHEM_Types.num = 0;
  
  if (response !== null) {
    var DevJSON = JSON.parse(response);

    console.log('Received Devices: ' + DevJSON.totalResultsReturned);
    
    var cnt=0;
    for (var i=0; i < DevJSON.totalResultsReturned; i++) {
      var State  = DevJSON.Results[i].Internals.STATE;
      var Device = DevJSON.Results[i].Internals.NAME;
      var Alias  = DevJSON.Results[i].Attributes.alias; // part of Description
      var Room   = DevJSON.Results[i].Attributes.room;
      console.log('\t#: ' + i);
      if (Room == null) {
	console.log('\t  -> not used device: ' + JSON.stringify(Device));
	continue;
      }
      if (RoomFilter != null && RoomFilter != "") {
	var FilterFound = false;
	// remove room RoomFilter
	var Rooms = Room.split(',');
	Room = "";
	for (var j = 0; j < Rooms.length; j++) {
	  if (Rooms[j] != RoomFilter) {
	    if (Room == "")
	      Room = Rooms[j];
	    else
	      Room = Room + "," + Rooms[j];
	  } else
	    FilterFound = true;
	}
	if (!FilterFound) {
	  console.log('\t  -> RoomFilter not found for device: ' 
		      + JSON.stringify(Device));  
	  continue;
	}
      }

      console.log('\t  Room:   ' + JSON.stringify(Room));
      console.log('\t  Alias:  ' + JSON.stringify(Alias));
      console.log('\t  Device: ' + JSON.stringify(Device));
      console.log('\t  State:  ' + JSON.stringify(State));
      
      FHEM_Types[DeviceType].push( 
        { 
          "State"  : State,
          "Device" : Device,
          "Alias"  : Alias,
          "Room"   : Room
        });
      cnt++;
    }
    FHEM_Types.num  = cnt;
    var today = new Date();
    FHEM_Types.Date = today.toString(); // 'yyyy-MM-dd hh:mm');
  }
    // test:
  /* Todo: rename to "FHEM_Devices"
  FHEM_Types = {
      "FS20" : [
        { 
          "State"  : "toggle",
          "Device" : "FS20_fr_bel",
          "Alias"  : "Licht umschalten",
          "Room"   : "Küche"
        },
        { 
          "State"  : "toggle",
          "Device" : "FS20_fz_bel",
          "Alias"  : "Licht an/aus",
          "Room"   : "Wohnzimmer"
        }
      ],
      "num": 2,
      "Date" : "15.12.2015 12.00"
      };  */
  
    /* FHEM_Types = { "FS20" : "test" }; */

    // console.log('Set in local storage (FHEM_URL_REQ_TYPE): ' + JSON.stringify(FHEM_Types));

  localStorage.setItem('FHEM_URL_REQ_TYPE', JSON.stringify(FHEM_Types));

  // send answer
  var dict;

  if (FHEM_Types.num > 0)
    dict = { 'FHEM_URL_REQ_TYPE' : 'success' };
  else
    dict = { 'FHEM_URL_REQ_TYPE' : 'failed' };

  Pebble.sendAppMessage(dict,
                        function(e) {
                          console.log('AppMsg: RequestTypes successful.');
                        },
                        function(e) {
                          console.log('AppMsg: RequestTypes failed!');
                        }
		       );
  
  return JSON.stringify(FHEM_Types);
}

var DebugSend=false;
var Cfg_Devices_buf = null;
var SendBusy = false;

function SendDevices(DeviceType)
{
  if (Cfg_Devices_buf === null) {
    console.log('SendDevices: List not defined');
    return;
  }
  if (Cfg_Devices_buf.length == 0) {
    console.log('SendDevices: no devices available ');
    return;
  }
  
  var dict = {
    'FHEM_NEW_DEV_BEG' : DeviceType
  };

  Pebble.sendAppMessage(dict, ack, function(e) { console.log('AppMsg: send FHEM_NEW_DEV_BEG failed!'); });
                        
  function ack() {
    if (DebugSend)
      console.log('AppMsg: Starting to send ' + Cfg_Devices_buf.length + ' new devices');
    SendBusy = false;
    if (Cfg_Devices_buf.length) {
      SendNextDevice();
    }
  }
}

function SendNextDevice()
{
  if (Cfg_Devices_buf.length === 0) {
    console.log('SendNextDev: List empty');
    return;
  }
  
  var FHEMDevice = Cfg_Devices_buf.shift();
  
  if (DebugSend)
    console.log('SendNextDev: shifted device: ' + JSON.stringify(FHEMDevice));

  if (!SendBusy) {
    SendBusy = true;
    
    var dict = {
      'FHEM_DEV_DEVICE' : FHEMDevice.Device,
      'FHEM_DEV_DESCR'  : FHEMDevice.Descr,
      'FHEM_DEV_STATE'  : FHEMDevice.State,
      'FHEM_DEV_ROOM'   : FHEMDevice.Room,
      'FHEM_DEV_CHECK'  : FHEMDevice.Check
    };
      
    Pebble.sendAppMessage(dict, ack, nack);

    function ack()
    {
      if (DebugSend) 
        console.log('AppMsg: TYPE_DEVICES successful. Still ' + Cfg_Devices_buf.length + ' to send');
      SendBusy = false;
      if (Cfg_Devices_buf.length) {
	      SendNextDevice();
      } else {
        // finished
        var DeviceType = "FS20"; // todo
        var dict = {
          'FHEM_NEW_DEV_END' : DeviceType
        };

        Pebble.sendAppMessage(dict,
                              function(e) {
                              if (DebugSend)
                                console.log('AppMsg: Sent new devices successful.');
                              },
                              function(e) {
                                console.log('AppMsg: send FHEM_NEW_DEV_END failed!');
                              }
                             );     
      }
    }

    function nack() {
      Cfg_Devices_buf.unshift(FHEMDevice);
      console.log('AppMsg: TYPE_DEVICES failed. Still ' + Cfg_Devices_buf.length + ' to send');
      SendNextDevice();
    }
  } else { // busy
    Cfg_Devices_buf.unshift(FHEMDevice);
  }
}



//////////////////////////////////////////////////////////////////////////////////////
// Pebble functions //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('MyFHEM2 JavaScript ready!');
    var DeviceType = "FS20"; // TODO
    var Cfg_DevicesStr = localStorage.getItem('FHEM_DEVS_CONFIG');
    console.log('CFGDevStr:' + Cfg_DevicesStr);
    if (Cfg_DevicesStr == "None")
      return;
    
    var Cfg_Devices = JSON.parse(Cfg_DevicesStr);
    
    // fill global buffer for SendDevice()
    console.log('Using Cfg_Devices of localStorage:' + Cfg_Devices);
    if (Cfg_Devices != null)
      Cfg_Devices_buf = Cfg_Devices[DeviceType];
    SendDevices(DeviceType);
  }
);

// https://developer.getpebble.com/guides/pebble-apps/pebblekit-js/app-configuration/#testing-on-pebble
Pebble.addEventListener('showConfiguration', 
  function(e) {
    // var url = 'https://rawgit.com/chelaz/MyFHEM2/master/config/index.html';
    var url = 'http://madita/config/index.html';
   
    var FHEM_Types = localStorage.getItem('FHEM_URL_REQ_TYPE');

    /* // for tests:
      var FHEM_Types_Obj = {
      "FS20" : [
        { 
          "State"  : "toggle",
          "Device" : "FS20_fr_bel",
          "Alias"  : "Licht umschalten",
          "Room"   : "Küche"
        },
        { 
          "State"  : "toggle",
          "Device" : "FS20_fz_bel",
          "Alias"  : "Licht an/aus",
          "Room"   : "Wohnzimmer"
        }
      ],
      "num": 2
    };
    // var FHEM_Types = JSON.stringify(FHEM_Types_Obj); 
    */

/*
    if(FHEM_Types !== null) {
      var Cfg_Devices = JSON.parse(localStorage.getItem('FHEM_DEVS_CONFIG'));
      if (Cfg_Devices !== null)
	      FHEM_Types = MergeTypesWithDevices(FHEM_Types, Cfg_Devices);
      url = url + "?options=" + encodeURIComponent(FHEM_Types);
    }
*/
    if(FHEM_Types !== null)
      url = url + "?options=" + encodeURIComponent(FHEM_Types);

    // console.log('Showing configuration page: ' + url);

    Pebble.openURL(url);
  }
);


Pebble.addEventListener('webviewclosed', 
  function(e) {
    var configData = JSON.parse(decodeURIComponent(e.response));
    console.log('Configuration page returned: ' + JSON.stringify(configData));

    var FHEM_SERVER_URL = configData['FHEM_SERVER_URL'];
    console.log('FHEM server URL: ' + FHEM_SERVER_URL);
    
    localStorage.setItem('FHEM_SERVER_URL', FHEM_SERVER_URL);

    var DeviceType = "FS20"; // todo
    
    if (!configData.Cfg_Devices)
      return;
    
    localStorage.setItem('FHEM_DEVS_CONFIG', JSON.stringify(configData.Cfg_Devices));

    
    var Cfg_Devices = JSON.parse(localStorage.getItem('FHEM_DEVS_CONFIG'));
    if (Cfg_Devices !== null) {
      // fill global buffer for SendDevices()
      Cfg_Devices_buf = Cfg_Devices[DeviceType];
      SendDevices(DeviceType);
    }
    
  }
);


// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    // console.log('appmessage received: ' + JSON.stringify(e.payload));   
    var ComID = e.payload['FHEM_COM_ID_KEY'];
    console.log('Received ID: ' + ComID);   

    var MsgID = e.payload['FHEM_MSG_ID'];
    console.log('Received MsgID: ' + MsgID);   
    
    if (e.payload['FHEM_URL_KEY']) {
      SendCom(ComID, MsgID, GetServerURL(e.payload['FHEM_URL_KEY']));
      return;
    }
    if (e.payload['FHEM_URL_GET_STATE']) {
      GetState(ComID, MsgID, GetServerURL(e.payload['FHEM_URL_GET_STATE']));
      return;
    }
    if (e.payload['FHEM_URL_REQ_TYPE']) {
      var DeviceType = "FS20"; // TODO
      var RoomFilter = "Pebble";
      //      var FHEM_Types = RequestTypes(GetServerURL("?cmd=jsonlist2%20TYPE="+DeviceType+"&XHR=1"), DeviceType);
      var FHEM_Types = RequestTypes(GetServerURL("?cmd=jsonlist2&XHR=1"), DeviceType, RoomFilter);
    
      // RequestTypes(GetServerURL(e.payload['FHEM_URL_REQ_TYPE']), "FS20");
      return;
    }
    console.log('Unknown appmessage received: ' + JSON.stringify(e.payload));   
  }                     
);
