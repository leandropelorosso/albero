<div class="noselect" id="alberoConfigurationBox" title="Configuration">

<pre class="changing"></pre>


	<span class="ui-helper-hidden-accessible"><input type="text"/></span>
	<form>
		<ul>
			<li>Analog Range</li>
			<li><input readonly name="analog-range-from" class="date" value="12/07/2002"/></li>
			<li>to</li>
			<li><input readonly name="analog-range-to" class="date" value="23/07/2015"/> </li>
		</ul>
		
		<ul>
			<li>Valid Date</li>
			<li><input id="current-forecast-date" name="date" value="04/01/2013"/></li>
		</ul>

		<ul>
			<li>Analogs Amount</li>
			<li><input class="small" name="analogs-amount" value="55"/></li>
		</ul>

		<h3>Thresholds (mm)</h3> 
	
		<ul style="width:462px;" id="threshold_ranges"><!--
			<li>From</li>
			<li><input name="threshold-from" class="small" value="0"> </input></li>
			<li>to</li>
			<li><input name="threshold-to" class="small" value="80"> </input></li>
			<li>step</li>
			<li><input name="threshold-step" class="small" value="20"> </input></li>-->
		</ul>

		<h3>Accumulation (hs)</h3> 

		<div id="accordion" style="width: 480px;">

			<span>By Lead Time and Length</span>

			<div>

				<ul>
					<li>Accumulation Range</li>
					<li><select id="albero-toolbox-accumulation-range" name="accumulation-range" ><?=printAccumulationRanges(6)?></select></li>
				</ul>

				<ul>
					<li>LeadTime</li>
					<li><select id="ablero-toolbox-lead-time" name="leadtime-from">
						<option>----</option>
					</select></li>
				</ul>


			</div>
			
			<span>By Range</span>

			<div>
				<ul>
					<li style="width:472px; margin-bottom:18px;" id="accumulation-range-container"></li>
				</ul>
			</div>

		</div>
		 
 


<!--
		<h3>Variables Weight</h3> 
		
		<ul>
			<li>Precipitation</li>
			<li class="slider-container"><div class="slider variables-weight" id="slider-precipitation"></div></li>		
			<input for="slider-precipitation" name="weight-precipitation"  type="hidden"></input>
		</ul>

		<ul>
			<li>Temperature</li>
			<li class="slider-container"><div class="slider variables-weight" id="slider-temperature"></div></li>		
			<input for="slider-temperature" name="weight-temperature"  type="hidden"></input>
		</ul>
-->
		<div>
			<div style ="overflow: auto;">
				<input style="float:right; margin-bottom: 10px;" id="btn-calculate" type="button" value="Initialize"/>
			</div>
		</div>
		
	</form>
	
</div>

<? 

	// from 0 to 192 every 6 hours
	function printAccumulationRanges($selected_value){

		for($i=6; $i<=192; $i+=6)
		{
			?>
				<option <?if($i==$selected_value)echo('selected')?> value="<?=$i?>"><?=$i?> hs</option>
			<?
		}	

	}

	function print0to72hsOptions($selected_value)
	{
		for($i=0; $i<=72; $i+=6)
		{
			?>
				<option <?if($i==$selected_value)echo('selected')?> value="<?=$i?>"><?=$i?> hs</option>
			<?
		}
	}
?>