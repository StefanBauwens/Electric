// clay initialization
var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
// default config
var clay = new Clay(clayConfig, null, {autoHandleEvents: false});
var messageKeys = require('message_keys');

const TIMEOUT_MS = 1000;


var xhrRequest = function (url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
      callback(this.responseText);
    };
    xhr.open(type, url);
    xhr.send();
};

function locationSuccess(pos) {
    //store last position
    localStorage.setItem("LAST_LATITUDE", pos.coords.latitude);
    localStorage.setItem("LAST_LONGITUDE", pos.coords.longitude);
    
    fetchWeather();
}

function fetchWeather() { //actual weather fetching is here
    // We will request the weather here
    var apiKey = localStorage.getItem("OPENWEATHER_APIKEY");
    var useFahrenheit = localStorage.getItem("USEFAHRENHEIT");

    var lat = localStorage.getItem("LAST_LATITUDE");
    var lon = localStorage.getItem("LAST_LONGITUDE");

    if (apiKey === null || apiKey == "" || lat === null)
    {
        console.log("Apikey is null. Returning empty string");
        var dict = { "temp": " " };

            // Send to Pebble
            setTimeout( 
                Pebble.sendAppMessage(dict,
                    function(e) {
                        console.log('Empty weather info sent to Pebble successfully!');
                    },
                    function(e) {
                        console.log('Error sending empty weather info to Pebble!');
                    }
                ),
            TIMEOUT_MS);
        return; //since we have no api key and no location data
    }

    // Construct URL
    var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
    lat + '&lon=' + lon + '&appid=' + apiKey + '&units=' + (useFahrenheit == 1?'imperial':'metric');

    // Send request to OpenWeatherMap
    xhrRequest(url, 'GET', 
        function(responseText) {
            // responseText contains a JSON object with weather info
            var json = JSON.parse(responseText);
            var temperature = 'Invalid API key'; //default return temp

            if (json.cod != 401)
            {
                //we use toFixed to get .0 at the end even if it's nice and round
                temperature = (Math.round(json.main.temp*10)/10).toFixed(1) + (useFahrenheit == 1?'°F':'°C');
                console.log('Temperature is ' + temperature);
            }

            var dict = { "temp": temperature };

            // Send to Pebble
            setTimeout( //timeout helps prevent errors
                Pebble.sendAppMessage(dict,
                    function(e) {
                        console.log('Weather info sent to Pebble successfully!');
                    },
                    function(e) {
                        console.log('Error sending weather info to Pebble!');
                    }
                ),
            TIMEOUT_MS);
        }      
    );
}
  
function locationError(err) {
    console.log('Error requesting location! Using last saved data...'); //TODO use failsafe then?
    fetchWeather();
}
  
function getWeather() { //will try to get location, then actually fetch weather
    navigator.geolocation.getCurrentPosition(
        locationSuccess,
        locationError,
        {timeout: 25000, maximumAge: 60000}
    );
}
  
// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
    function(e) {
        console.log('PebbleKit JS ready!');

        // Get the initial weather
        getWeather();
    }   
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    getWeather();
  }                     
);

Pebble.addEventListener('showConfiguration', //we need to implement this since we are overriding events in webviewclosed
  function(e) {
    clay.config = clayConfig;
    Pebble.openURL(clay.generateUrl());
});

// Listen for when web view is closed
Pebble.addEventListener('webviewclosed', //TODO PEbble config too big, so perhaps we have to replace sending data to pebble to optimise it? //send it in chunks?
    function(e) {
    if (e && !e.response) { return; }
  
    var dict = clay.getSettings(e.response); //dict is too large so we only send neccesary stuff:

    var dictKeys = Object.keys(dict);
    var newDict = {};
    for (var i = 0; i < dictKeys.length - 2; i++) //we skip temperature stuff
    {
	    newDict[dictKeys[i]] = dict[dictKeys[i]];
    }

    // Send settings values to watch side
    Pebble.sendAppMessage(newDict, function(e) {
        console.log('Sent config data to Pebble');
    }, function(e) {
        console.log('Failed to send config data!');
        console.log(JSON.stringify(e));
    });

    console.log("apikey: " + dict[messageKeys.apiKey]);
    localStorage.setItem("OPENWEATHER_APIKEY", dict[messageKeys.apiKey]);
    localStorage.setItem("USEFAHRENHEIT", dict[messageKeys.useFahrenheit]);
    getWeather(); //call getweather every time config is set in case user changed degrees settings
  }
);
