<?

$request_body = $_GET;
//var_dump($request_body);
//return;

ini_set('max_execution_time', 300); //300 seconds = 5 minutes

//set_error_handler("error_handler");

//$request_body = file_get_contents('php://input');

if($request_body['action']=='stop_server'){
	exec( 'pkill albero');
	echo( 'done' );	
	return;
}

/* Get the port for the WWW service. */
$service_port = 1105;

/* Get the IP address for the target host. */
$address = gethostbyname('localhost');

/* Create a TCP/IP socket. */
$socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
if ($socket === false) {
    echo "socket_create() failed: reason: " . socket_strerror(socket_last_error()) . "\n";
} else {

}

//echo "Attempting to connect to '$address' on port '$service_port'...";

	$result = socket_connect($socket, $address, $service_port);
	if ($result === false) {

			echo "socket_connect() failed.\nReason: ($result) " . socket_strerror(socket_last_error($socket)) . "\n";
			return;		

	
	} else {
		
	}


$in = json_encode($request_body);

socket_write($socket, $in, strlen($in));
/*
while ($out = socket_read($socket, 65535)) {
	break;
}*/


$out="";
while($resp = socket_read($socket, 65535)) {
   $out .= $resp;
   if (strpos($out, "\n") !== false) break;
}

$out = rtrim($out);

/*
$i = 0;

$out="";
while ($partial = socket_read($socket, 65535)) {

if(!$partial){break;}
$i++;
if($i==10) break;

	var_dump($partial);

	$out  .= $partial;	
}
*/
//echo("DOne");
//return;


// open the file in a binary mode
switch($request_body['action'])
{

	case "stop_server":
		exec("pkill albero");
	break;

	// -----------------------------------------------------
	// GET FORECAST (probabilistic)
	// GET CURRENT FORECAST (current numerical forecast)
	// -----------------------------------------------------
	case "get_forecast":
	case "get_current_forecast":
	case "get_num_forecast":
	case "get_observation":
	case "get_mse":


		// Return the image from the string returned by the server.
		try
		{
			$name = $out;

			$fp = fopen($name, 'rb');

			// send the right headers
			header("Content-Type: image/png");
			header("Content-Length: " . filesize($name));

			// dump the picture and stop the script
			fpassthru($fp);
		}
		catch(Exception $e){}

	break;

	case "get_analogs":
	case "get_analog_viewer":
	case "initialize":
	case "ping":
	default:
		echo($out);
	break;

}


socket_close($socket);


function error_handler()
{
    // Check for unhandled errors (fatal shutdown)
    $e = error_get_last();
    var_dump($e);
}

?>
