#include "Albero2.h"

#include <stdio.h>
#include <netcdf.h>
#include "iostream"
#include "helper.h"
#include <chrono>
#include <assert.h>  
#include <fstream>
#include <string>     
#include <iomanip>
#include <math.h>
#include "ObservationReader.h"
#include "Color.h"
#include "PCT.h"
#include "ForecastReader.h"
#include <string.h>
#include <thread>


extern string albero_images_path;
extern string reforecast_file_path;
extern int times_in_range;


/* Handle errors by printing an error message and exiting with a
* non-zero status. */
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); system("pause"); return(0);}


#pragma region Netcdf Indexes Functions
	#define CPU_HISTORIC_FORECAST_INDEX(year,time,fhour, lat, lon) (year * (NTIME_WINDOW * NFHOUR * NLAT * NLON) + (time * NFHOUR * NLAT * NLON) + (fhour * NLAT * NLON) + (lat * NLON) + lon)
	#define CPU_ANALOG_INDEX(range_index, lat, lon)  ((((range_index) * NLAT + lat) * NLON + lon) * N_ANALOGS_PER_LAT_LON)
	#define CPU_SINGLE_FORECAST_MEAN_INDEX(fhour, lat, lon) (fhour * (NLAT * NLON) + (lat * NLON) + lon)
	#define CPU_SINGLE_FORECAST_INDEX(fhour,ensamble, lat, lon) (ensamble * NFHOUR * NLAT*NLON) + (fhour * NLAT * NLON) + (lat * NLON) + lon
	#define CPU_PROBABILITY_POINT(x,y,z) (x * probability_map_height * analog_dimension * (N_ANALOGS_PER_LAT_LON+1) ) + (y * analog_dimension * (N_ANALOGS_PER_LAT_LON+1)) + (z* (N_ANALOGS_PER_LAT_LON+1))
#pragma endregion

#define OBSERVATIONS_PER_PANEL 5	// the amount of observations per panel. Remember that the analog region is conformed by a square
									// of ANALOG_DIMENSION * ANALOG_DIMENSION panels. 

Albero2::Albero2()
{
}


Albero2::~Albero2()
{
	

	// Lets clean the structures if they were used
	if (this->forecasts != NULL) delete(forecasts);
	
	if (this->probability_map != NULL){
		for (int i = 0; i < this->threshold_ranges.size() * nAccumulationRanges; i++){
			delete(probability_map[i]);
		}
		delete(probability_map);
	}

	if (this->mean_square_error_map != NULL) {
		for (int i = 0; i < this->nAccumulationRanges; i++){
			delete(mean_square_error_map[i]);
		}
		delete(mean_square_error_map);
	}

	if (this->current_forecast_by_range != NULL){
		for (int i = 0; i<this->nAccumulationRanges; i++){
			delete(current_forecast_by_range[i]);
		}
		delete(current_forecast_by_range);
	}

	if (this->historic_forecast_by_range != NULL) {
		for (int i = 0; i < this->nAccumulationRanges; i++){
			delete(historic_forecast_by_range[i]);
		}
		delete(historic_forecast_by_range);
	}

	if (this->analogs != NULL) delete(analogs);
	if (this->stats != NULL) delete(stats);

}


