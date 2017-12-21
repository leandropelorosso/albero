#include "albero2.h"
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
#include "observation_reader.h"
#include "color.h"
#include "pct.h"
#include "forecast_reader.h"
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
    if (this->observations!= NULL) delete(observations);

    if (this->probability_map != NULL){
        for (int i = 0; i < this->threshold_ranges.size() * this->accumulation_ranges.size(); i++){
            delete(probability_map[i]);
        }
        delete(probability_map);
    }

    if (this->mean_square_error_map != NULL) {
        for (int i = 0; i < this->accumulation_ranges.size(); i++){
            delete(mean_square_error_map[i]);
        }
        delete(mean_square_error_map);
    }

    if (this->analogs != NULL) delete(analogs);
    if (this->stats != NULL) delete(stats);

}


// Reads the historic forecasts from the forecasts netcdf.
int Albero2::ReadHistoricForecasts(int current_date){

    forecasts->Read(current_date);

    NFHOUR = (int)forecasts->NFHOUR;
    NLAT = (int)forecasts->NLAT;
    NLON = (int)forecasts->NLON;
    NTIME = (int)forecasts->NTIME;


    size_t dev_analogs_size = NLAT * NLON * this->accumulation_ranges.size() * N_ANALOGS_PER_LAT_LON;
    analogs = new AnalogIndex[dev_analogs_size];


    // TODO: READ AND GET STATS
    //stats->min_numerical_forecast[range] = min_value;
    //stats->max_numerical_forecast[range] = max_value;
}


