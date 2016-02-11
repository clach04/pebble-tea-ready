Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.openURL('https://leomike.com/pebble/tea_config.html');
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode and parse config data as JSON
  var config_data = JSON.parse(decodeURIComponent(e.response));
  
  // Prepare AppMessage payload
  var dict = {
    'PERSIST_TEA_BLACK': (config_data.black + 4) * 60,
    'PERSIST_TEA_GREEN': (config_data.green + 1) * 60,
    'PERSIST_TEA_HERBAL': (config_data.herbal + 4) * 60,
    'PERSIST_TEA_MATE': (config_data.mate + 4) * 60,
    'PERSIST_TEA_OOLONG': (config_data.oolong + 4) * 60,
    'PERSIST_TEA_PUERH': (config_data.puerh + 4) * 60,
    'PERSIST_TEA_ROOIBOS': (config_data.rooibos + 4) * 60,
    'PERSIST_TEA_WHITE': (config_data.white + 3) * 60
  };
  
  // Send settings to Pebble watchapp
  console.log('Sending configuration: ' + JSON.stringify(dict));
  Pebble.sendAppMessage(dict,
    function(){
      console.log('Configuration sent to Pebble');  
    },
    function() {
      console.log('Failed to send configuration to Pebble');
    });
});