// Reads the historic forecasts from the forecasts netcdf.
int Albero2::ReadHistoricForecasts(int current_date){

	if (forecasts == NULL) {
		forecasts = new ForecastReader();
		forecasts->Initialize(reforecast_file_path);
	}

	// TODO ! IMPORTANT: IT SHOULD PROCESS THE DAYS AFTER THE DATE TOO, NOT ONLY THE ONES BEFORE
	// THIS IS PRENTENDING WE HAVE INFORMATION ONLY UP TO THE DESIRED DATE
	forecasts->NTIME = forecasts->GetDateIndex(current_date) +  1;
	forecasts->NYEARS = (int)((float)forecasts->NTIME / 365.25f);

	NFHOUR = (int)forecasts->NFHOUR;
	NLAT = (int)forecasts->NLAT;
	NLON = (int)forecasts->NLON;
	NTIME = (int)forecasts->NTIME;
	NYEARS = (int)forecasts->NYEARS;
	
	size_t const NDIMS = 4;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LEEMOS LA INFORMACION 
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	size_t dev_analogs_size = NLAT * NLON * this->nAccumulationRanges * N_ANALOGS_PER_LAT_LON;
	analogs = new AnalogIndex[dev_analogs_size];

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LEEMOS EL CURRENT FORECAST
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// What are we going to read from the netcdf.
	size_t start[NDIMS], count[NDIMS];

	count[0] = 1; // time
	count[1] = NFHOUR; // hour
	count[2] = NLAT; // lat
	count[3] = NLON; // lon
	start[0] = forecasts->GetDateIndex(current_date);
	start[1] = 0;
	start[2] = 0;
	start[3] = 0;

	cout << "Forecast for Date:" << forecasts->forecast_days[start[0]] << endl;

	// Let's read the current forecast (the one on the NTIMEth day)
	int retval;
	float *current_forecast = new float[NFHOUR * NLAT * NLON];
	if ((retval = nc_get_vara_float(forecasts->ncid, forecasts->precipitation_varid, start, count, current_forecast)))
		ERR(retval);

	cout << "1" << endl;

	/*******************************************************************************/
	// HERE WE ACCUMULATE THE VALUES OF THE RANGE FOR THE CURRENT FORECAST
	/*******************************************************************************/


	current_forecast_by_range = new float*[this->nAccumulationRanges];

	// Now we can can accumulate the information for the range.
	for (int range = 0; range < this->nAccumulationRanges; range++){

		float min_value = MAX_FLOAT;
		float max_value = 0;

		// initialize the current forecast for the range
		current_forecast_by_range[range] = new float[NLAT * NLON * this->nAccumulationRanges];
		memset(current_forecast_by_range[range], 0, sizeof(float) * NLAT * NLON);

		int hour_start = range * times_in_range + 1; // (remember, Total precipitation (kg m�2, i.e., mm) in last 6�h period (00, 06, 12, 18 UTC) or in last 3�h period(03, 09, 15, 21 UTC)[Y])
		int hour_end = hour_start + times_in_range;

		for (int hour = hour_start; hour < hour_end; hour ++ )
		{
			// get the start index for that hour in the forecast from the netcd
			int current_original_index = (hour * NLAT * NLON);
			

			for (int lat = 0; lat < NLAT; lat++){
				for (int lon = 0; lon < NLON; lon++){

					int i = lat*NLON + lon;

					float value = current_forecast[current_original_index + i];

					/*
					value = 0;
					if (this->forecasts->lats[lat] == -35 && this->forecasts->lons[lon] == -59)	value = 10;
					if (this->forecasts->lats[lat] == -34 && this->forecasts->lons[lon] == -59)	value = 10;
					*/

					current_forecast_by_range[range][i] += value;

					min_value = fminf(min_value, current_forecast_by_range[range][i]);
					max_value = fmaxf(max_value, current_forecast_by_range[range][i]);
				}
			}
			

		}

		// stats
		stats->min_numerical_forecast[range] = min_value;
		stats->max_numerical_forecast[range] = max_value;
	}


	delete(current_forecast);

	cout << "2" << endl;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// LEEMOS LOS HISTORIC FORECAST
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	count[0] = NTIME_WINDOW;
	//float *historic_forecast = new float[NYEARS * NTIME_WINDOW * NFHOUR * NLAT * NLON];
	float *historic_forecast = new float[NYEARS * NTIME_WINDOW * NFHOUR * NLAT * NLON];



	for (int year = 0; year < NYEARS; year++) // we want 90 days of each year.
	{
		// read the 90 days window for the year.
		start[0] = (size_t)(NTIME - (NYEARS - year)*365.25f - (NTIME_WINDOW/2));

		cout << "Segment Date " << forecasts->forecast_days[start[0]] << " - " << forecasts->forecast_days[start[0]+NTIME_WINDOW] << endl;
		
		if ((retval = nc_get_vara_float(forecasts->ncid, forecasts->precipitation_varid, start, count, &historic_forecast[year*NTIME_WINDOW*NFHOUR*NLAT*NLON])))
			ERR(retval);
	}

	/*******************************************************************************/
	// HERE WE ACCUMULATE THE VALUES OF THE RANGE FOR THE ANALOGS
	/*******************************************************************************/
	
	cout << "3" << endl;

	historic_forecast_by_range = new float*[this->nAccumulationRanges];

	cout << "4" << endl;

	// Now we can can accumulate the information for the range.
	for (int range = 0; range < this->nAccumulationRanges; range++){
		
		historic_forecast_by_range[range] = new float[NYEARS * NTIME_WINDOW * NLAT * NLON];
		memset(historic_forecast_by_range[range], 0, sizeof(float) * NYEARS * NTIME_WINDOW * NLAT * NLON);

		// Iterate the days
		
		for (int year = 0; year < NYEARS; year++)
		{
			for (int day = 0; day < NTIME_WINDOW; day++)
			{
				int hour_start = range * times_in_range + 1; // (remember, Total precipitation (kg m�2, i.e., mm) in last 6�h period (00, 06, 12, 18 UTC) or in last 3�h period(03, 09, 15, 21 UTC)[Y])
				int hour_end = hour_start + times_in_range;
				
				for (int hour = hour_start; hour < hour_end; hour ++)
				{
					// get the start index for that hour in the forecast from the netcd
					int current_original_index = CPU_HISTORIC_FORECAST_INDEX(year, day, hour, 0, 0);
					// get the save index, the start of the lat lon of the day
					int save_index = (year * NTIME_WINDOW * NLAT * NLON) + day * NLAT * NLON ;

					
					for (int lat = 0; lat < NLAT; lat++){
						for (int lon = 0; lon < NLON; lon++){

							int i = lat*NLON + lon;

							float value = historic_forecast[current_original_index + i];

							/*
							value = 0;
							if (year == 0 && day == 0){
								if (this->forecasts->lats[lat] == -35 && this->forecasts->lons[lon] == -59) value = 10;
								if (this->forecasts->lats[lat] == -34 && this->forecasts->lons[lon] == -59)	value = 10;
							}
							*/

							historic_forecast_by_range[range][save_index + i] += value;
						}
					}
				}

			}
		}

	}

	delete(historic_forecast);


	return 0;
}


