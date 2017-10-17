<div class="noselect" id="alberoAnalogViewer" title="Analog Viewer [24 hs - 48 hs)">

	<div>

		<div style="float:left;width:560px;margin-bottom:20px;">

			<!--<div style="margin-top:3px; margin-bottom:3px;">Forecast (total precipitation accumulation [kg m-2])</div>-->
			
			<div id="selected_lat_lon" style="margin-top:13px; margin-bottom:18px;"></div>

			<div style="width:100%; height:160px; position:relative;">
				
				<!-- NUMERICAL FORECAST -->
				<div id="wrapper_numerical_forecast" style="float:left; width:250px;">
					<div class="big_grid"></div> 
					<img width="152" height="152" id="analogViewerForecast" src=""/>
					<div class="big_scale forecast">
						<img width="11" height="152" src="albero_resources/forecast_scale_3_small.png" />
						<div class="scale_indicators">
							<div class="max">MAX</div>
							<div class="min">MIN</div>
						</div>
					</div>
				</div>

				<div style="float:left; width:300px;" id="albero_analog_viewer_summary">
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

			</div>

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

			<div style="overflow-x:scroll;">
				<div id="alberoAnalogViewerAnalogs" style="width:auto; white-space:nowrap;" >
				</div>
			</div>

		</div>
		<div style="float:right; width:75px;" >
			<img style="border:none;" src="albero_resources/forecast_scale_3.png" />
		</div>

	</div>

	<div style="margin-top:10px;">

			<!-- ANALOG NUMERICAL FORECAST MEAN -->
			<div id="wrapper_numerical_forecast_mean" style="float:left; width:215px; position:relative;">
				<img width="152" height="152" id="analogViewerForecastMean"/>
				<div class="big_grid"></div> 
				<div class="big_scale forecast">
					<img width="11" height="100%" src="albero_resources/forecast_scale_3_small.png" />
					<div class="scale_indicators">
						<div class="max">MAX</div>
						<div class="min">MIN</div>
					</div>
				</div>
				<p class="image_label">Analog Forecasts Mean (1)</p>
			</div>

			<!-- ANALOG OBSERVATION MEAN -->
			<div id="wrapper_observation_mean" style="float:left; width:215px; position:relative;">
				<img width="152" height="152" id="analogViewerObservationMean"/>
				<div class="big_grid"></div> 
				<div class="big_scale observation">
					<img width="11" height="100%"  src="albero_resources/forecast_scale_3_small.png" />
					<div class="scale_indicators">
						<div class="max">MAX</div>
						<div class="min">MIN</div>
					</div>
				</div>
				<p class="image_label">Analog Observations Mean (2)</p>
			</div>

			<!-- BIAS -->
			<div id="wrapper_bias" style="float:left; width:215px; position:relative;">
				<img width="152" height="152" id="analogViewerBias"/>
				<div class="big_grid"></div> 
				<div class="big_scale bias">
					<img width="11" height="152" src="albero_resources/bias_scale_small.png" />
					<div class="scale_indicators">
						<div class="max">MAX</div>
						<div class="min">MIN</div>
					</div>
				</div>
				<p class="image_label">Bias (1 - 2)</p>
			</div>

	</div>

</div>


