
$(document).ready(function(){
	$( "#alberoAnalogViewer" ).dialog({width:'auto', maxWidth:'auto', autoOpen:false});

	// subscribe on mouse click event of the main map.
	onMouseClickEvents.push(alberoAnalogViewerClickOnMap);

	window.document.onkeydown = function (e)
	{
		if (!e) e = event;
		if (e.keyCode == 27)
		{
			$("#toggleShowHoverQuad").click();
		}
	}

});

// displays the max and min value on the scale (the color scale next to the regions)
// zero_at_height indicates, with a number between 0 and 1, where is located the zero in the scale.
function displayMaxMinInScale(wrapper_selector, min, max, local_min, local_max, zero_at_height){
	
	// get the heaight of the scale
	var wrapper = $(wrapper_selector);
	var height = wrapper.find(".big_scale").height();

	// get the max and min cursor elements
	var max_cursor = wrapper.find(".scale_indicators .max");
	var min_cursor = wrapper.find(".scale_indicators .min");

	var total_max_cursor = wrapper.find(".scale_indicators .total_max");
	var total_min_cursor = wrapper.find(".scale_indicators .total_min");

	truncated_local_max = local_max;
	truncated_local_min = local_min;


	// crop the max and min value
	if(truncated_local_max > max) truncated_local_max = max;
	if(truncated_local_max < min) truncated_local_min = min;

	if(min!=max){
		min_position = (height/(max-min))*(truncated_local_min-min);
		max_position = (height/(max-min))*(truncated_local_max-min);
	}else {
		min_position = height * zero_at_height;
		max_position = min_position;
	}

	max_cursor.text(parseFloat(local_max).toFixed(1));
	min_cursor.text(parseFloat(local_min).toFixed(1));

	total_max_cursor.text(parseFloat(max).toFixed(1));
	total_min_cursor.text(parseFloat(min).toFixed(1));

	max_cursor.css("bottom", max_position);
	min_cursor.css("bottom", min_position);
}