// Calculates the Analogss
int Albero2::CalculateAnalogs(){

	//This line should be all, what you need to add to your code.
	static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 required");


	float max_mse = MIN_FLOAT;
	float min_mse = MAX_FLOAT;

	// Foreach accumulation Range
	for (int accumulation_range = 0; accumulation_range< this->nAccumulationRanges; accumulation_range++){

		// for each latitud, longitud and fhour...
		for (int lat = 0; lat < NLAT; lat++){

			for (int lon = 0; lon < NLON; lon++){

				int c_i = lat*NLON + lon;

				// now, for this (latitude : longitude : fhour), retrieve the index to the analogs.
				// the (latitude:longitude) corresponds to the upper left corner of the center block.

				size_t index = CPU_ANALOG_INDEX(accumulation_range, lat, lon);
				AnalogIndex *local_analogs = &analogs[index];

				// and collect the max analog so far (which is any)

				AnalogIndex *maxAnalog = &local_analogs[0];


				// now, for the current (lat,lon) point, we'll iterate the surounding points, for evey year and window time

				for (int year = 0; year < NYEARS; year++){

					for (int time = 0; time < NTIME_WINDOW; time++){

						int date = forecasts->forecast_days[(int)(NTIME - (NYEARS - year)*365.25f - 45) + time];

						// ... and calculate the MSE of the region suroinding the center block.

						float mse = 0;  // mse of the block
						int cells_in_block = 0;; // amount of cells on the block


						
						/*****************************************************
						/***** DEBUG **************************************/
						
						float point_lat = forecasts->lats[lat];
						float point_lon = forecasts->lons[lon];

						int start_x = lon - ((int)ANALOG_DIMENSION / 2);
						int end_x = lon + ((int)ANALOG_DIMENSION / 2) + 1;

						int start_y = lat - ((int)ANALOG_DIMENSION / 2) - 1;
						int end_y = lat + ((int)ANALOG_DIMENSION / 2);

					/*	if (point_lat == -34 && point_lon == -59){
							cout << "found" << endl;
						}
						*/
						/******************************************************/

					
						// iterate X as the longitud of the surounding points.
						for (int x = lon - ((int)ANALOG_DIMENSION / 2); x <= lon + ((int)ANALOG_DIMENSION / 2) + 1; x++){ // lon

							if (x < 0) continue;
							if (x >= NLON) continue;

							// iterate Y as the latitude of the surounding points.
							for (int y = lat - ((int)ANALOG_DIMENSION / 2)-1; y <= lat + ((int)ANALOG_DIMENSION / 2); y++){ // lat

								if (y < 0) continue;
								if (y >= NLAT) continue;

								//---------------------------------------------------------------------------
							
								float current_mean = current_forecast_by_range[accumulation_range][y*NLON + x];

								float historic_mean = this->historic_forecast_by_range[accumulation_range][year * NTIME_WINDOW*NLAT*NLON + (time * NLAT * NLON) + (y * NLON) + x];

								//---------------------------------------------------------------------------

								mse += ((current_mean - historic_mean) * (current_mean - historic_mean));
								cells_in_block++;
								
							}
						}

						if (cells_in_block > 0){
							mse = mse / cells_in_block;
							mse = sqrtf(mse);

							// find the analog with highest mse
							for (int i = 0; i < N_ANALOGS_PER_LAT_LON; i++){
								if (local_analogs[i].mse == MAX_FLOAT){
									maxAnalog = &local_analogs[i];
									break;
								}

								if (local_analogs[i].mse > maxAnalog->mse){
									maxAnalog = &local_analogs[i];
								}
							}

							// if the analog with highest mse is higher than the mse of the parameter, update.
							if (maxAnalog->mse > mse){
								maxAnalog->mse = mse;
								maxAnalog->time = time;
								maxAnalog->year = year;
								maxAnalog->lat = lat;
								maxAnalog->lon = lon;
								maxAnalog->date = date;

								/*if (point_lat == -34 && point_lon == -59){
									std::cout << "date: " <<  date << endl;
								}
								*/



							}
						}
					}
				}
			}
		}
	}

	// create the mean square error map for the accumulation ranges
	this->mean_square_error_map = new float*[this->nAccumulationRanges];

	// Iterate the accumulation ranges
	for (int accumulation_range = 0; accumulation_range < this->nAccumulationRanges; accumulation_range++){

		max_mse = MIN_FLOAT;
		min_mse = MAX_FLOAT;

		this->mean_square_error_map[accumulation_range] = new float[NLAT * NLON];

		for (int lat = 0; lat < NLAT; lat++){ // for each cell on the map
			for (int lon = 0; lon < NLON; lon++){
				size_t index = CPU_ANALOG_INDEX(accumulation_range, lat, lon);
				AnalogIndex *local_analogs = &analogs[index];
				float sum_mse = 0;
				for (int i = 0; i < N_ANALOGS_PER_LAT_LON; i++){ // Iterate the analogs and sum the mse.
					sum_mse += local_analogs[i].mse;
				}
				float avg_mse = sum_mse / N_ANALOGS_PER_LAT_LON;
				this->mean_square_error_map[accumulation_range][lat*NLON + lon] = avg_mse; // calculate the mean and store
				
				max_mse = fmaxf(max_mse, avg_mse);
				min_mse = fminf(min_mse, avg_mse);
				
//				cout << lat << " " << lon << " " << this->mean_square_error_map[fhour][(NLAT-lat-1)*NLON + lon] << endl;
			}
		}

		// stats
		this->stats->max_mse[accumulation_range] = max_mse;
		this->stats->min_mse[accumulation_range] = min_mse;

	}
	cout << "Min MSE: " << min_mse << endl;
	cout << "Max MSE: " << max_mse << endl;
	return 0;
}


RegionForecastCollection* Albero2::GetCurrentForecast(float lat, float lon, int accumulation_range)
{
	// get the lat and lon index for the upper left corner of the desired region.

	// get the lat lon index of the specified coordinate
	float deltaLon = (forecasts->lons[NLON - 1] - forecasts->lons[0]) / (NLON - 1);
	float deltaLat = (forecasts->lats[NLAT - 1] - forecasts->lats[0]) / (NLAT - 1);
	int init_lat_index = (int)ceil(((lat - forecasts->lats[0]) / deltaLat));
	int init_lon_index = (int)floor(((lon - (forecasts->lons[0])) / deltaLon));

	// get the upper left point coordinates for the region surounding the coordinate.
	init_lat_index += (int)(ANALOG_DIMENSION / 2);
	init_lon_index -= (int)(ANALOG_DIMENSION / 2);

	// define the values array
	float* values = new float[(ANALOG_DIMENSION + 1)*(ANALOG_DIMENSION + 1)];

	float min_value = MAX_FLOAT;
	float max_value = MIN_FLOAT;

	// iterate X as the longitud of the surounding points.
	for (int x = 0; x <= ANALOG_DIMENSION; x++){ // lon

		int ix = x + init_lon_index;

		if (ix < 0) continue;
		if (ix > NLON) continue;

		// iterate Y as the latitude of the surounding points.
		for (int y = 0; y <= ANALOG_DIMENSION; y++){ // lon

			int iy = init_lat_index - y;

			if (iy < 0) continue;
			if (iy >= NLAT) continue;

		 	float value = current_forecast_by_range[accumulation_range][ (iy * NLON) + ix];

			// assign the value
			values[y*(ANALOG_DIMENSION + 1) + x] = value;

			if (max_value < value) max_value = value;
			if (min_value > value) min_value = value;
		}
	}

	float* interpolated_values = Interpolate3(values, ANALOG_DIMENSION + 1, ANALOG_DIMENSION + 1,
		1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f,
		1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f, 256, 256, false);

	delete(values);

	RegionForecastCollection *c = new RegionForecastCollection(1);
	c->forecast_region[0] = new RegionForecast();
	c->forecast_region[0]->forecast_values = interpolated_values;
	c->max_forecast_value = max_value;
	c->min_forecast_value = min_value;
	
	return c;
}


