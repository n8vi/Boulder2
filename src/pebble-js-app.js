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

function updatePebble(e)
{
    var transactionId = Pebble.sendAppMessage( 
	                    { '0': d.getTimezoneOffset(), '1': get_tzname() },
                        function(e) { console.log('Success'); },
                        function(e) { console.log('Failure'); }
	                    );	
}

var d = new Date();

Pebble.addEventListener('ready', updatePebble);
Pebble.addEventListener('appmessage',updatePebble);

