// the range we will use for tasks where is not implicit. (Retrieving tiles, for instance)
albero_selected_range = 0;

// Selects an accumulation range as active
function activate_range(range_index){
	$('#alberoMipmapDialog .ubermap-column-header').removeClass("active");
	$('#alberoMipmapDialog .ubermap-column-header[range-index="'+range_index+'"]').addClass("active");
	albero_selected_range = range_index;
	LoadLayer(map_sub[0], range_index, -4);
	LoadLayer(map_sub[1], range_index, -2);
	LoadLayer(map_sub[2], range_index, -3);
}


/// Opens the UberMap and render its content accordingly.
function OpenUberMap(){
	
	$('#alberoMipmapDialog').dialog({ position: { my: "right-200 top+100", at: "right top", of: "#mapDiv" }, width:'auto', maxWidth:'auto'}).dialog('open');

			/*
			<tr>
				<td>80 mm</td>
				<?RenderSelectorTD(4,0);?>
				<?RenderSelectorTD(4,1);?>
				<?RenderSelectorTD(4,2);?>
			</tr>
			*/

	// console.log(Albero);

		$("#alberoMipmapDialog #ubermap-header th").remove();
		$("#alberoMipmapDialog #ubermap_body tr").remove();

		// Display HEADERS
		var accumulation_range = Albero.configuration['accumulation-range'];
		$("#alberoMipmapDialog #ubermap-header").append($("<th></th>"));
		for(var accumulation_range_index = 0; accumulation_range_index < Albero.configuration["accumulation-ranges"]; accumulation_range_index++){
			var from = accumulation_range * accumulation_range_index; 
			var to = from + accumulation_range;
			var th = $("<th class='ubermap-column-header' range-index='"+accumulation_range_index+"'><span>["+ from + " hs - " + to + " hs)</span></th>");
			$("#alberoMipmapDialog #ubermap-header").append(th);
		}

		// Display MINIMAPS
		// Iterate the threshold range
		var threshold_index = 0;
		Albero.configuration["threshold-ranges"].forEach(function(threshold) {

			var tr = $("<tr></tr>");
			var td = $("<td>&ge; " + threshold +" mm</td>");
			var e = tr.append(td);

			// For each accumulation range
			for(var accumulation_range_index = 0; accumulation_range_index < Albero.configuration["accumulation-ranges"]; accumulation_range_index++){
				var e = tr.append(RenderSelectorTD(threshold_index, accumulation_range_index));
				$("#alberoMipmapDialog #ubermap_body").prepend(e);
			}
			threshold_index++;
	    });


		// DISPLAY OTHER VISUALIZATION OPTIONS FOR THE COLUMN

		// FORECAST
		var tr = $("<tr><td class='small'></td></tr>");
		for(var accumulation_range_index = 0; accumulation_range_index < Albero.configuration["accumulation-ranges"]; accumulation_range_index++){
				var td = $("<td class='threshold_map_selector label' threshold_index='-4' range_index='"+accumulation_range_index+"'><p>Forecast</p></td>")
				tr.append(td);
		}
		$("#alberoMipmapDialog #ubermap_body").append(tr);


		// OBSERVED
		var tr = $("<tr><td class='small'></td></tr>");
		for(var accumulation_range_index = 0; accumulation_range_index < Albero.configuration["accumulation-ranges"]; accumulation_range_index++){
				var td = $("<td class='threshold_map_selector label' threshold_index='-2' range_index='"+accumulation_range_index+"'><p>Observed</p></td>")
				tr.append(td);
		}
		$("#alberoMipmapDialog #ubermap_body").append(tr);


		// MSE
		var tr = $("<tr><td class='small'></td></tr>");
		for(var accumulation_range_index = 0; accumulation_range_index < Albero.configuration["accumulation-ranges"]; accumulation_range_index++){
				var td = $("<td class='threshold_map_selector label' threshold_index='-3' range_index='"+accumulation_range_index+"'><p>MSE</p></td>")
				tr.append(td);
		}
		$("#alberoMipmapDialog #ubermap_body").append(tr);



	
}


// Renders a mipmap selector for the Ubermap
function RenderSelectorTD(threshold_index, range_index){

	var url = "albero_images/forecast8_prob_map_" + threshold_index + "_" + range_index + ".png?time="+Date.now();

	var e = " \
	<td class = 'threshold_map_selector minimap' threshold_index='{0}' range_index='{1}' >			\
		<div class='threshold_selctor_image_wrapper'>								\
			<img class='ubermap_minimap' width='90' height='151' src='{2}' />				\
			<img class='ubermap_contour' width='90' height='151' src='albero_resources/contourmap.png' />		\
		</div>			\
	</td>	\
	".format(threshold_index,range_index,url);

	return e;

}