// Renders the block of ANALOG_DIMENSION panels surounding the desired (lat, lon) block.
AnalogsResponse* Albero2::RenderAnalogForecasts(float lat, float lon, int fhour){
	
	PCT::SelectScale(-3, false); // deberia ser 0

	// the result (the array of AnalogImages, which is a date and the filename).
	// the images part of the response
	AnalogsResponse *result = new AnalogsResponse(N_ANALOGS_PER_LAT_LON);
	
	RegionForecastCollection* analogs = GetAnalogForecasts(lat, lon, fhour);
	RegionForecastCollection* current = GetCurrentForecast(lat, lon, fhour);

	cout << "current: " << current->min_forecast_value << " to " << current->max_forecast_value << endl;
	cout << "analog: " << analogs->min_forecast_value << " to " << analogs->max_forecast_value << endl;
	cout << "observation: " << analogs->min_observation_value << " to " << analogs->max_observation_value << endl;


	// Max and Min for the forecasts
	float max_forecast_value = analogs->max_forecast_value; //fmaxf(analogs.max_forecast_value, current.max_forecast_value);
	float min_forecast_value = analogs->min_forecast_value; //fminf(analogs.min_forecast_value, current.min_forecast_value);

	float min_value = 0;
	float max_value = 60;

	for (int analog_index = 0; analog_index < N_ANALOGS_PER_LAT_LON; analog_index++){
		
		RegionForecast* analog = analogs->forecast_region[analog_index];

		// Render the Analog Forecast
		string filename = "analog_region_" + to_string(analog_index) + "_" + to_string(analog->date) + ".png";
		
		// Add to result
		result->Add(filename, analog->date, analog->mse);
		
		// Write Forecast Image
		WriteImage(analog->forecast_values, 152, 152, albero_images_path + filename, min_value, max_value);

		// Write Observation Image
		WriteImage(analog->observation_values, 152, 152, albero_images_path + "observation_analog_region_" + to_string(analog_index) + "_" + to_string(analog->date) + ".png", min_value, max_value);
	}
	
	// Write Mean Observation
	WriteImage(analogs->mean_observation_values, 152, 152, albero_images_path + "analog_observation_mean.png", min_value, max_value);


	// Write Mean Forecast
	WriteImage(analogs->mean_forecast_values, 152, 152, albero_images_path + "analog_region_mean.png", min_value, max_value);


	// Write the BIAS



	// Write Current Forecast
	float *values = current->forecast_region[0]->forecast_values;
	//string filename = "forecast_region_" + to_string((int)lat) + "_" + to_string((int)lon) + "_.png";
	string filename = "forecast_region.png";
	WriteImage(values, 256, 256, albero_images_path + filename, min_value, max_value);
	//delete(values);

	// define the values array we'll use to store the BIAS (the difference between the forecast media and observations media
	float* bias = new float[152 * 152];
	memset(bias, 0, 152 * 152 * sizeof(int));
	float min_bias_value = MAX_FLOAT;
	float max_bias_value = MIN_FLOAT;
	// Now calculate the mean of the observations based on the accumulation, and the BIAS
	for (int i = 0; i < 152 * 152; i++){

		float analog_mean_value = ((floor(analogs->mean_forecast_values[i] / 5.0f))*5.0f);
		float observation_mean_value = ((floor(analogs->mean_observation_values[i] / 5.0f))*5.0f);
		float bias_discretized = analog_mean_value - observation_mean_value;
		float bias_plain = analogs->mean_forecast_values[i] - analogs->mean_observation_values[i];
		bias_plain = ((floor(bias_plain / 5.0f))*5.0f);

		bias[i] = bias_plain;
	
		//	cout << bias[i] << "\t";
		min_bias_value = fminf(min_bias_value, bias_plain);
		max_bias_value = fmaxf(max_bias_value, bias_plain);
	}



	//min_bias_value = -30.0f;
	//max_bias_value = 30.0f;


	float max_abs_bias = fmaxf(fabsf(max_bias_value), fabsf(min_bias_value));

	PCT::SelectScale(3, false); // so it uses the scale file with multiple levels
	Color::enable_invert_palette = false;
	Color::enable_zero_transparency = false;
	WriteImage(bias, 152, 152, albero_images_path + "analog_region_bias.png", -max_abs_bias, max_abs_bias);


	result->max_forecast_value = max_forecast_value;
	result->min_forecast_value = min_forecast_value;
	result->min_observation_value = analogs->min_observation_value;
	result->max_observation_value = analogs->max_observation_value;
	result->min_value = min_value;
	result->max_value = max_value;
	result->max_bias_value = max_bias_value;
	result->min_bias_value = min_bias_value;

	result->region_max_forecast_value = current->max_forecast_value;
	result->region_min_forecast_value = current->min_forecast_value;

	result->min_mean_forecast_value = analogs->min_mean_forecast_value;
	result->max_mean_forecast_value = analogs->max_mean_forecast_value;

	result->min_mean_observation_value = analogs->min_mean_observation_value;
	result->max_mean_observation_value = analogs->max_mean_observation_value;

	delete(analogs);
	delete(current);
	
	delete(bias);

	return result;
}


