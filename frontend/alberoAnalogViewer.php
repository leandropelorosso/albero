<div class="noselect" id="alberoAnalogViewer" title="Analog Viewer [24 hs - 48 hs)"  style="padding-top:26px;">

	<div >

		<div style="float:left; width: 970px; margin-bottom: 40px;">

			<!--<div style="margin-top:3px; margin-bottom:3px;">Forecast (total precipitation accumulation [kg m-2])</div>-->
			
			<!--<div id="selected_lat_lon" style="margin-top:20px; margin-bottom:30px;"></div>-->

			<div style="width: 100%;   position: relative;     overflow: visible;     padding-top: 10px;    height: 223px;">
				
				<!-- NUMERICAL FORECAST -->
				<div id="wrapper_numerical_forecast" class="big-box-wrapper">
					<div class="big_grid"></div> 
					<img width="152" height="152" id="analogViewerForecast" src=""/>
					<div class="big_scale forecast">
						<img class="img_forecast_scale_small img_scale" width="11" height="100%" />
						<div class="scale_indicators">
							<div class="max">MAX</div>
							<div class="min">MIN</div>
							<div class="total_max">MAX</div>
							<div class="total_min">MIN</div>
						</div>
					</div>
					<p class="image_label">Numerical Forecast</p>
				</div>


				<!-- ANALOG NUMERICAL FORECAST MEAN -->
				<div id="wrapper_numerical_forecast_mean" class="big-box-wrapper">
					<img width="152" height="152" id="analogViewerForecastMean"/>
					<div class="big_grid"></div> 
					<div class="big_scale forecast">
						<img class="img_forecast_scale_small img_scale" width="11" height="100%" />
						<div class="scale_indicators">
							<div class="max">MAX</div>
							<div class="min">MIN</div>
							<div class="total_max">MAX</div>
							<div class="total_min">MIN</div>
						</div>
					</div>
					<p class="image_label">Analog Forecasts Mean (1)</p>
				</div>

				<!-- ANALOG OBSERVATION MEAN -->
				<div id="wrapper_observation_mean" class="big-box-wrapper">
					<img width="152" height="152" id="analogViewerObservationMean"/>
					<div class="big_grid"></div> 
					<div class="big_scale observation">
						<img class="img_forecast_scale_small img_scale" width="11" height="100%" />
						<div class="scale_indicators">
							<div class="max">MAX</div>
							<div class="min">MIN</div>
							<div class="total_max">MAX</div>
							<div class="total_min">MIN</div>
						</div>
					</div>
					<p class="image_label">Analog Observations Mean (2)</p>
				</div>

				<!-- BIAS -->
				<div id="wrapper_bias" class="big-box-wrapper" style="width:190px;">
					<img width="152" height="152" id="analogViewerBias"/>
					<div class="big_grid"></div> 
					<div class="big_scale bias">
						<img class="img_bias_scale_small img_scale" width="11" height="152" src="albero_resources/bias_scale_small.png" />
						<div class="scale_indicators">
							<div class="max">MAX</div>
							<div class="min">MIN</div>
						</div>
					</div>
					<p class="image_label">Bias (1 - 2)</p>
				</div>



			</div>


			<!-- The analog forecast template -->
			<div id="analog_forecast_wrapper" class="analog_forecast_wrapper" style="display:none;">
				<div class="analog_forecast">
					<img/>
				</div>
				<div class="analog_observation">
					<img/>
					<div class="labels">
						<p class="date"></p>
						<p class="mse"></p>
					</div>
				</div>
			</div>
			<!---->

			<div>
				<span class="title-h2"> Analog Forecasts </span>
			</div>

			<div>
				<div class="analog-list-labels">
					<div style="height:53px;">Forecast</div>
					<div style="height:36px;">Observation</div>
					<div style="height:20px;">Date</div>
					<div style="height:20px;">MSE</div>
				</div>

				<!-- The div with all the analog forecasts -->
				<div style="overflow-x:scroll; width: 850px; float:left;">
					<div id="alberoAnalogViewerAnalogs" style="width:auto; white-space:nowrap;" >
					</div>
				</div>

			</div>


			<!--- stats -->
<!--		<div style="float:left; width:300px; margin-top:20px;" id="albero_analog_viewer_summary">
			  <table>
			  	<thead>
			  		<th>mm</th>
			  		<th>min</th>
			  		<th>max</th>
			  	</thead>
			  	<tbody>
			  	</tbody>
			  </table>
			</div>
-->
		</div>



<!---
		<div style="float:right; width:75px;" >
			<img style="border:none;" src="albero_resources/forecast_scale_3.png" />
		</div>
-->
	</div>

</div>