// open the viewer
function alberoDisplayAnalogViewer(lat, lon, range_index)
{

	// mask the analog viwer if its visible, otherwise the whole body
	if($("#alberoAnalogViewer").is(':visible')){
		$("#alberoAnalogViewer").mask("Loading...");	
	}else{
		$("#albero-wrapper").mask("Loading...");
	}

	

	$.ajax({
		  method: "GET",
		  url: "albero.php",
		  data: { action: "get_analog_viewer", lat: lat, lon:lon, range_index:range_index}
		})
		.done(function( data ) {
			
			$("#alberoAnalogViewer").unmask();
			$("#albero-wrapper").unmask();

			$("#mini-bingmaps").show();

			data = JSON.parse(data);
			var images = data[0].images;
		
			if(images.length ==0 ) return; // we probably clicked outside range
		
			// Display new Label

			var from = Albero.configuration["accumulation-range"]*range_index;
			var to = from + Albero.configuration["accumulation-range"];
			
			var center_lat = (Math.ceil(lat)-0.5);
			var center_lon = (Math.ceil(lon)-0.5);

			var title = "Analog Viewer [" + from + " hs - " + to + " hs) centered at (" + center_lat + ", " + center_lon + ")";

			$( "#alberoAnalogViewer" ).dialog({title: title, closeOnEscape: false}).dialog("open");
			
			$( "#alberoAnalogViewer .img_forecast_scale_small").attr("src", "albero_images/numerical_forecast_small_scale.png?" + new Date().getTime());

			$( "#alberoAnalogViewer .img_bias_scale_small").attr("src", "albero_images/bias_small_scale.png?" + new Date().getTime());


			// Display new Label
			var center_lat = (Math.ceil(lat)-0.5);
			var center_lon = (Math.ceil(lon)-0.5);
			var label = "RAR Analog viewer for the region of 3 x 3 centered at ("+center_lat+", "+center_lon+") forecasted in 04/01/2013 for the next [24-48) hs.";
			//$( "#alberoAnalogViewer #selected_lat_lon" ).html(label);

			$( "#alberoAnalogViewer #analogViewerForecast").attr("src", "albero_images/forecast_region.png?" + new Date().getTime());

			var max_value = parseFloat(data[0].max_value).toFixed(2);
			var min_value = parseFloat(data[0].min_value).toFixed(2);

			var max_observation_value = parseFloat(data[0].max_observation_value).toFixed(2);
			var min_observation_value = parseFloat(data[0].min_observation_value).toFixed(2);

			var max_forecast_value = parseFloat(data[0].max_forecast_value).toFixed(2);
			var min_forecast_value = parseFloat(data[0].min_forecast_value).toFixed(2);

			var max_bias_value = parseFloat(data[0].max_bias_value).toFixed(2);
			var min_bias_value = parseFloat(data[0].min_bias_value).toFixed(2);


			var max_mean_forecast_value = parseFloat(data[0].max_mean_forecast_value).toFixed(2);
			var min_mean_forecast_value = parseFloat(data[0].min_mean_forecast_value).toFixed(2);
			
			var max_mean_observation_value = parseFloat(data[0].max_mean_observation_value).toFixed(2);
			var min_mean_observation_value = parseFloat(data[0].min_mean_observation_value).toFixed(2);

			var region_max_forecast_value = parseFloat(data[0].region_max_forecast_value).toFixed(2);
			var region_min_forecast_value = parseFloat(data[0].region_min_forecast_value).toFixed(2);


			// STATS
		
			$("#albero_analog_viewer_summary tbody").empty();
			$("#albero_analog_viewer_summary tbody").append("<tr><td>forecast</td><td>" + region_min_forecast_value  + "</td><td>" + region_max_forecast_value + "</td></tr>");
			$("#albero_analog_viewer_summary tbody").append("<tr><td>analogs forecast</td><td>" + min_forecast_value  + "</td><td>" + max_forecast_value + "</td></tr>");
			$("#albero_analog_viewer_summary tbody").append("<tr><td>analogs forecast mean</td><td>" + min_mean_forecast_value  + "</td><td>" + max_mean_forecast_value + "</td></tr>");
			$("#albero_analog_viewer_summary tbody").append("<tr><td>analogs observation</td><td>" + min_observation_value  + "</td><td>" + max_observation_value + "</td></tr>");
			$("#albero_analog_viewer_summary tbody").append("<tr><td>analogs observation mean</td><td>" + min_mean_observation_value  + "</td><td>" + max_mean_observation_value + "</td></tr>");
			$("#albero_analog_viewer_summary tbody").append("<tr><td>bias</td><td>" + min_bias_value  + "</td><td>" + max_bias_value + "</td></tr>");
					
			// GRAPHS

			// Numerical Forecast Mean
			displayMaxMinInScale("#wrapper_numerical_forecast_mean", Albero.scale_min_forecast, Albero.scale_max_forecast, min_mean_forecast_value, max_mean_forecast_value,0);

			// Forecast Observation Mean
			displayMaxMinInScale("#wrapper_observation_mean", Albero.scale_min_forecast, Albero.scale_max_forecast, min_mean_observation_value, max_mean_observation_value,0);

			// Forecast Observation Mean
			displayMaxMinInScale("#wrapper_numerical_forecast", Albero.scale_min_forecast, Albero.scale_max_forecast, region_min_forecast_value, region_max_forecast_value,0);

			// Bias
			var max_abs_bias = Math.max(Math.abs(max_bias_value), Math.abs(min_bias_value));
			displayMaxMinInScale("#wrapper_bias", -max_abs_bias, max_abs_bias, min_bias_value, max_bias_value, 0.5);

			// ANALOGS

			// remove content of analogs holder
			$( "#alberoAnalogViewer #alberoAnalogViewerAnalogs").empty();

			var images = data[0].images;

			// Display the analogs, observations and means.
			for (var i = 0, len = images.length; i < len; i++) {
				analog = images[i];
				// clone analog template
				var a = $( "#alberoAnalogViewer #analog_forecast_wrapper").clone();
				$(a).removeAttr('id');
				$(a).addClass('analog-column');
				$(a).attr('mse', analog.mse);
				$(a).attr('date', analog.date);
				$(a).css('display', 'inline-block');
				$(a).find(".analog_forecast img").attr("src", "albero_images/" + analog.filename + "?" + new Date().getTime());
				$(a).find(".analog_observation img").attr("src", "albero_images/observation_" + analog.filename + "?" + new Date().getTime());
				$(a).find(".analog_observation p.date").html(analog.date.substring(0,4)+"/"+analog.date.substring(4,6)+"/"+analog.date.substring(6, 8));
				$(a).find(".analog_observation p.mse").html(parseFloat(analog.mse).toFixed(3));
				
				$( "#alberoAnalogViewer #alberoAnalogViewerAnalogs").append($(a));
			}

			// display means
			$("#alberoAnalogViewer #analogViewerForecastMean").attr("src","albero_images/analog_region_mean.png?" + new Date().getTime());
			$("#alberoAnalogViewer #analogViewerObservationMean").attr("src","albero_images/analog_observation_mean.png?" + new Date().getTime());
			$("#alberoAnalogViewer #analogViewerBias").attr("src","albero_images/analog_region_bias.png?" + new Date().getTime());

			sortAnalogs();
		});


}

// main mouse on click on particular location
function alberoAnalogViewerClickOnMap(e, loc)
{
	if(alberoToolbox.showHoverQuad)
	{
		alberoDisplayAnalogViewer(loc.latitude, loc.longitude, albero_selected_range);
	}
}

function sortAnalogs()
{
	$('.analog_forecast_wrapper.analog-column').sort(function (a, b) {
      var contentA = parseFloat($(a).attr('mse'));
      var contentB = parseFloat($(b).attr('mse'));
      return (contentA < contentB) ? -1 : (contentA > contentB) ? 1 : 0;
   }).appendTo($("#alberoAnalogViewer #alberoAnalogViewerAnalogs"));
}