RegionForecastCollection* Albero2::GetAnalogForecasts(float lat, float lon, int accumulation_range){

	// The analog collection we'll return
	RegionForecastCollection* c = new RegionForecastCollection(N_ANALOGS_PER_LAT_LON);
	
	// Initialize the forecast region array that will be part of the response
	RegionForecast **forecast_region = c->forecast_region;

	// get the lat lon index of the specified coordinate
	float deltaLon = (forecasts->lons[NLON - 1] - forecasts->lons[0]) / (NLON - 1);
	float deltaLat = (forecasts->lats[NLAT - 1] - forecasts->lats[0]) / (NLAT - 1);
	int init_lat_index = (int)ceil(((lat - forecasts->lats[0]) / deltaLat));
	int init_lon_index = (int)floor(((lon - (forecasts->lons[0])) / deltaLon));

	// Retrieve the analog index
	size_t index = CPU_ANALOG_INDEX(accumulation_range, init_lat_index, init_lon_index);
	AnalogIndex *local_analogs = &analogs[index];

	// get the upper left point coordinates for the region surounding the coordinate.
	init_lat_index += (int)(ANALOG_DIMENSION / 2);
	init_lon_index -= (int)(ANALOG_DIMENSION / 2);
	
	// define the values array we'll use to store the mean of the analogs
	float* mean_analog = new float[(ANALOG_DIMENSION + 1)*(ANALOG_DIMENSION + 1)];
	memset(mean_analog, 0, (ANALOG_DIMENSION + 1)*(ANALOG_DIMENSION + 1) * sizeof(float));

	// define the values array we'll use to store the mean of the observation
	float* mean_observation = new float[152*152];
	memset(mean_observation, 0, 152 * 152 * sizeof(float));

	float min_analog = MAX_FLOAT;
	float max_analog = 0;
	float min_observation = MAX_FLOAT;
	float max_observation = 0;

	// Iterate the analogs
	for (int analog_index = 0; analog_index < N_ANALOGS_PER_LAT_LON; analog_index++)
	{

		int date = local_analogs[analog_index].date;

		// define the values array
		float* values = new float[(ANALOG_DIMENSION + 1)*(ANALOG_DIMENSION + 1)];

		// iterate X as the longitud of the surounding points.
		for (int x = 0; x <= ANALOG_DIMENSION; x++){ // lon

			int ix = x + init_lon_index;

			if (ix < 0) continue;
			if (ix > NLON) continue;

			// iterate Y as the latitude of the surounding points.
			for (int y = 0; y <= ANALOG_DIMENSION; y++){ // lon

				int iy = init_lat_index - y;

				if (iy < 0) continue;
				if (iy >= NLAT) continue;

				float value = this->historic_forecast_by_range[accumulation_range][local_analogs[analog_index].year * NTIME_WINDOW*NLAT*NLON + (local_analogs[analog_index].time * NLAT * NLON) + (iy * NLON) + ix];

				// assign the value
				values[y*(ANALOG_DIMENSION + 1) + x] = value; // store value for this particular analog
				mean_analog[y*(ANALOG_DIMENSION + 1) + x] += value; // accumulate value for the mean analog

				if (max_analog < value) max_analog = value;
				if (min_analog > value) min_analog = value;

			}
		}

		// We have the values, which could be obtained using a more efficient method, but this gives me an idea of how well it's working,
		// so now we can write an image.



		float* interpolated_values = Interpolate3(values, ANALOG_DIMENSION + 1, ANALOG_DIMENSION + 1,
			1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f,
			1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f, 152, 152, false);
		delete(values);

		forecast_region[analog_index] = new RegionForecast();
		forecast_region[analog_index]->date = date;
		forecast_region[analog_index]->forecast_values = interpolated_values;
		forecast_region[analog_index]->mse = local_analogs[analog_index].mse;
		

		/// ----------------------------------------------------------------------
		/// Now let's try to print the observation of the analog.


		float init_lat = forecasts->lats[init_lat_index - ANALOG_DIMENSION];
		float end_lat = forecasts->lats[init_lat_index];

		float init_lon = forecasts->lons[init_lon_index];
		float end_lon = forecasts->lons[init_lon_index + ANALOG_DIMENSION];

		float *observation_values = ObservationReader::GetInterpolatedValues(date, accumulation_range, init_lat, init_lon, end_lat, end_lon, 152, 152);
		
		forecast_region[analog_index]->observation_values = observation_values;

		// accumulate the observation value
		for (int i = 0; i < 152 * 152; i++) {
			mean_observation[i] += observation_values[i];
			if (max_observation < observation_values[i]) {
				max_observation = observation_values[i];
				//cout << "new max:" << max_observation << endl;
				//cout << date << endl;
			}
			if (min_observation >  observation_values[i]) min_observation = observation_values[i];
		}
		// -----------------------------------------------------------------------

	}

	// Divide the addition of all observations by the amount of forecast and calculate max and min of the mean.
	float min_mean_observation_value = MAX_FLOAT;
	float max_mean_observation_value = 0;
	for (int i = 0; i < 152 * 152; i++){
		mean_observation[i] /= N_ANALOGS_PER_LAT_LON;
		min_mean_observation_value = fminf(mean_observation[i], min_mean_observation_value);
		max_mean_observation_value = fmaxf(mean_observation[i], max_mean_observation_value);
	}


	// Interpolate the addition of all forecasts
	float* interpolated_mean = Interpolate3(mean_analog, ANALOG_DIMENSION + 1, ANALOG_DIMENSION + 1,
		1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f,
		1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f, 152, 152, false);
	delete(mean_analog);

	// Divide the addition of all forecasts by the amount of forecast and calculate max and min of the mean.
	float min_mean_forecast_value = MAX_FLOAT;
	float max_mean_forecast_value = 0;
	for (int i = 0; i < 152 * 152; i++){
		interpolated_mean[i] /= N_ANALOGS_PER_LAT_LON;
		min_mean_forecast_value = fminf(interpolated_mean[i], min_mean_forecast_value);
		max_mean_forecast_value = fmaxf(interpolated_mean[i], max_mean_forecast_value);
	}


	// Data we will return.
	c->max_forecast_value = max_analog;
	c->min_forecast_value = min_analog;
	c->max_observation_value = max_observation;
	c->min_observation_value = min_observation;
	c->mean_observation_values = mean_observation;
	c->mean_forecast_values = interpolated_mean;
	c->min_mean_observation_value = min_mean_observation_value;
	c->max_mean_observation_value = max_mean_observation_value;
	c->min_mean_forecast_value = min_mean_forecast_value;
	c->max_mean_forecast_value = max_mean_forecast_value;


	return c;
}


