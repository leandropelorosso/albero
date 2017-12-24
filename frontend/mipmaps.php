<?
	function RenderSelectorTD($threshold_index, $range_index){
		echo("<td class=\"threshold_map_selector\" threshold_index=\"$threshold_index\" range_index=\"$range_index\" >
				<div class=\"threshold_selctor_image_wrapper\">
					<img class=\"ubermap_minimap\" width=\"60\" src=\"albero_images/forecast8_prob_map_" . $threshold_index . "_" . $range_index . ".png\" />
					<img class=\"ubermap_contour\" width=\"60\" src=\"albero_resources/contourmap.png\" />
				</div
			</td>
		");
	}

?>

<div class="noselect" id="alberoMipmapDialog" title="Übermap (04/01/2013)">

<div style="margin-right:100px; min-width: 115px; max-width: 420px; padding-bottom: 10px; margin-bottom:20px; overflow-x: auto;">
	<table>
		<thead>
			<tr id="ubermap-header" style="height:40px;">
			</tr>
		</thead>
		<tbody id="ubermap_body">
		<!--
			<tr>
				<td>80 mm</td>
				<?RenderSelectorTD(4,0);?>
				<?RenderSelectorTD(4,1);?>
				<?RenderSelectorTD(4,2);?>
			</tr>
			<tr>
				<td>60 mm</td>
				<?RenderSelectorTD(3,0);?>
				<?RenderSelectorTD(3,1);?>
				<?RenderSelectorTD(3,2);?>
			</tr>
			<tr>
				<td>40 mm</td>
				<?RenderSelectorTD(2,0);?>
				<?RenderSelectorTD(2,1);?>
				<?RenderSelectorTD(2,2);?>
			</tr>
			<tr>
				<td>20 mm</td>
				<?RenderSelectorTD(1,0);?>
				<?RenderSelectorTD(1,1);?>
				<?RenderSelectorTD(1,2);?>
			</tr>
			<tr>
				<td>0 mm</td>
				<?RenderSelectorTD(0,0);?>
				<?RenderSelectorTD(0,1);?>
				<?RenderSelectorTD(0,2);?>
			</tr>

			<tr>
				<td></td>
			</tr>
		-->
		<!--
			<tr class="no-delete">
				<td></td>
				<td class="threshold_map_selector label" threshold_index="-4" range_index="0" ><p>Forecast</p></td>
				<td class="threshold_map_selector label" threshold_index="-4" range_index="1" ><p>Forecast</p></td>
				<td class="threshold_map_selector label" threshold_index="-4" range_index="2" ><p>Forecast</p></td>
			</tr>

			<tr class="no-delete">
				<td></td>
				<td class="threshold_map_selector label" threshold_index="-2" range_index="0" ><p>Observed</p></td>
				<td class="threshold_map_selector label" threshold_index="-2" range_index="1" ><p>Observed</p></td>
				<td class="threshold_map_selector label" threshold_index="-2" range_index="2" ><p>Observed</p></td>
			</tr>

			<tr class="no-delete">
				<td></td>
				<td class="threshold_map_selector label" threshold_index="-3" range_index="0" ><p>MSE</p></td>
				<td class="threshold_map_selector label" threshold_index="-3" range_index="1" ><p>MSE</p></td>
				<td class="threshold_map_selector label" threshold_index="-3" range_index="2" ><p>MSE</p></td>
			</tr>
		-->
		</tbody>

	</table>

</div>

<div id="ubermap_scale" class="map-sub-wrapper">
	<div class="">
		<div class="big_scale">
			<img class="img_scale" width="11" height="152" src="albero_images/probabilistic_forecast_big_scale.png" />
		</div>
	</div>	
	<p style="font-size:11px;margin-left: -37px; text-align:center;">Probability (%)</p>
</div>






</div>	

<style>
	#alberoMipmapDialog table th
	{	

	}
</style>