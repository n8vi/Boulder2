function get_tzname() 
{
  var d = new Date()
  var year = d.getFullYear();
  var jan = new Date(year, 1, 1, 0, 0, 0, 0);
  var soffs = jan.getTimezoneOffset();
  var offs = d.getTimezoneOffset();
  var zname = "";

  if (soffs == 300) {
    zname = "E";
  } else if (soffs == 360) {
    zname = "C";
  } else if (soffs == 420) {
    zname = "M";
  } else if (soffs == 480) {
    zname = "P";
    }

  if (offs != soffs) {
    zname = zname + "DT";
  } else {
    zname = zname + "ST"; 
    }

  return zname;
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 };

function locationError(err) {}

function locationSuccess(pos)
{
  var coordinates = pos.coords;
  fetchWeather(coordinates.latitude, coordinates.longitude);
}

function fetchWeather(lat, lon)
{
  var h = new XMLHttpRequest();

  h.open("GET", "http://api.openweathermap.org/data/2.5/weather?lat="+lat+"&lon="+lon, false);
  h.send(null);

  var obj = JSON.parse(h.responseText);
  var kelvin = obj.main.temp;
  var celsius = kelvin-273.15;
  var farenheit = celsius*1.8+32.0;
  kelvin = kelvin.toFixed();
  celsius = celsius.toFixed();
  farenheit = farenheit.toFixed();
	
  var tmps = farenheit+'F/'+celsius+'C';

  var transactionId = Pebble.sendAppMessage( 
  { '0': d.getTimezoneOffset(), '1': get_tzname(), '2': tmps },
  function(e) { console.log('Success'); },
  function(e) { console.log('Failure'); }
  );	
}	

function updatePebble(e)
{
  var locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
}

var d = new Date();

Pebble.addEventListener('ready', updatePebble);
Pebble.addEventListener('appmessage',updatePebble);

