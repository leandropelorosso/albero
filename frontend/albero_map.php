<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
   <head>
	<title></title>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<script type="text/javascript" src="http://ecn.dev.virtualearth.net/mapcontrol/mapcontrol.ashx?v=7.0"></script>
	<link rel="stylesheet" href="//code.jquery.com/ui/1.11.2/themes/smoothness/jquery-ui.css">
	<link rel="stylesheet" href="alberoToolbox.css">
	<link rel="stylesheet" href="alberoConfigurationBox.css">
	<link rel="stylesheet" href="alberoAnalogViewer.css">
	<link rel="stylesheet" href="albero_ubermap.css">
	<script src="//code.jquery.com/jquery-1.10.2.js"></script>
	<script src="//code.jquery.com/ui/1.11.2/jquery-ui.js"></script>
	<script src="alberoToolbox.js"></script>
	<script src="alberoAnalogViewer.js"></script>
	<script src="alberoConfigurationBox.js"></script>
	<script src="mipmaps.js"></script>
	<script type="text/javascript" src="jquery.linkedsliders.min.js"></script>
	<script type="text/javascript" src="/ThirdParty/jquery.loadmask.min.js"></script>
	<script type="text/javascript" src="/ThirdParty/elessar.min.js"></script>
	<script type="text/javascript" src="/ThirdParty/jquery.limitslider.js"></script>
	<link rel="stylesheet" type="text/css" href="/ThirdParty/jquery.loadmask.css">
	<link rel="stylesheet" type="text/css" href="/ThirdParty/elessar.css">
	<link rel="stylesheet" type="text/css" href="albero.css">
	<script src="http://momentjs.com/downloads/moment.min.js"></script>
	<script src="https://rawgithub.com/quarterto/Estira/master/index.js"></script>


   </head>


   <body>
	   		
	   <div id="albero-wrapper" style="width:100%; height:100%;">

	   		<div id="labels" style="position: fixed;
			    left: 10px;
			    top: 10px;
			    background-color:white;
			    color: grey;
			    z-index: 10;
			    padding: 3px;
			    border-radius: 4px;
			    padding-left: 10px;
			    padding-right: 11px;
			    padding-bottom: 4px;
			    border: 1px solid #aaaaaa;
		    "> 
	   		<div id="label_lat"></div>
		   		<div id="label_lon"></div>
			</div>
	    	
	    	<div id='mapDiv' style="position:relative; width: 100%; height: 100%;"></div>       

	    	<!-- MINI BINGMAPS COLLECTION-->
    		<div id="mini-bingmaps">

    			<!-- MINI BINGMAP -->
	    		<div id="map-sub-1-wrapper" class="map-sub-wrapper">
	    			<div class="albero_floating_scale">
						<div class="big_scale">
							<img class="img_floting_scale" width="11" height="152" src="albero_resources/bias_scale_small.png" />
							<div class="scale_indicators">
								<div class="max">MAX</div>
								<div class="min">MIN</div>
								<div class="total_max">MAX</div>
								<div class="total_min">MIN</div>
							</div>
						</div>
					</div>
		    		<div id='map-sub-1' class="mini_bing_map"></div>  
		    		<div class='label'>numerical forecast (precipitation accumulation) (mm)</div>     
		    	</div>

    			<!-- MINI BINGMAP -->
	    		<div id="map-sub-2-wrapper" class="map-sub-wrapper" style="display: none;">
	    			<div class="albero_floating_scale">
						<div class="big_scale">
							<img class="img_floting_scale" width="11" height="152" src="albero_resources/bias_scale_small.png" />
							<div class="scale_indicators">
								<div class="max">MAX</div>
								<div class="min">MIN</div>
								<div class="total_max">MAX</div>
								<div class="total_min">MIN</div>
							</div>
						</div>
					</div>
		    		<div id='map-sub-2' class="mini_bing_map"></div>       
		    		<div class='label'>observed precipitation accumulation (mm)</div>     
		    	</div>

    			<!-- MINI BINGMAP -->
	    		<div id="map-sub-3-wrapper" class="map-sub-wrapper">
	    			<div class="albero_floating_scale">
						<div class="big_scale">
							<img class="img_floting_scale" width="11" height="152" src="albero_resources/bias_scale_small.png" />
							<div class="scale_indicators">
								<div class="max">MAX</div>
								<div class="min">MIN</div>
								<div class="total_max">MAX</div>
								<div class="total_min">MIN</div>
							</div>
						</div>
					</div>
		    		<div id='map-sub-3' class="mini_bing_map"></div>       
		    		<div class='label'>mean squared error</div>     
		    	</div>

	    	</div>
	    	  
			<div id="message-box" title="Processing" style="display:none;">
			<p>
				Please wait...
			</p>
			</div>

			
			<? include('alberoToolbox.php') ?> 
			<? include('alberoConfigurationBox.php') ?> 
			<? include('alberoAnalogViewer.php') ?> 
			<? include('mipmaps.php') ?> 
			<? include('alberoFloatingScale.php') ?> 
			

		</div>

   </body>

	<style>
		.labelPushpin
		{
			text-shadow: 1px 0px black, -1px 0px black, 0px -1px black, 0px 1px black;  
		}
	</style>

   
