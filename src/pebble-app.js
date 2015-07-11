Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL('http://hexahedria.com/misc/squared3config');
});

Pebble.addEventListener('webviewclosed',
  function(e) {
    var configuration = JSON.parse(decodeURIComponent(e.response));
    console.log('Configuration window returned: ', JSON.stringify(configuration));
    Pebble.sendAppMessage(configuration);
  }
);