// Calculates probabilistic forecast.
int Albero2::CalculateProbabilisticForecast(){

	// This matrix will hold the observations for all the points on the map. We'll call this the probability points, which really contains the 
	// different observations and the weights associated to the point on the overlapping calculations.
	//int analog_observation_height = ((NLAT-1) * (OBSERVATIONS_PER_PANEL - 1)) + 1;
	//int analog_observation_width = ((NLON-1) * (OBSERVATIONS_PER_PANEL - 1)) + 1;

	probability_map_height = ((NLAT - 1) * (OBSERVATIONS_PER_PANEL-1)) + 1;
	probability_map_width = ((NLON - 1) * (OBSERVATIONS_PER_PANEL-1)) + 1;

	int analog_dimension = ANALOG_DIMENSION * ANALOG_DIMENSION;
	int analog_observations_size = probability_map_width * probability_map_height * analog_dimension * (N_ANALOGS_PER_LAT_LON + 1); // Note that we have +1 since the first one will be the distance to the center of the analog area for that point in that dimension.
 	float* probability_points = new float[analog_observations_size]; // intermediate to generate the probability map
	

	// one probability map per threshold per accumulation range
	probability_map = new float*[threshold_ranges.size() * nAccumulationRanges];


	// Iterate the accumulation range
	for (int accumulation_range_index = 0; accumulation_range_index < nAccumulationRanges; accumulation_range_index++)
	{
		memset(probability_points, 0, analog_observations_size * sizeof(float));

		float max_distance = 0;

		// Iterate all the lats and lons. 
		for (int lat_index = 0; lat_index < NLAT ; lat_index++){

			float point_lat = forecasts->lats[lat_index];
			float debug_lat = forecasts->lats[0] + lat_index * 4 * 0.25f;

			for (int lon_index = 0; lon_index < NLON; lon_index++){

				// the lat and lon of the current dot (upper left corner)
				float point_lon = forecasts->lons[lon_index];
				float debug_lon = forecasts->lons[0] + lon_index * 4 * 0.25f;

				// get the center of the analog area
				float center_lat = (point_lat - 0.5f); // BECAUSE WE KNOW THE RESOLUTION IS 1 DEGREE
				float center_lon = (point_lon + 0.5f); // BECAUSE WE KNOW THE RESOLUTION IS 1 DEGREE

				// Retrieve the analog index
				size_t index = CPU_ANALOG_INDEX(accumulation_range_index, lat_index, lon_index);
				AnalogIndex *local_analogs = &analogs[index];

				// now we retrieve the observations for the whole analog area (the area surounding the current block)

				/*
				float init_lat = point_lat - floor((ANALOG_DIMENSION) / 2.0f) * 1.0f; // BY 1.0 BECAUSE THE RESOLUTION IS 1 DEGREE
				float end_lat = init_lat + ANALOG_DIMENSION * 1.0f; // BY 1.0 BECAUSE THE RESOLUTION IS 1 DEGREE
				float init_lon = point_lon - floor((ANALOG_DIMENSION) / 2.0f) * 1.0f; // BY 1.0 BECAUSE THE RESOLUTION IS 1 DEGREE
				float end_lon = init_lon + ANALOG_DIMENSION * 1.0f; // BY 1.0 BECAUSE THE RESOLUTION IS 1 DEGREE
				*/
				
				float init_lat = point_lat + 1;
				float end_lat = point_lat - 2;

				float init_lon = point_lon - 1;
				float end_lon = point_lon + 2;
				

				// here we'll store the analogs observations for the region.
				float** region_observations = new float*[N_ANALOGS_PER_LAT_LON];

				// get the amount of observations we'll have on the whole region (this is, the width/height in observations that we'll retrieve for the analog region.
				int observations_in_region_width = ANALOG_DIMENSION * (OBSERVATIONS_PER_PANEL-1) + 1;

				// iterate the analogs and retrieve the observation for the region.
				for (int analog_index = 0; analog_index < N_ANALOGS_PER_LAT_LON; analog_index++){

					// get the analog date.
					//int date = forecasts->forecast_days[(int)(NTIME - (NYEARS - local_analogs[analog_index].year)*365.25f - 45) + local_analogs[analog_index].time];
					//	date = 20110826;

					// get the observation
					float *observation_values = ObservationReader::GetInterpolatedValues(local_analogs[analog_index].date, accumulation_range_index, init_lat, init_lon, end_lat, end_lon, observations_in_region_width, observations_in_region_width);
					
					/*
					if (point_lat == -34 && point_lon == -59){
						for (int i = 0; i < observations_in_region_width*observations_in_region_width; i++){
						//	cout << observation_values[i] << " ";
						}
					}
					else
					{
						for (int i = 0; i < observations_in_region_width*observations_in_region_width; i++){
							observation_values[i] = 0;
						}
					}
					*/

					if (observation_values == NULL){
						cout << "ERROR!!!!" << endl;
					}
					// store in region_observations array
					region_observations[analog_index] = observation_values;
				}

				// Now we iterate the probability points inside the region and assign the found observation values.
				// IMPORTANT: Note that we are ignoring the last one, since it will be considered as part of the neighbor
				for (int y = 0; y < observations_in_region_width - 1; y++){

					for (int x = 0; x < observations_in_region_width - 1; x++){

						// x and y are local indices to the probability points on this particular region, so we need
						// to find the global indices to the global probability points matrix.
						int global_x = (int)(lon_index - 1) * (OBSERVATIONS_PER_PANEL-1)+x;
						int global_y = (int)(lat_index - 2) * (OBSERVATIONS_PER_PANEL-1)+y;

						
						// careful, when calculating for the borders, it may get out of range (since, for instance, to the left of the first tile there's nothing).
						if (global_x < 0 || global_y < 0 || global_x >= probability_map_width || global_y >= probability_map_height){
							continue;
						}
						// now we need the dimension from which we are retrieving the observations (ul, um, ur, ml, m, mr, ll, lm)
						int dimension_x = x / (OBSERVATIONS_PER_PANEL-1 );
						int dimension_y = y / (OBSERVATIONS_PER_PANEL-1 );
						int dimension = dimension_y * ANALOG_DIMENSION + dimension_x;

						// now let's calculate the distance between the point and the center of the region
						float sub_point_lat = forecasts->lats[0] + global_y * 0.25f;
						float sub_point_lon = forecasts->lons[0] + global_x * 0.25f;
						
					/*	if (point_lat == -34 && point_lon == -59){
							cout << "[lat,lon]=" << sub_point_lat << " : " << sub_point_lon << endl;
						}
					*/	


					/*	if (sub_point_lat == -33 && sub_point_lon == -60){
							//assert(global_x == 70);
							//assert(global_y == 86);
							cout << global_x;
							cout << global_y;
						}
						/*
						if (point_lat == -34 && point_lon == -59){
							cout << "[x,y]=" << global_x << " : " << global_y << endl;
						}
						*/

						// find the index to the probability point matrix 
						int pp_index = CPU_PROBABILITY_POINT(global_x, global_y, dimension);
						float *point_observations = &probability_points[pp_index];

						// store the distance of the point to the center of the analog region 
						point_observations[0] = Distance(sub_point_lat, sub_point_lon, center_lat, center_lon);
						if (max_distance < point_observations[0]) max_distance = point_observations[0];

						// iterate the analogs and retrieve the observations for the particular point
						for (int analog_index = 0; analog_index < N_ANALOGS_PER_LAT_LON; analog_index++){
							float *observation_values = region_observations[analog_index];
							// Note that we skip the first one since it is the distance of the point to the center of the analog area for this dimension.
							point_observations[analog_index + 1] = observation_values[(observations_in_region_width - y - 1)*observations_in_region_width + x];
						}

					}

				}
				
				// some memory cleaning
				for (int h = 0; h < N_ANALOGS_PER_LAT_LON; h++){
					delete(region_observations[h]);
				}
				delete(region_observations);

			}

		}

		cout << "." << endl;


		float D = max_distance; // maximun distance (used on the weight calculation)
		
		//float threshold = 0.0f; // we are looking for observations higher than the threshold.
		//int threshold_amount = 5;
		//int max_threshold = 80;
		//float step = (float)max_threshold / ((float)threshold_amount - 1.0f);

		//for (int threshold_index = 0; threshold_index < threshold_amount; threshold_index++)
		int threshold_index = 0;
		for (std::list<ThresholdRange>::const_iterator iterator = threshold_ranges.begin(), end = threshold_ranges.end(); iterator != end; ++iterator) 
		{
			ThresholdRange range = *iterator;

			cout << range.from << "   " << endl;

			//float threshold = threshold_index * step;

			size_t probability_map_index = accumulation_range_index * threshold_ranges.size() + threshold_index;

			probability_map[probability_map_index] = new float[(probability_map_width)*(probability_map_height)];
			memset(probability_map[probability_map_index], 0, (probability_map_width)*(probability_map_height)* sizeof(float));


			/************ DEBUG TESTING ***/
			/*
			float init_lat = forecasts->lats[0];
			float init_lon = forecasts->lons[0];

			for (int y = 0; y < probability_map_height; y++){
			
				for (int x = 0; x < probability_map_width; x++){

					float lat = init_lat + y * 0.25f;
					float lon = init_lon + x * 0.25f;

					if (lat == -34.5 && lon == -58.5){
						probability_map[probability_map_index][y * probability_map_width + x] = 100;
					}
					

				}
			}
			*/

				//	probability_map[probability_map_index][y*probability_map_width + x] = v;

			/*
			if ((y >= 0 && y <= 1 && x >= 0 && x < probability_map_width) ){
			probability_map[probability_map_index][(probability_map_height - y-1)*probability_map_width + x] = 100;
			}
			else
			{
			probability_map[probability_map_index][(probability_map_height - y - 1)*probability_map_width + x] = 0;
			}
			*/



			/******************************/




			// --------------------------------------------------------
			// Generate the smoothed probability matrix
			// --------------------------------------------------------

			

			float min_value = MAX_FLOAT;
			float max_value = 0;


			float min_weight= MAX_FLOAT;
			float max_weight = 0;
			float min_normalized_weight = MAX_FLOAT;
			float max_normalized_weight = 0;



			for (int y = 0; y < probability_map_height; y++){
				for (int x = 0; x < probability_map_width; x++){

					float weight_sum = 0; // the sum of weights of different dimensions.

					// iterate the analogs and get the weight sum of all dimenssions.
					for (int dimension = 0; dimension < ANALOG_DIMENSION*ANALOG_DIMENSION; dimension++)
					{
						int pp_index = CPU_PROBABILITY_POINT(x, y, dimension);
						float *point_observations = &probability_points[pp_index];

						// get the distance
						float d = point_observations[0];
						
						// calculate weight
						float weight = (d >= D) ? 0 : (D - d) / (D + d);
						
						//weight = 1.0f;

						// accumulate weight
						weight_sum += weight;

						// store weight instead of distance
						point_observations[0] = weight;
					}

					float v = 0; // probabilistic value for the point

					// iterate all the dimensions
					for (int dimension = 0; dimension < ANALOG_DIMENSION*ANALOG_DIMENSION; dimension++)
			//		int dimension = 4;
					{

						// retroeve the observations for the point in the current dimension
						int pp_index = CPU_PROBABILITY_POINT(x, y, dimension);
						float *point_observations = &probability_points[pp_index];

						// obtain the weight and normalize it
						float weight = point_observations[0];
						float normalized_weight = 0;
						if (weight_sum != 0){
							normalized_weight = weight / weight_sum;
							//cout << weight << " " << normalized_weight << endl;
						}

						max_normalized_weight = fmaxf(normalized_weight, max_normalized_weight);
						min_normalized_weight = fminf(normalized_weight, min_normalized_weight);
						max_weight = fmaxf(weight, max_weight);
						min_weight = fminf(weight, min_weight);

						// lets calculate the probability
						int amount_above_treshold = 0;
						// count the amount above threhsold for this point and dimension (or inside range)
						for (int analog_index = 0; analog_index < N_ANALOGS_PER_LAT_LON; analog_index++){
							
							float point_value = point_observations[analog_index + 1];

							if (point_value > range.from ) {
								amount_above_treshold++; // note that +1 since 0 is the weight
							}

						}

						// calculate the probability
						float probability = (100.0f / N_ANALOGS_PER_LAT_LON)*amount_above_treshold;

						//normalized_weight = 1;
						// acumulate the probability using the dimensions weight.
						v += probability *normalized_weight;


						//v = point_observations[1];
						//delete(point_observations);
					}
					/*
					if (x >= 64 && x <= 76 && y >= 80 && y <= 92){
						probability_map[probability_map_index][y*probability_map_width + x] = 100;
					}
					*/
					max_value = fmaxf(v, max_value);
					min_value = fminf(v, min_value);


					probability_map[probability_map_index][y*probability_map_width + x] = v;




					//x = 64...75
					//	y = 84...95
					/*
					if (y == 92 || x == 64){
					//if (x>=64 && x<=76 && y>=80 && y<=92){
						probability_map[probability_map_index][y*probability_map_width + x] = 100;
					}else
					{
						probability_map[probability_map_index][y*probability_map_width + x] = 0;
					}
					*/
				}
			}

			//stats
			this->stats->max_probabilistic_forecast[accumulation_range_index] = max_value;
			this->stats->min_probabilistic_forecast[accumulation_range_index] = min_value;
			


			/************ DEBUG TESTING ***/
			
			float init_lat = forecasts->lats[0];
			float init_lon = forecasts->lons[0];

			int lat_index = 0;
			int lon_index = 0;

			for (int y = 0; y < probability_map_height; y++){

				for (int x = 0; x < probability_map_width; x++){

					float lat = init_lat + y * 0.25f;
					float lon = init_lon + x * 0.25f;


					if (x % 4 == 0 && y % 4 == 0){
						lon_index = x / 4;
						lat_index = y / 4;
						assert(forecasts->lats[lat_index] = lat);
						assert(forecasts->lons[lon_index] = lon);
					}
					
					if (y == 0 && x ==00){
						probability_map[probability_map_index][y * probability_map_width + x] = 100;
					}
					/*else{
						probability_map[probability_map_index][y * probability_map_width + x] = 0;
					}*/

				/*	if (int(lat) == -33 && int(lon) == -60){
						probability_map[probability_map_index][y * probability_map_width + x] = 100;
						//						cout << "lon_index : lat_index : global_x : global_y =" << lon_index << ":" << lat_index << ":" << x << " : " << y << endl;
					}
					else{
						probability_map[probability_map_index][y * probability_map_width + x] = 0;
					}*/
				}
			}
			/************ DEBUG TESTING ***/
			
			


			PCT::SelectScale(-7, true);

			// Grabamos los valores interpolados 
			float* interpolated_values = Interpolate3(probability_map[probability_map_index], probability_map_width, probability_map_height,
				1.0f, 1.0f, (float)probability_map_width, (float)probability_map_height,
				1.0f, 1.0f, (float)probability_map_width, (float)probability_map_height, probability_map_height * 3, probability_map_width * 3, true);
			WriteImage(interpolated_values, probability_map_width * 3, probability_map_height * 3, albero_images_path + "forecast8_prob_map_" + to_string(threshold_index) + "_" + to_string(accumulation_range_index) + ".png", 0, 100);
			delete(interpolated_values);

			cout << albero_images_path + "forecast8_prob_map_" + to_string(threshold_index) + "_" + to_string(accumulation_range_index) + ".png" << endl;
			
			threshold_index++;

			
		}

		cout << "." << endl;

	}

	delete(probability_points);

	return 0;


}