</html>


<script>

	// variables for the hover quads
	var dragStartPosition, lastMouseLatitudeCell, lastMouseLongitudeCell, draggedSelectionQuad = [], dragSelectionQuadEntities;
	var Maps = [];	 // all the maps
	var mapLayers = {}; // this one could go inside the map
	
		
	// variables to see if user clicked or dragged
	var lastCursorPosition, draggedStartPosition, draggedStartLatitude, draggedStartLongitude;
	var draggedAreaWidth, draggedAreaHeight;
	var labelPushpin = null;
	
	// opens a very simple message box.
	function OpenMessageBox(title, message)
	{
		$("#message-box").attr("title", title);
		$("#message-box p").html(message);
		$("#message-box" ).dialog({
			modal: true
		}).dialog("open");
	}

	function CloseMessageBox()
	{
		$( "#dialog-message" ).dialog("close");
	}

	// events to run on mouse click
	var onMouseClickEvents = [];

	// Adds the selected region to the map.
	function AddSelectedRegion(map, location){
		// round the latitude and longitude
		ilatitude = parseInt(location.latitude);
		ilongitude = parseInt(location.longitude);

		// removed the previously added quads
		map.selectionQuadEntities.remove(map.clickedSelectionQuad[0]);
		map.selectionQuadEntities.remove(map.clickedSelectionQuad[1]);

		// generate new polygon for the clicked selector
		map.clickedSelectionQuad[0] = generateSelectionQuad(ilatitude-0.5, ilongitude-0.5, 1.5, 1.5, new Microsoft.Maps.Color(40,0,255,0));						
		map.clickedSelectionQuad[1] = generateSelectionQuad(ilatitude-0.5, ilongitude-0.5, 0.5, 0.5, new Microsoft.Maps.Color(40,0,255,0));						
		
		// Add the shape to the map
		map.selectionQuadEntities.push(map.clickedSelectionQuad[0]);					   			
		map.selectionQuadEntities.push(map.clickedSelectionQuad[1]);					   			
	}

	// Mouse click callback
	function onMouseClick(e) {
	 	if (e.targetType == "map") {
			var point = new Microsoft.Maps.Point(e.getX(), e.getY());
			var locTemp = e.target.tryPixelToLocation(point);

			// add the selected region rectangle
			$.each(Maps, function( i, m ) {
				AddSelectedRegion(m, locTemp);
			});

			// execute all the subscribed events
			for (var i = 0, len = onMouseClickEvents.length; i < len; i++) {
				var fn = onMouseClickEvents[i];
				fn(e, locTemp);
			}
		}			
	}
	
	// Mouse down callback: save cursor position
	function onMouseDown(e) {
	 	if (e.targetType == "map") {
			var point = new Microsoft.Maps.Point(e.getX(), e.getY());
			lastCursorPosition = e;
			dragStartPosition = e;
		}			
	}	

	// Mouse up callback: see if mouse has been moved since last mousedown event
	function onMouseUp(e) {
	 	if (e.targetType == "map") {
			//var point = new Microsoft.Maps.Point(e.getX(), e.getY());
			var cursorPosition = e;
			if(Math.abs(lastCursorPosition.getX()-cursorPosition.getX())>0  || 
			   Math.abs(lastCursorPosition.getY()-cursorPosition.getY())> 0) {
				// the user dragged
		
				var ev = window.event;
				// The user is pressing Ctrl, show the dragged selection
				if (ev.ctrlKey) {
					console.log('the user just dragged to select an area');
					console.log("Lat, Long: " + draggedStartLatitude + ", " + draggedStartLongitude);
					console.log("width, height: " + draggedAreaWidth + " " + draggedAreaHeight);
				}
				
				
			}
			else // the user clicked
			{
				onMouseClick(e);
			}
		}			
	}	
	
	function onMouseMove(e) {
	
		var ev = window.event;
		// if CTRL is being pressed, cancel the map dragging
		if (ev.ctrlKey) {
			e.handled = true;			
		}

	
	 	if (e.targetType == "map") {
			var point = new Microsoft.Maps.Point(e.getX(), e.getY());
			var locTemp = e.target.tryPixelToLocation(point);
			var location = new Microsoft.Maps.Location(locTemp.latitude, locTemp.longitude);
			var zoom=map.getZoom();
						
			var request = {'action':'get_forecast', 
						   'lat':locTemp.latitude, 
						   'lon':locTemp.longitude};
			
			$("#labels #label_lat").text(locTemp.latitude.toFixed(2));
			$("#labels #label_lon").text(locTemp.longitude.toFixed(2));

			// round the latitude and longitude
			ilatitude = parseInt(locTemp.latitude);
			ilongitude = parseInt(locTemp.longitude);

			// console.log("("+locTemp.latitude + ", " + locTemp.longitude + ") = (" + ilatitude + ", " + ilongitude + ")");
			
			// if we need to update current position polygon's position
			if(ilatitude != lastMouseLatitudeCell || ilongitude != lastMouseLongitudeCell)
			{		
				// save the current position
				lastMouseLatitudeCell = ilatitude;
				lastMouseLongitudeCell = ilongitude;

				// display selector on each map
				$.each(Maps, function( i, m ) {

					// remove currently visible polygon
					if(m.selectionQuad!=null){
						m.selectionQuadEntities.remove(m.selectionQuad[0]);
						m.selectionQuadEntities.remove(m.selectionQuad[1]);
					}
					// generate new polygon
					m.selectionQuad[0] = generateSelectionQuad(ilatitude-0.5, ilongitude-0.5, 1.5, 1.5, new Microsoft.Maps.Color(40,255,0,0));						
					m.selectionQuad[1] = generateSelectionQuad(ilatitude-0.5, ilongitude-0.5, 0.5, 0.5, new Microsoft.Maps.Color(40,0,0,255));						
					// Add the shape to the map
					m.selectionQuadEntities.push(m.selectionQuad[0]);					   			
					m.selectionQuadEntities.push(m.selectionQuad[1]);	

				});

				
/*
				
				// The user is pressing Ctrl, show the dragged selection
				if (ev.ctrlKey) {
					e.handled = true;			
					// calculate the size of the dragged area

					var pointDragStartPosition = new Microsoft.Maps.Point(dragStartPosition.getX(), dragStartPosition.getY());
					pointDragStartPosition = e.target.tryPixelToLocation(pointDragStartPosition);					
									
					var w = ilatitude - parseInt(pointDragStartPosition.latitude);
					var h = ilongitude - parseInt(pointDragStartPosition.longitude);
			 
					 draggedAreaWidth = w;
					 draggedAreaHeight = h;
			 
					 draggedStartLatitude = parseInt(pointDragStartPosition.latitude);
					 draggedStartLongitude = parseInt(pointDragStartPosition.longitude);
			 
					// remove currently visible polygon
					if(draggedSelectionQuad[0]!=null){
						dragSelectionQuadEntities.remove(draggedSelectionQuad[0]);
					}			 
			 
					// create the draggedSelectionQuad (it's an array but it's only one)		
					draggedSelectionQuad[0] = generateDraggedSelectionQuad(draggedStartLatitude, draggedStartLongitude, w, h, new Microsoft.Maps.Color(40,255,0,0));						
					dragSelectionQuadEntities.push(draggedSelectionQuad[0]);
					
					// create a push pin with a transparent image
					// and the coordinates of the area
					var pushpinOptions = {
						icon: "data:image/gif;base64,R0lGODlhAQABAIAAAP///////yH5BAEKAAEALAAAAAABAAEAAAICTAEAOw==",/* Transparent 1px x 1px gif with a 1bit palette, created with gimp and encoded with base64 tool in linux.
						text: "(" + draggedStartLatitude + ", " + draggedStartLongitude + ") - (" + (draggedStartLatitude + draggedAreaWidth) + ", " + (draggedStartLongitude + draggedAreaHeight) + ")",
						draggable: false,					
						visible: true,
						width: 200,
						typeName: "labelPushpin"
					};

					if(labelPushpin!=null) map.entities.remove(labelPushpin);
					labelPushpin = new Microsoft.Maps.Pushpin(new Microsoft.Maps.Location(draggedStartLatitude+draggedAreaWidth/2, draggedStartLongitude+draggedAreaHeight/2), pushpinOptions);					
					map.entities.push(labelPushpin);
						
				}
				*/
			}
			
	    }              
	}

	function generateSelectionQuad(latitude, longitude, width, height, color)
	{	
		// Create the locations
		var location1 = new Microsoft.Maps.Location(latitude-width,longitude-height);
		var location2 = new Microsoft.Maps.Location(latitude-width,longitude+height);
		var location3 = new Microsoft.Maps.Location(latitude+width,longitude+height);
		var location4 = new Microsoft.Maps.Location(latitude+width,longitude-height);

		// Create a polygon 
		var vertices = new Array(location1, location2, location3, location4, location1);
		var polygon = new Microsoft.Maps.Polygon(vertices,{fillColor: color,strokeThickness:0});
		return polygon;
	}

	function generateDraggedSelectionQuad(latitude, longitude, width, height, color)
	{	
		// Create the locations
		var location1 = new Microsoft.Maps.Location(latitude,longitude);
		var location2 = new Microsoft.Maps.Location(latitude+width,longitude);
		var location3 = new Microsoft.Maps.Location(latitude+width,longitude+height);
		var location4 = new Microsoft.Maps.Location(latitude,longitude+height);

		// Create a polygon 
		var vertices = new Array(location1, location2, location3, location4, location1);
		var polygon = new Microsoft.Maps.Polygon(vertices,{fillColor: color,strokeThickness:0});
		return polygon;
	}
	
	
	
	function submitAlberoRequest(request,success,failed){
		$.ajax({
    		url: "albero.php",
			contentType: "application/json",
			type: "POST",
			data: JSON.stringify(request),
			async:   true,
			complete: function(data){
				success(data)
			},
			error: function(data){
				failed(data);
			}
		});
	}


