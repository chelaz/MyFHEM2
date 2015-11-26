// https://developer.getpebble.com/guides/pebble-apps/pebblekit-js/js-app-comm/
// https://developer.getpebble.com/guides/pebble-apps/communications/appmessage/
// https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/JSON
// pass options to the settings page
// https://stackoverflow.com/questions/30559207/is-it-possible-to-display-a-settings-page-and-communicate-with-a-phone-using-peb

function GetServerURL(URL_Args)
{
  var URL="";
  //  if(localStorage['FHEM_SERVER_URL'])
  var storageURL = localStorage.getItem('FHEM_SERVER_URL');
  if(storageURL !== null)
    URL = storageURL;
  
  return URL + URL_Args;
}

function HTTPGET(url) {
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
  // var StateURL = e.payload['FHEM_URL_GET_STATE'];
  console.log('GetState: ' + URL);
  var response = HTTPGET(URL);

  // tests:
  /* JSONLIST:
      response = JSON.stringify({
        "ResultSet"  : {
          "Results" : {
            "STATE": "off",
            "Type": "FS20",
            "XMIT": "3bcd"
          }
        }
	});
      // JSONLIST2:
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

  // console.log('Received response: ' + response);

  var dict;
  if (response !== null) {
    var DevJSON = JSON.parse(response);

    /*jsonlist
        var State = DevJSON.ResultSet.Results.STATE;

         console.log('ResultSet:' + DevJSON.ResultSet);
         console.log('ResultSet, Results:' + DevJSON.ResultSet.Results);
         console.log('Received STATE: ' + State);
	  */
    // jsonlist2
    var State = "unknown";
    if (DevJSON.totalResultsReturned == 1) {
      State = DevJSON.Results[0].Internals.STATE;
      // console.log('Results:' + JSON.stringify(DevJSON.Results[0]));
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

function RequestTypes(URL)
{
  console.log('RequestTypes: ' + URL);
  var response = HTTPGET(URL);

  // console.log('Received requested type: ' + response);

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

  var FHEM_Types = { "FS20" : [], "num": 0 };
  if (response !== null) {
    var DevJSON = JSON.parse(response);

    console.log('Received Devices: ' + DevJSON.totalResultsReturned);
    
    var cnt=0;
    for (var i=0; i < DevJSON.totalResultsReturned; i++) {
      var State  = DevJSON.Results[i].Internals.STATE;
      var Device = DevJSON.Results[i].Internals.NAME;
      var Descr  = DevJSON.Results[i].Attributes.alias;
      var Room   = DevJSON.Results[i].Attributes.room;
      console.log('\t#: ' + i);
      if (Room == null) {
	console.log('\t  -> not used device: ' + JSON.stringify(Device));
	continue;
      }
      console.log('\t  Room:   ' + JSON.stringify(Room));
      console.log('\t  Descr:  ' + JSON.stringify(Descr));
      console.log('\t  Device: ' + JSON.stringify(Device));
      console.log('\t  State:  ' + JSON.stringify(State));
      
      FHEM_Types["FS20"].push( 
        { 
          "State"  : State,
          "Device" : Device,
          "Descr"  : Descr,
          "Room"   : Room
        });
      cnt++;
    }
    FHEM_Types.num = cnt;
  }
    // test:
  /*
  FHEM_Types = {
      "FS20" : [
        { 
          "State"  : "toggle",
          "Device" : "FS20_fr_bel",
          "Descr"  : "Licht umschalten",
          "Room"   : "Küche"
        },
        { 
          "State"  : "toggle",
          "Device" : "FS20_fz_bel",
          "Descr"  : "Licht an/aus",
          "Room"   : "Wohnzimmer"
        }
      ],
      "num": 2
      };  */
  
    /* FHEM_Types = {
      "FS20" : "test"
    }; */

    // console.log('Set in local storage (FHEM_URL_REQ_TYPE): ' + JSON.stringify(FHEM_Types));

    localStorage.setItem('FHEM_URL_REQ_TYPE', JSON.stringify(FHEM_Types));
  // }
  return JSON.stringify(FHEM_Types);
}


//////////////////////////////////////////////////////////////////////////////////////
// Pebble functions //////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('MyFHEM2 JavaScript ready!');
    
    var FHEM_Types = RequestTypes(GetServerURL("?cmd=jsonlist2%20TYPE=FS20&XHR=1"));

    // console.log('MyFHEM2: Types: '+FHEM_Types);
  }
);

// https://developer.getpebble.com/guides/pebble-apps/pebblekit-js/app-configuration/#testing-on-pebble
Pebble.addEventListener('showConfiguration', 
  function(e) {
    var url = 'https://rawgit.com/chelaz/MyFHEM2/master/config/index.html';
    // var url = 'http://madita/config/index.html';
   
    var FHEM_Types = localStorage.getItem('FHEM_URL_REQ_TYPE');
    
    var FHEM_Types_Obj = {
      "FS20" : [
        { 
          "State"  : "toggle",
          "Device" : "FS20_fr_bel",
          "Descr"  : "Licht umschalten",
          "Room"   : "Küche"
        },
        { 
          "State"  : "toggle",
          "Device" : "FS20_fz_bel",
          "Descr"  : "Licht an/aus",
          "Room"   : "Wohnzimmer"
        }
      ],
      "num": 2
  }; 
  
    // var FHEM_Types = JSON.stringify(FHEM_Types_Obj); 
  // var FHEM_Types = RequestTypes(GetServerURL("?cmd=jsonlist2%%20TYPE=%s&XHR=1"));
        
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
    
    // localStorage['FHEM_SERVER_URL'] = FHEM_SERVER_URL;
    localStorage.setItem('FHEM_SERVER_URL', FHEM_SERVER_URL);
    /*
      // Example with the following JSON data:
      var config = {
        'animSetting': true,
        'tickSetting': 'seconds',
        'bgColor': 'red'
      };
      
    // Prepare AppMessage payload
    var dict = {
      'KEY_ANIMATIONS': config_data[animSetting],
      'KEY_TICK': config_data[tickSetting],
      'KEY_BACKGROUND_COLOR': config_data[bgColor]
    };
    
    // Send settings to Pebble watchapp
    Pebble.sendAppMessage(dict, function(){
			    console.log('Sent config data to Pebble');  
			  }, function() {
			    console.log('Failed to send config data!');
			  });
    */
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
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
      RequestTypes(GetServerURL(e.payload['FHEM_URL_REQ_TYPE']));
      return;
    }
  }                     
);