// Calculates the Analogss
int Albero2::CalculateAnalogs(){

    //This line should be all, what you need to add to your code.
    static_assert(std::numeric_limits<float>::is_iec559, "IEEE 754 required");


    float max_mse = MIN_FLOAT;
    float min_mse = MAX_FLOAT;

    // Foreach accumulation Range
    for (int accumulation_range = 0; accumulation_range< this->accumulation_ranges.size(); accumulation_range++){

        // Retrieve the forecast for the selected date [NLATxNLON]
        float* current_forecast = &this->forecasts->forecasts_by_range[accumulation_range][this->forecasts->forecast_index[this->current_date]];

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

                // now, for the current (lat,lon) point, we'll iterate the surounding points, for evey date on the time window
                for (int date : forecasts->forecast_date)
                {
                    // get the historic forecast [NLATxNLON]
                    // TODO: It is not necessary to use forecast_index here, it can be replaces by a simple calculation.

                    if(!this->observations->HasDate(date)) continue; // Only if there is an observation associated with the forecast.
                    float* historic_forecast = &this->forecasts->forecasts_by_range[accumulation_range][this->forecasts->forecast_index[date]];

                    // ... and calculate the MSE of the region suroinding the center block.

                    float mse = 0;  // mse of the block
                    int cells_in_block = 0;; // amount of cells on the block

                    // iterate X as the longitud of the surounding points.
                    for (int x = lon - ((int)ANALOG_DIMENSION / 2); x <= lon + ((int)ANALOG_DIMENSION / 2) + 1; x++){ // lon

                        if (x < 0) continue;
                        if (x >= NLON) continue;

                        // iterate Y as the latitude of the surounding points.
                        for (int y = lat - ((int)ANALOG_DIMENSION / 2)-1; y <= lat + ((int)ANALOG_DIMENSION / 2); y++){ // lat

                            if (y < 0) continue;
                            if (y >= NLAT) continue;

                            float current_mean = current_forecast[y*NLON + x];
                            float historic_mean = historic_forecast[y*NLON + x];

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
                            maxAnalog->lat = lat;
                            maxAnalog->lon = lon;
                            maxAnalog->date = date;
                        }
                    }

                }
            }
        }
    }

    // create the mean square error map for the accumulation ranges
    this->mean_square_error_map = new float*[this->accumulation_ranges.size()];

    // Iterate the accumulation ranges
    for (int accumulation_range = 0; accumulation_range < this->accumulation_ranges.size(); accumulation_range++){

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
    // Retrieve the forecast for the selected date [NLATxNLON]
    float* current_forecast = &this->forecasts->forecasts_by_range[accumulation_range][this->forecasts->forecast_index[this->current_date]];

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

            float value = current_forecast[ (iy * NLON) + ix];

            // assign the value
            values[y*(ANALOG_DIMENSION + 1) + x] = value;

            if (max_value < value) max_value = value;
            if (min_value > value) min_value = value;
        }
    }

    float* interpolated_values = Interpolate3(values, ANALOG_DIMENSION + 1, ANALOG_DIMENSION + 1,
        1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f,
        1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f, 220, 220, false);

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
        WriteImage(PCT::numerical_forecast_schema, analog->forecast_values, 220, 220, albero_images_path + filename, min_value, max_value);

        // Write Observation Image
        WriteImage(PCT::numerical_forecast_schema, analog->observation_values, 220, 220, albero_images_path + "observation_analog_region_" + to_string(analog_index) + "_" + to_string(analog->date) + ".png", min_value, max_value);
    }

    // Write Mean Observation
    WriteImage(PCT::numerical_forecast_schema, analogs->mean_observation_values, 220, 220, albero_images_path + "analog_observation_mean.png", min_value, max_value);


    // Write Mean Forecast
    WriteImage(PCT::numerical_forecast_schema, analogs->mean_forecast_values, 220, 220, albero_images_path + "analog_region_mean.png", min_value, max_value);


    // Write the BIAS



    // Write Current Forecast
    float *values = current->forecast_region[0]->forecast_values;
    //string filename = "forecast_region_" + to_string((int)lat) + "_" + to_string((int)lon) + "_.png";
    string filename = "forecast_region.png";
    WriteImage(PCT::numerical_forecast_schema, values, 220, 220, albero_images_path + filename, min_value, max_value);
    //delete(values);

    // define the values array we'll use to store the BIAS (the difference between the forecast media and observations media
    float* bias = new float[220 * 220];
    memset(bias, 0, 220 * 220 * sizeof(int));
    float min_bias_value = MAX_FLOAT;
    float max_bias_value = MIN_FLOAT;
    // Now calculate the mean of the observations based on the accumulation, and the BIAS
    for (int i = 0; i < 220 * 220; i++){

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
    WriteImage(PCT::bias_schema, bias, 220, 220, albero_images_path + "analog_region_bias.png", -max_abs_bias, max_abs_bias);


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
    float* mean_observation = new float[220*220];
    memset(mean_observation, 0, 220 * 220 * sizeof(float));

    float min_analog = MAX_FLOAT;
    float max_analog = 0;
    float min_observation = MAX_FLOAT;
    float max_observation = 0;

    // Iterate the analogs
    for (int analog_index = 0; analog_index < N_ANALOGS_PER_LAT_LON; analog_index++)
    {

        int date = local_analogs[analog_index].date;

        // Retrieve the forecast for the analog date [NLATxNLON]
        float* forecast = &this->forecasts->forecasts_by_range[accumulation_range][this->forecasts->forecast_index[date]];


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

                float value = forecast[iy * NLON + ix];

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
            1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f, 220, 220, false);
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

        float *observation_values = this->observations->GetInterpolatedValues(date, accumulation_range, init_lat, init_lon, end_lat, end_lon, 220, 220);

        forecast_region[analog_index]->observation_values = observation_values;

        // accumulate the observation value
        for (int i = 0; i < 220 * 220; i++) {
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
    for (int i = 0; i < 220 * 220; i++){
        mean_observation[i] /= N_ANALOGS_PER_LAT_LON;
        min_mean_observation_value = fminf(mean_observation[i], min_mean_observation_value);
        max_mean_observation_value = fmaxf(mean_observation[i], max_mean_observation_value);
    }


    // Interpolate the addition of all forecasts
    float* interpolated_mean = Interpolate3(mean_analog, ANALOG_DIMENSION + 1, ANALOG_DIMENSION + 1,
        1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f,
        1.0f, 1.0f, ANALOG_DIMENSION + 1.0f, ANALOG_DIMENSION + 1.0f, 220, 220, false);
    delete(mean_analog);

    // Divide the addition of all forecasts by the amount of forecast and calculate max and min of the mean.
    float min_mean_forecast_value = MAX_FLOAT;
    float max_mean_forecast_value = 0;
    for (int i = 0; i < 220 * 220; i++){
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

    probability_map_height = ((NLAT - 1) * (OBSERVATIONS_PER_PANEL-1)) + 1;
    probability_map_width = ((NLON - 1) * (OBSERVATIONS_PER_PANEL-1)) + 1;

    int analog_dimension = ANALOG_DIMENSION * ANALOG_DIMENSION;
    int analog_observations_size = probability_map_width * probability_map_height * analog_dimension * (N_ANALOGS_PER_LAT_LON + 1); // Note that we have +1 since the first one will be the distance to the center of the analog area for that point in that dimension.
    float* probability_points = new float[analog_observations_size]; // intermediate to generate the probability map


    // one probability map per threshold per accumulation range
    probability_map = new float*[threshold_ranges.size() * accumulation_ranges.size()];


    // Iterate the accumulation range
    for (int accumulation_range_index = 0; accumulation_range_index < this->accumulation_ranges.size(); accumulation_range_index++)
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

                    // get the observation
                    float *observation_values = this->observations->GetInterpolatedValues(local_analogs[analog_index].date, accumulation_range_index, init_lat, init_lon, end_lat, end_lon, observations_in_region_width, observations_in_region_width);

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

        int threshold_index = 0;
        for (std::list<ThresholdRange>::const_iterator iterator = threshold_ranges.begin(), end = threshold_ranges.end(); iterator != end; ++iterator)
        {
            ThresholdRange range = *iterator;

            cout << range.from << "   " << endl;

            //float threshold = threshold_index * step;

            size_t probability_map_index = accumulation_range_index * threshold_ranges.size() + threshold_index;

            probability_map[probability_map_index] = new float[(probability_map_width)*(probability_map_height)];
            memset(probability_map[probability_map_index], 0, (probability_map_width)*(probability_map_height)* sizeof(float));

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
                    {

                        // retroeve the observations for the point in the current dimension
                        int pp_index = CPU_PROBABILITY_POINT(x, y, dimension);
                        float *point_observations = &probability_points[pp_index];

                        // obtain the weight and normalize it
                        float weight = point_observations[0];
                        float normalized_weight = 0;
                        if (weight_sum != 0){
                            normalized_weight = weight / weight_sum;                          
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

                        // acumulate the probability using the dimensions weight.
                        v += probability *normalized_weight;
                    }

                    max_value = fmaxf(v, max_value);
                    min_value = fminf(v, min_value);

                    probability_map[probability_map_index][y*probability_map_width + x] = v;
                }
            }

            //stats
            this->stats->max_probabilistic_forecast[accumulation_range_index] = max_value;
            this->stats->min_probabilistic_forecast[accumulation_range_index] = min_value;

            PCT::SelectScale(-7, true);

            // Grabamos los valores interpolados
            float* interpolated_values = Interpolate3(probability_map[probability_map_index], probability_map_width, probability_map_height,
                1.0f, 1.0f, (float)probability_map_width, (float)probability_map_height,
                1.0f, 1.0f, (float)probability_map_width, (float)probability_map_height, probability_map_height * 3, probability_map_width * 3, true);
            WriteImage(PCT::probabilistic_forecast_schema, interpolated_values, probability_map_width * 3, probability_map_height * 3, albero_images_path + "forecast8_prob_map_" + to_string(threshold_index) + "_" + to_string(accumulation_range_index) + ".png", 0, 100);
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
    if (this->analogs != NULL) delete(analogs); analogs = NULL;
    if (this->stats!= NULL) delete(stats); stats = NULL;

    this->current_date = current_date;

    cout << ":: Initializing..." << endl;

    // Create accumulation ranges, just for now we hardcode them.
    AccumulationRange a1(0,1);  // [0, 24)
    AccumulationRange a2(1,2);  // [24, 48)
    AccumulationRange a3(2,3); // [48, 72)
    this->accumulation_ranges.push_back(a1);
    this->accumulation_ranges.push_back(a2);
    this->accumulation_ranges.push_back(a3);

    this->stats = new Statistics(accumulation_ranges.size());

    // Read the observations, once on the life cycle.
    if(this->observations == NULL){
        cout << "[] ObservationReader::Init()" << endl;
        this->observations = new ObservationReader();
        this->observations->accumulation_ranges = this->accumulation_ranges;
        this->observations->Init();
    }

    // Read Forecasts
    if (forecasts == NULL) {
        forecasts = new ForecastReader();
        this->forecasts->accumulation_ranges = this->accumulation_ranges;
        forecasts->Initialize(reforecast_file_path);
    }

    // Read the historic forecasts
    cout << ":: Reading historic forecasts..." << endl;
    ReadHistoricForecasts(current_date);


    // Calculate Analogs
    cout << ":: Calculating analogs..." << endl;
    CalculateAnalogs();

    cout << ":: Calculating probabilistic forecast..." << endl;
    CalculateProbabilisticForecast();


    // We'll do one last thing, which is calculating max and min for the observed data on the accumulation ranges
    for (int range = 0; range < this->accumulation_ranges.size(); range++){

        float *values = this->observations->GetRawValues(current_date * 100, range);

        float min_value = MAX_FLOAT;
        float max_value = 0;

        for (int i = 0; i < this->observations->file_lats * this->observations->file_lons; i++){
            float v = values[i];
            max_value = fmaxf(max_value, v);
            min_value = fminf(min_value, v);
        }

        this->stats->min_observation[range] = min_value;
        this->stats->max_observation[range] = max_value;

        delete(values);
    }

    // Check if the requested date has observations.
    this->stats->current_date_has_observations = this->observations->HasDate(current_date);

    return 0;
}

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
            ", \"min_probabilistic_forecast\":\"" + to_string(this->min_probabilistic_forecast[i]) + "\"" +
            ", \"current_date_has_observations\":\"" + to_string(this->current_date_has_observations) + "\""
            "}";

        if (i < accumulation_ranges-1) result += ",";
    }

    result += "]";

    return result;
}
