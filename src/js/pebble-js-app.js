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
    // 
    if (e.payload['FHEM_URL_KEY']) {
      console.log('Received URL: ' + e.payload['FHEM_URL_KEY']);
      var response = HTTPGET(e.payload['FHEM_URL_KEY']);
      console.log('Received response: ' + response);
      if (response != null)
	Pebble.sendAppMessage({ 'FHEM_RESP_KEY'   :  'success' },
			      { 'FHEM_COM_ID_KEY' :   e.payload['FHEM_COM_ID_KEY'] });
      else
	Pebble.sendAppMessage({ 'FHEM_RESP_KEY' :  'not connected' });  
      
    } else {
      console.log('Received URL not known');
      console.log('Received message: ' + JSON.stringify(e.payload));
    }


    //    var status = true;

    // var dict = {"KEY_STATUS" : status}

    //    Pebble.sendAppMessage(dict);
  }                     
);