/*
	$("#apply_patterns").click(function(){
		
		OpenMessageBox("Processing", "Please wait while your request is being processed...");
		
		$.ajax({
			//blah options
			url: "process_patterns.php",
			dataType: "json",
			type: "POST",
			data: JSON.stringify(setup),
			async:   true,
			complete: function(data){
				
				map.entities.remove(tilelayer);
				
				// Create the tile layer source
				tileSource = new Microsoft.Maps.TileSource({uriConstructor: '/albero.php?q={quadkey}&t=' + new Date().getTime()});
				// Construct the layer using the tile source
				tilelayer= new Microsoft.Maps.TileLayer({ mercator: tileSource, opacity: .7 });
				// Push the tile layer to the map
				
				map.entities.push(tilelayer);
				
				response = data.responseJSON[data.responseJSON.length-1];
				
				CloseMessageBox();
				
				if(response!="OK")
				{
					OpenMessageBox("Error", response);
				}
				
			}//end success
		});//end ajax
	
		
	});
*/


	function LoadLayer(map, range_index, threshold_index)
	{	
//		console.log(range_index);
//		console.log(threshold_index);

		var tilelayer = mapLayers[map.albero_id];
		
		try
        {
           
           if (typeof tilelayer !== 'undefined') map.entities.remove(tilelayer);

           if(threshold_index>=0)
           {
			   tileSource = new Microsoft.Maps.TileSource({uriConstructor: getTilePath});
           }
           else
           {
           	   tileSource = new Microsoft.Maps.TileSource({uriConstructor: getObservationTile});
           }

           // Construct the layer using the tile source
           tilelayer= new Microsoft.Maps.TileLayer({ mercator: tileSource, opacity: .7 });

           // Push the tile layer to the map
           map.entities.push(tilelayer);
           mapLayers[map.albero_id] = tilelayer;

           
        }
        catch(err)
        {
           alert( 'Error Message:' + err.message);
        }
        
         function getObservationTile(tile)
		 {
		 	if(threshold_index==-2) // OBSERVATIONS
			 	return 'albero.php?action=get_observation&z=' + tile.levelOfDetail + '&x=' + tile.x + '&y=' + tile.y + '&threshold_index=' + threshold_index + '&range_index=' + range_index+ "&" + new Date().getTime();

 		 	if(threshold_index==-3) // MSE
			 	return 'albero.php?action=get_mse&z=' + tile.levelOfDetail + '&x=' + tile.x + '&y=' + tile.y + '&range_index=' + range_index+ "&" + new Date().getTime();

 		 	if(threshold_index==-4) // NUMERICAL FORECAST
			 	return 'albero.php?action=get_num_forecast&z=' + tile.levelOfDetail + '&x=' + tile.x + '&y=' + tile.y + '&range_index=' + range_index+ "&" + new Date().getTime();

		 }

		 function getTilePath(tile)
		 {
			 return 'albero.php?action=get_forecast&z=' + tile.levelOfDetail + '&x=' + tile.x + '&y=' + tile.y + '&threshold_index=' + threshold_index + '&range_index=' + range_index + "&" + new Date().getTime();
		 }
	}

	function GetMap()
    {

    	// main map
    	map = SetupMap("mapDiv", "main_map");
    	Maps.push(map);
       
		// Attach your events in the map load callback:
		Microsoft.Maps.Events.addHandler(map, 'mousemove',onMouseMove );      

		// this events are to see if the user dragged or clicked
		Microsoft.Maps.Events.addHandler(map, 'mousedown',onMouseDown );      
		Microsoft.Maps.Events.addHandler(map, 'mouseup', onMouseUp );      
	
		// linked sub maps
		map_sub = [];
		for(var i=0; i<=2; i++){
    	    map_sub[i] = SetupMap("map-sub-"+(i+1), "sub_map_" + i);
    	    Maps.push(map_sub[i]);
    	}
       
       	// subscribe on mouse click event of the main map.
		onMouseClickEvents.push(alberoUpdateMiniBingMaps);
		 
	}

	// Creates the map and assigns entities and grids
	function SetupMap(map_div_id, albero_id){
		 // Initialize the map
        var map = new Microsoft.Maps.Map(document.getElementById(map_div_id),{credentials:"AhvmYWAXOIhP2nFNIZE8QoT8JmMVY0AIAW2SqfE_wz22SDhr1l0TmUdpFqA8fkDv", center:new Microsoft.Maps.Location(-38.5,-65), zoom:5, mapTypeId: Microsoft.Maps.MapTypeId.road }); 

        map.albero_id = albero_id;

    	// entity to hold the selection quads (this is to be able to set a different z-index)
		map.selectionQuadEntities = new Microsoft.Maps.EntityCollection();
		map.selectionQuadEntities.setOptions({visible:false,zIndex:-1});
		map.entities.push(map.selectionQuadEntities);
		/*
		dragSelectionQuadEntities = new Microsoft.Maps.EntityCollection();
		dragSelectionQuadEntities.setOptions({visible:true,zIndex:-2});
		map.entities.push(dragSelectionQuadEntities);
		*/
		map.clickedSelectionQuad = [];
		map.selectionQuad = [];

    	GenerateGridLines(map);

    	return map;
	}

	// update position of all 3 mini bingmaps
	function alberoUpdateMiniBingMaps(e, loc)
	{
		for(var i=0; i<=2; i++){
			map_sub[i].setView({zoom:5, center:loc});
		}
	}



    GetMap();
     //LoadLayer(map,0,0);



     function GenerateGridLines(map){
        var line;

        //Draw vertical grid lines (Longitudes)
        for(var x = -180; x < 180; x+=0.25)
        {
        	if(Math.floor(x)==x){
				line = new Microsoft.Maps.Polyline([new Microsoft.Maps.Location(-85, x), new Microsoft.Maps.Location(85, x)], {strokeColor:new Microsoft.Maps.Color(50, 100, 100, 100), strokeThickness:0.3});                               
				map.entities.push(line);
        	}
        }

        //Draw horizontal grid lines (Latitudes)
        //The wrap around effect in Bing Maps causes shapes that wide to sometimes loop around out of view. 
        //To prevent this we can break the lines into 4 segments. so that these grid lines aways stay in view
        for(var y = -85; y <= 85; y+=.25)
        {
        	if(Math.floor(y)==y){
                line = new Microsoft.Maps.Polyline([new Microsoft.Maps.Location(y, -180),new Microsoft.Maps.Location(y, -90)], {strokeColor:new Microsoft.Maps.Color(50, 100, 100, 100), strokeThickness:0.3});
                map.entities.push(line);

                line = new Microsoft.Maps.Polyline([new Microsoft.Maps.Location(y, -90),new Microsoft.Maps.Location(y, 0)], {strokeColor:new Microsoft.Maps.Color(50, 100, 100, 100), strokeThickness:0.3});
                map.entities.push(line);

                line = new Microsoft.Maps.Polyline([new Microsoft.Maps.Location(y, 0),new Microsoft.Maps.Location(y, 90)], {strokeColor:new Microsoft.Maps.Color(50, 100, 100, 100), strokeThickness:0.3});
                map.entities.push(line);

                line = new Microsoft.Maps.Polyline([new Microsoft.Maps.Location(y, 90),new Microsoft.Maps.Location(y, 180)], {strokeColor:new Microsoft.Maps.Color(50, 100, 100, 100), strokeThickness:0.3});
                map.entities.push(line);
        	}
        }
     }





	// First, checks if it isn't implemented yet.
	if (!String.prototype.format) {
	  String.prototype.format = function() {
	    var args = arguments;
	    return this.replace(/{(\d+)}/g, function(match, number) { 
	      return typeof args[number] != 'undefined'
	        ? args[number]
	        : match
	      ;
	    });
	  };
	}



</script>
