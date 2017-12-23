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


	// Return accumulation ranges from configuration, as [{from(int), to(int)}]
	function getAccumulationRangeConfiguration(){
		
		// get the type of range active (which accordion is open)
		var range_type = $("#range-type-selector .ui-accordion-header-active").attr("type");
		
		// the ranges we will return
		var ranges = [];

		// if it is by lead time and length
		if(range_type == "by-lead-time-and-length"){

			// Create ranges from the duration and lead time		
			var accumulation_range = parseInt($("#albero-toolbox-accumulation-range").val());
			var lead_time = parseInt($("#ablero-toolbox-lead-time").val());
			
			for(var i=0; i<lead_time; i+=accumulation_range){
				ranges.push([i/6, (i+accumulation_range)/6]);
			}			

		}else{ // if it is by range (by-range)

			$("#accumulation-ranges .acculumation-range-selector").each(function(){
				var from = $(this).find(".accumulation-from").val();
				var to = $(this).find(".accumulation-to").val();
				if(from && to){
					ranges.push([parseInt(from)/6, parseInt(to)/6]);
				}
			})
		}

		// return ranges
		return (ranges);
	}


	// Initialize Server with Configuration 	
	$('#alberoConfigurationBox #btn-calculate').click(function(){

		var configuration = $('#alberoConfigurationBox form').serializeObject();
		configuration["action"] = "initialize";


		var date_parts = configuration["date"].split('/');
		configuration["date"] = date_parts[2] + date_parts[0] + date_parts[1];
		
		//configuration["threshold-ranges"] = rangeBar.val();
		configuration["threshold-ranges"] = $("#alberoConfigurationBox #threshold_ranges").limitslider("values");

		configuration["analogs-amount"] = parseInt(configuration["analogs-amount"]);

		configuration["accumulation-ranges"] = getAccumulationRangeConfiguration();

		configuration["analogs-amount"] = parseInt(configuration["analogs-amount"]);

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
	

	// Initialize accordion
 	$( "#range-type-selector" ).accordion({autoHeight: true, heightStyle: "content"});


 	// --------------------- ACCUMULATION BY VARIABLE RANGES

	// Populates the FROM range selector with values from 0 to 192
	function populateRangeSelectorOptions(selector, from_value, to_value, empty){
		// remove all options
		selector.find("option").remove();
		// default (none)
			if(empty) selector.append($("<option></option>"));

		// now add the FROM time options				
		for(var value = from_value; value <= to_value; value +=6 ){
			   selector.append($("<option></option>")
		 		.attr("value",value)
		 		.text(value + " hs")); 		  
		}	
	}

	// Populate Range FROM dropdown with values from to 192
	$("#accumulation-ranges .accumulation-from").each(function() {
		populateRangeSelectorOptions($(this), 0, 192-6, true);
	});

	// Since the accumulation range changed, lets reconfigure the lead time
	$("#accumulation-ranges .accumulation-from").on("change", function(){
		// get selected accumulation range
		var value = parseInt($(this).val()) + 6;
		// update the TO selector
		var to = $(this).closest('.acculumation-range-selector').find('.accumulation-to');				
		populateRangeSelectorOptions($(to), value, 192, false);
	});

	// Set some default values
	$("#accumulation-ranges > div:nth-child(1) > select.accumulation-from").val(0).change()
	$("#accumulation-ranges > div:nth-child(1) > select.accumulation-to").val(24);

	// ---------------------------------------


/*
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
  });*/


/*
	var rangeBar = new RangeBar({
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
		label: function(a){ return "" },
		indicator: null, // pass a function(RangeBar, Indicator, Function?) Value to calculate where to put a     current indicator, calling the function whenever you want the position to be recalculated
		allowDelete: true, // set to true to enable double-middle-click-to-delete
		deleteTimeout: 5000, // maximum time in ms between middle clicks
		vertical: false, // if true the rangebar is aligned vertically, and given the class elessar-vertical
		bounds: null, // a function that provides an upper or lower bound when a range is being dragged. call     with the range that is being moved, should return an object with an upper or lower key
		htmlLabel: false, // if true, range labels are written as html
		allowSwap: true, // swap ranges when dragging past
	  	barClass: 'progress',
	  	rangeClass: 'bar'
	});
*/

//  $('#alberoConfigurationBox #accumulation-range-container').append(rangeBar.$el);
/*
$('.acculumation-range-input').jRange({
    from: 0,
    to: 192,
    step: 6,
    scale: [0,6,12,18,24,30,36,42,48,54,60,66,72,78,84,90,96,102,108,114,120,126,130,136,142,148,154,160,168,174,180,186,192],
    format: '%s',
    width: 1200,
    showLabels: true,
    isRange : true,
    snap : true,
});
*/	



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
