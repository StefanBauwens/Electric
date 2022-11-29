// clay initialization
var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
// default config
var clay = new Clay(clayConfig);
var messageKeys = require('message_keys');


var xhrRequest = function (url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
      callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

function locationSuccess(pos) {
    // We will request the weather here
    clay.getSettings()
    //TODO use clay.getSettings() --> for api key and fahrenheit settings, if not set, don't call?
    //https://github.com/pebble/clay#methods
    //https://developer.rebble.io/developer.pebble.com/tutorials/watchface-tutorial/part3/index.html
    //https://api.openweathermap.org/data/2.5/weather?q=Sint-Niklaas,%20Belgium&APPID=c408d3f777d8e4ad81fe4bfce26397e0&units=metric

    //get api key and use fahrenheit
    var apiKey = clayConfig.getItemByMessageKey() //TODO not sure how exactly it should be gotten //TODO or try getting from localstorage?

    // Construct URL
    var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
    pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey;

    // Send request to OpenWeatherMap
    xhrRequest(url, 'GET', 
        function(responseText) {
        // responseText contains a JSON object with weather info
        var json = JSON.parse(responseText);

        // Temperature in Kelvin requires adjustment
        var temperature = Math.round(json.main.temp - 273.15);
        console.log('Temperature is ' + temperature);

        // Conditions
        var conditions = json.weather[0].main;      
        console.log('Conditions are ' + conditions);
        }      
    );
}
  
function locationError(err) {
    console.log('Error requesting location!'); //TODO use failsafe then?
}
  
function getWeather() {
    navigator.geolocation.getCurrentPosition(
        locationSuccess,
        locationError,
        {timeout: 15000, maximumAge: 60000}
    );
}
  
// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
    function(e) {
        console.log('PebbleKit JS ready!');

        // Get the initial weather
        //getWeather();
    }   
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
  }                     
);


// Listen for when web view is closed
Pebble.addEventListener('webviewclosed', //TODO PEbble config too big, so perhaps we have to replace sending data to pebble to optimise it? //send it in chunks?
    function(e) {
    if (e && !e.response) { return; }
  
    var dict = clay.getSettings(e.response);
    console.log("webviewclosed!");
    console.log(e.response);
    console.log(dict[messageKeys.apiKey]); //this works!
    if (dict.apiKey)
    {
        console.log("apikey: " + dict.apiKey);
        localStorage.setItem("APIKEY_KEY", dict.apiKey);
    }
  }
);
