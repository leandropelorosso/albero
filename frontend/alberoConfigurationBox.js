// Some configurations (which have to be the same on the C++, some careful while this is hardcoded)
Albero = {}
Albero.scale_min_forecast = 0; // the minimum value displayed on the scale
Albero.scale_max_forecast = 60; // the maximun value displayed on the scale
Albero.configuration = Array();


// auxiliary function to serialize form to JSON format
$.fn.serializeObject = function()
{
    var o = {};
    var a = this.serializeArray();
    $.each(a, function() {
        if (o[this.name] !== undefined) {
            if (!o[this.name].push) {
                o[this.name] = [o[this.name]];
            }
            o[this.name].push(this.value || '');
        } else {
            o[this.name] = this.value || '';
        }
    });
    return o;
};

$(document).ready(function(){
	$( "#alberoConfigurationBox" ).dialog({width:'auto', maxWidth:'auto', autoOpen: false });
	
	$('input.date').datepicker();

	// initialize sliders
	$('#alberoConfigurationBox .variables-weight').slider({max:100, min:0,  
	change: function(event, ui) {
		$('#alberoConfigurationBox input[for="'+$(this).attr('id')+'"]').val(ui.value);		
	}}).linkedSliders({ 
		total: 100,  // The total for all the linked sliders 
		policy: 'next' // Adjustment policy: 'next', 'prev', 'first', 'last', 'all' 
	}); 	


	// Initialize Server with Configuration 	
	$('#alberoConfigurationBox #btn-calculate').click(function(){

		var configuration = $('#alberoConfigurationBox form').serializeObject();
		configuration["action"] = "initialize";


		var date_parts = configuration["date"].split('/');
		configuration["date"] = date_parts[2] + date_parts[0] + date_parts[1];
		
		//configuration["threshold-ranges"] = rangeBar.val();
		configuration["threshold-ranges"] = $("#alberoConfigurationBox #threshold_ranges").limitslider("values");

		configuration["analogs-amount"] = parseInt(configuration["analogs-amount"]);
 		configuration["leadtime-from"] = parseInt(configuration["leadtime-from"]);
		configuration['accumulation-range'] = parseInt(configuration['accumulation-range']);
		configuration["accumulation-ranges"] = configuration["leadtime-from"] / configuration['accumulation-range'];


		// The accumulation ranges (fixed for now)
		/*var accumulation_ranges = Array( Array(0,24),
										 Array(24,48),
										 Array(48,72)
									   );
		configuration["accumulation-ranges"] = accumulation_ranges;*/
		//-----------------------


		// save the configuration as a global inside Albero{}		
		Albero.configuration = configuration;
		console.log(configuration);
		
		$("#albero-wrapper").mask("Loading...");


		$.ajax({
		  method: "GET",
		  type: "json",
		  url: "albero.php",
		  data: { action: "initialize", configuration}
		})
		.done(function( data ) {
			
			data = JSON.parse(data);
			Albero.stats = data;

			// unmask
			$("body").unmask();

			$("#alberoToolbox #li_toggleShowHoverQuad").show();
			$("#alberoToolbox #li_buttonShowMipmapDialog").show();
			
			// open ubermap
			OpenUberMap();

			// update mini bingmaps with the scale
			UpdateMiniBingmaps(0);
		});


	});
	

 $( "#accordion" ).accordion({autoHeight: true, heightStyle: "content"});

	rangeBar = new RangeBar
	({
  		  values: [[0,24],[24,48],[48,72]], // array of value pairs; each pair is the min and max of the range it creates (Ex: [[50, 100], [150, 200]])
		  readonly: false, // whether this bar is read-only
		  min: 0, // value at start of bar
		  max: 192, // value at end of bar
		  valueFormat: function(a) {return Math.round(a)}, // formats a value on the bar for output
		  valueParse: function(a) {return Math.round(a)}, // parses an output value for the bar
		  snap: 6, // clamps range ends to multiples of this value (in bar units)
		  minSize: 0, // smallest allowed range (in bar units)
		  maxRanges: 8, // maximum number of ranges allowed on the bar
		  bgMarks: {
		    count: 0, // number of value labels to write in the background of the bar
		    interval: Infinity, // provide instead of count to specify the space between labels
		    label: "kk" // string or function to write as the text of a label. functions are called with normalised     values.
		  },
		  label: function(a){ return "[" + (Math.round(a[0])) + "-" + (Math.round(a[1])) + ")"},
		  indicator: null, // pass a function(RangeBar, Indicator, Function?) Value to calculate where to put a     current indicator, calling the function whenever you want the position to be recalculated
		  allowDelete: true, // set to true to enable double-middle-click-to-delete
		  deleteTimeout: 5000, // maximum time in ms between middle clicks
		  vertical: false, // if true the rangebar is aligned vertically, and given the class elessar-vertical
		  bounds: null, // a function that provides an upper or lower bound when a range is being dragged. call     with the range that is being moved, should return an object with an upper or lower key
		  htmlLabel: false, // if true, range labels are written as html
		  allowSwap: true // swap ranges when dragging past
  });
  $('#alberoConfigurationBox #accumulation-range-container').append(rangeBar.$el);


    $('#threshold_ranges').limitslider({
        values:     [0, 20, 40, 60],
         max:80,
         label:      true,
    	showRanges: true,
   });


	// Since the accumulation range changed, lets reconfigure the lead time
	$("#albero-toolbox-accumulation-range").on("change", function(){
		// get selected accumulation range
		var accumulation_range = parseInt($(this).val());
		// update the leadtime selector
		updateLeadTimeSelector(accumulation_range);		
	})

	// select some default accumulation and leadtime
	$('#albero-toolbox-accumulation-range option[value="24"]').attr('selected', 'selected');
	updateLeadTimeSelector(24);		



	// check if the server is up and running
	checkServer();
	setInterval(checkServer, 1000*60);
});



function updateLeadTimeSelector(accumulation_range){

	// find lead time selector
	var lead_time_selector = $("#ablero-toolbox-lead-time");

	// remove all options
	lead_time_selector.find("option").remove();

	// now add the lead time options
	
	for(var lead_time = accumulation_range; lead_time <= 192; lead_time+=accumulation_range ){
		   lead_time_selector.append($("<option></option>")
     		.attr("value",lead_time)
     		.text(lead_time + " hs")); 
		   console.log(lead_time);
	}

}


function checkServer(){

		$.ajax({
		  method: "GET",
		  type: "json",
		  url: "albero.php",
		  data: { action: "ping" }
		})
		.done(function( data ) {

			$("#albero-server-status").removeClass("up").removeClass("down").removeClass("unknown");

			if(data=="pong"){
				$("#albero-server-status").addClass("up");
			}
			else{
				$("#albero-server-status").addClass("down");
			} 
		});
}