$(document).ready(function(){

	// select accumulation range
	activate_range(0);
	
	$( "#alberoMipmapDialog" ).dialog({autoOpen:false});

	$("#alberoMipmapDialog").on("click",".threshold_map_selector", function(){
	
		// Add the active class to the minimap option selected
		$("#alberoMipmapDialog .threshold_map_selector").removeClass('active');
		$(this).addClass('active');

		// Also add the active class to the first TD of the row.
		$("#alberoMipmapDialog").find("tr td:first-child").removeClass("active");
		$(this).closest("tr").find("td:first-child").addClass("active");


		var threshold_index = $(this).attr("threshold_index");
		var range_index = $(this).attr("range_index");
	
		LoadLayer(map, range_index, threshold_index);
	
		activate_range(range_index);
		var stats  = Albero.stats[range_index];

		threshold_index = parseInt(threshold_index);


		$("#albero_floating_scale .scale_indicators .max").show();
		$("#albero_floating_scale .scale_indicators .min").show();


		if(threshold_index>=0) // we want to display probabilities
		{
			$("#albero_floating_scale .scale_indicators .max").hide();  // we dont have this information yet
			$("#albero_floating_scale .scale_indicators .min").hide();



			$("#albero_floating_scale img").attr("src", "albero_images/probabilistic_forecast_big_scale.png")
			$("#albero_floating_scale p").text("probability (%)");
			displayMaxMinInScale("#albero_floating_scale", 0, 100, stats.min_probabilistic_forecast, stats.max_probabilistic_forecast,0);
		}
		else
		{

/*
max_mse
max_numerical_forecast
max_observation
max_probabilistic_forecast
min_mse
min_numerical_forecast
min_observation
min_probabilistic_forecast
*/

			switch(threshold_index)
			{


				case -2: // OBSERVATION
					$("#albero_floating_scale img").attr("src", "albero_images/numerical_forecast_big_scale.png?time="+Date.now());
					$("#albero_floating_scale p").text("observed precipitation accumulation (mm)");
					displayMaxMinInScale("#albero_floating_scale", Albero.scale_min_forecast, Albero.scale_max_forecast, stats.min_observation, stats.max_observation,0);

				break;
				case -3: // MSE
					$("#albero_floating_scale img").attr("src", "albero_images/mse_big_scale.png?time="+Date.now());
					$("#albero_floating_scale p").text("mean squared error");
					displayMaxMinInScale("#albero_floating_scale", 0, 20, stats.min_mse, stats.max_mse, 0);
				break;
				case -4: // NUMERICAL FORECAST
					$("#albero_floating_scale img").attr("src", "albero_images/numerical_forecast_big_scale.png?time="+Date.now());
					$("#albero_floating_scale p").text("numerical forecast (precipitation accumulation) (mm)");
					displayMaxMinInScale("#albero_floating_scale", Albero.scale_min_forecast, Albero.scale_max_forecast, stats.min_numerical_forecast, stats.max_numerical_forecast,0);

				break;


			}


		}

		$("#albero_floating_scale").show();


	})

});


function UpdateMiniBingmaps(range_index){

	var stats  = Albero.stats[range_index];

	$("#map-sub-1-wrapper .albero_floating_scale img").attr("src", "albero_images/numerical_forecast_big_scale.png?time="+Date.now());
	displayMaxMinInScale("#map-sub-1-wrapper .albero_floating_scale", Albero.scale_min_forecast, Albero.scale_max_forecast, stats.min_numerical_forecast, stats.max_numerical_forecast,0);

	$("#map-sub-2-wrapper .albero_floating_scale img").attr("src", "albero_images/numerical_forecast_big_scale.png?time="+Date.now());
	displayMaxMinInScale("#map-sub-2-wrapper .albero_floating_scale", Albero.scale_min_forecast, Albero.scale_max_forecast, stats.min_observation, stats.max_observation,0);

	$("#map-sub-3-wrapper .albero_floating_scale img").attr("src", "albero_images/mse_big_scale.png?time="+Date.now());
	displayMaxMinInScale("#map-sub-3-wrapper .albero_floating_scale", 0, 20, stats.min_mse, stats.max_mse, 0);

}

	
