Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://evangoo.de/pebble/metronome/index.html';

  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(event) {
  var configData = JSON.parse(decodeURIComponent(event.response));

  console.log('Configuration page returned: ' + JSON.stringify(configData));

  if (configData.vibe_duration) {
    Pebble.sendAppMessage({
      vibe_duration: configData.vibe_duration
    }, function() {
      console.log('send successful');
    }, function() {
      console.log('send failed');
    });
  }
});

