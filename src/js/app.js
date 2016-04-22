Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://bake-san.com/pebble/analogpshock13/';

  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));

  console.log('Configuration page returned: ' + JSON.stringify(configData));

  //if (configData.connectionLostVibe) {
    Pebble.sendAppMessage({
      showSecondsHand: configData.showSecondsHand ,
      connectionLostVibe: configData.connectionLostVibe ,
      colorReverse: configData.colorReverse 
    }, function() {
      console.log('Send successful!');
    }, function() {
      console.log('Send failed!');
    });
 // }
});