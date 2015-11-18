// https://developer.getpebble.com/guides/pebble-apps/pebblekit-js/js-app-comm/
// https://developer.getpebble.com/guides/pebble-apps/communications/appmessage/

function HTTPGET(url) {
    var req = new XMLHttpRequest();
    req.open("GET", url, false);
    req.send(null);
    return req.responseText;
}


// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('MyFHEM2 JavaScript ready!');
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    var ComID = e.payload['FHEM_COM_ID_KEY'];
    console.log('Received ID: ' + ComID);   
    
    if (e.payload['FHEM_URL_KEY']) {
      console.log('Received URL: ' + e.payload['FHEM_URL_KEY']);
      var response = HTTPGET(e.payload['FHEM_URL_KEY']);
      console.log('Received response: ' + response);
      
      var dict;
      if (response != null) {
        dict = { 'FHEM_RESP_KEY'   :  'success',
                 'FHEM_COM_ID_KEY' :   e.payload['FHEM_COM_ID_KEY'] };
      } else {
        dict = { 'FHEM_RESP_KEY'   :  'not connected',
                 'FHEM_COM_ID_KEY' :   e.payload['FHEM_COM_ID_KEY'] };        
      }
     
      Pebble.sendAppMessage(dict,
        function(e) {
          console.log('Send successful.');
        },
        function(e) {
          console.log('Send failed!');
        }
      );
      return;
    }

    // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/JSON
    if (e.payload['FHEM_URL_GET_STATE']) {
      var StateURL = e.payload['FHEM_URL_GET_STATE'];
      console.log('Received URL to get state: ' + StateURL);
      var response = HTTPGET(StateURL);
      console.log('Received response: ' + response);
      
      var dict;
      if (response != null) {
        dict = { 'FHEM_RESP_KEY'   :  'success',
                 'FHEM_COM_ID_KEY' :   e.payload['FHEM_COM_ID_KEY'] };
      } else {
        dict = { 'FHEM_RESP_KEY'   :  'not connected',
                 'FHEM_COM_ID_KEY' :   e.payload['FHEM_COM_ID_KEY'] };        
      }
     
      Pebble.sendAppMessage(dict,
        function(e) {
          console.log('Send successful.');
        },
        function(e) {
          console.log('Send failed!');
        }
      );
     
      return;
    }

    //    var status = true;

    // var dict = {"KEY_STATUS" : status}

    //    Pebble.sendAppMessage(dict);
  }                     
);
