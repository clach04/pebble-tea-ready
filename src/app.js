Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.openURL('https://clach04.github.io/pebble-tea-ready/tea_config_0.8.html');
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode and parse config data as JSON
  var config_data = JSON.parse(decodeURIComponent(e.response));
  
  // Prepare AppMessage payload
  var dict = {
    'PERSIST_READY': config_data.ready,
    'PERSIST_TEMP_UNIT': config_data.temp_unit,
    'PERSIST_TEA_BLACK': config_data.black * 60 * config_data.black_hide,
    'PERSIST_TEA_GREEN': config_data.green * 60 * config_data.green_hide,
    'PERSIST_TEA_HERBAL': config_data.herbal * 60 * config_data.herbal_hide,
    'PERSIST_TEA_MATE': config_data.mate * 60 * config_data.mate_hide,
    'PERSIST_TEA_OOLONG': config_data.oolong * 60 * config_data.oolong_hide,
    'PERSIST_TEA_PUERH': config_data.puerh * 60 * config_data.puerh_hide,
    'PERSIST_TEA_ROOIBOS': config_data.rooibos * 60 * config_data.rooibos_hide,
    'PERSIST_TEA_WHITE': config_data.white * 60 * config_data.white_hide,
    'PERSIST_TEA_MATCHA': config_data.matcha * 60 * config_data.matcha_hide
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