int Albero2::Initialize(int current_date)
{
	initialized = true;

	// Lets clean the structures if they were used
	if (this->forecasts != NULL) delete(forecasts); forecasts = NULL;
	if (this->probability_map != NULL) delete(probability_map); probability_map = NULL;
	if (this->mean_square_error_map != NULL) delete(mean_square_error_map); mean_square_error_map = NULL;
	if (this->current_forecast_by_range != NULL) delete(current_forecast_by_range); current_forecast_by_range = NULL;
	if (this->historic_forecast_by_range != NULL) delete(historic_forecast_by_range); historic_forecast_by_range = NULL;
	if (this->analogs != NULL) delete(analogs); analogs = NULL;
	if (this->stats!= NULL) delete(stats); stats = NULL;

	this->stats = new Statistics(nAccumulationRanges); //3 accumulation ranges for now

	this->current_date = current_date;
	
	cout << ":: Initializing..." << endl;

	// Read the historic forecasts
	cout << ":: Reading historic forecasts..." << endl;
	ReadHistoricForecasts(current_date);
	

	// Calculate Analogs
	cout << ":: Calculating analogs..." << endl;
	CalculateAnalogs();

	cout << ":: Calculating probabilistic forecast..." << endl;
	CalculateProbabilisticForecast();




	// We'll do one last thing, which is calculating max and min for the observed data on the accumulation ranges
	for (int range = 0; range < nAccumulationRanges; range++){
		
		float *values = ObservationReader::GetRawValues(current_date * 100, range);

		float min_value = MAX_FLOAT;
		float max_value = 0;

		for (int i = 0; i < ObservationReader::file_lats * ObservationReader::file_lons; i++){
			float v = values[i];
			max_value = fmaxf(max_value, v);
			min_value = fminf(min_value, v);
		}
	
		this->stats->min_observation[range] = min_value;
		this->stats->max_observation[range] = max_value;

		delete(values);
	}







	return 0;
}


