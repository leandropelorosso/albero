var alberoToolbox = {showHoverQuad:false}

$(document).ready(function(){
	$( "#alberoToolbox" ).dialog({  position: { my: "left+2 top+45", at: "left top", of: "body" },  width:'auto', top:"10", left:"10", maxWidth:'auto', height:'120', autoOpen:true, closeOnEscape: false, open: function(event, ui) { $(".ui-dialog-titlebar-close", ui.dialog | ui).hide(); }});
	$( "#alberoToolbox .toggle-button").button();

	$('#alberoToolbox #toggleShowHoverQuad').click(function(){
		alberoToolbox.showHoverQuad = !alberoToolbox.showHoverQuad;	

		// display or hide the selector
		$.each(Maps, function( i, m ) {
			m.selectionQuadEntities.clear();
		});
	});

	$('#alberoToolbox #buttonShowConfigurationBox').click(function(){
		$('#alberoConfigurationBox').dialog('open');	
	});

	$('#alberoToolbox #buttonShowMipmapDialog').click(function(){
		OpenUberMap();
	});
	
/*
	$("#color_scale_selector").on("change", function(){
		var filename = $(this).val();

	$.ajax({
		  method: "GET",
		  url: "/albero.php",
		  data: { action: "set_color", file: filename }
		})
		.done(function( data ) {
		});
	});
*/

});


