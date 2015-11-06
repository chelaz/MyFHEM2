
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
    if (e.payload[0xabbababe]) {
      console.log('Received URL: ' + e.payload[0xabbababe]);
      var response = HTTPGET(e.payload[0xabbababe]);
    } else {
      console.log('Received URL not known');
      console.log('Received message: ' + JSON.stringify(e.payload));
    }


    //    var status = true;

    // var dict = {"KEY_STATUS" : status}

    //    Pebble.sendAppMessage(dict);
  }                     
);