/*
// Sets the drawing values ranges
void Albero2::SetDrawingValues(float min, float max){
	this->render_max_value = max;
	this->render_min_value = min;
}

void Albero2::ResetDrawingValues(){
	this->render_max_value = MAX_FLOAT;
	this->render_min_value = MAX_FLOAT;
}*/






Statistics::Statistics(int accumulation_ranges){
	this->max_observation = new float[accumulation_ranges];
	this->min_observation = new float[accumulation_ranges];
	this->max_numerical_forecast = new float[accumulation_ranges];
	this->min_numerical_forecast = new float[accumulation_ranges];
	this->max_mse = new float[accumulation_ranges];
	this->min_mse = new float[accumulation_ranges];
	this->min_probabilistic_forecast = new float[accumulation_ranges];
	this->max_probabilistic_forecast = new float[accumulation_ranges];
	this->accumulation_ranges = accumulation_ranges;
}

Statistics::~Statistics(){
	delete(this->max_observation);
	delete(this->min_observation);
	delete(this->max_numerical_forecast);
	delete(this->min_numerical_forecast);
	delete(this->min_mse);
	delete(this->max_mse);
	delete(this->min_probabilistic_forecast);
	delete(this->max_probabilistic_forecast);
}

string Statistics::ToJSON(){
	// Build the result, something like: [{"date":"232323232", "filename" : "fdsfdsffsfsfsf"}]
	string result = "[";
	for (int i = 0; i < accumulation_ranges; i++){
	
		result += 
			"{ \"max_numerical_forecast\":\"" + to_string(this->max_numerical_forecast[i]) + "\"" +
			", \"min_numerical_forecast\":\"" + to_string(this->min_numerical_forecast[i]) + "\"" +
			", \"max_mse\":\"" + to_string(this->max_mse[i]) + "\"" +
			", \"min_mse\":\"" + to_string(this->min_mse[i]) + "\"" +
			", \"max_observation\":\"" + to_string(this->max_observation[i]) + "\"" +
			", \"min_observation\":\"" + to_string(this->min_observation[i]) + "\"" +
			", \"max_probabilistic_forecast\":\"" + to_string(this->max_probabilistic_forecast[i]) + "\"" +
			", \"min_probabilistic_forecast\":\"" + to_string(this->min_probabilistic_forecast[i]) + "\""
			"}";

		if (i < accumulation_ranges-1) result += ",";
	}
	
	result += "]";

	return result;
}