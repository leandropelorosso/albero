#pragma once

#include "helper.h"
#include "ForecastReader.h"
#include <list>
#include "ColorSchema.h"
#include "map"

// When initializing Albero2 for a particular date, this struct will be filled and returned, 
// to be able to display minimun and maxumun values on the client's side.
// Note that we are storing this information for each accumulation range
class Statistics{
	
public:
	
	float* min_numerical_forecast, *max_numerical_forecast; // numerical forecast in acculumation ranges
	float* min_mse, *max_mse; // mse in accumulation ranges
	float* min_probabilistic_forecast, *max_probabilistic_forecast; // probabilistic forecasts in acculumation ranges
	float* min_observation, *max_observation; // observations in accumulation ranges
	int accumulation_ranges;
	Statistics(int accumulation_ranges);
	~Statistics();

	string ToJSON();
	
};


class Albero2
{
public:
	Albero2();
	~Albero2();

	int Initialize(int date);

	bool initialized = false;

	// Configuration
	int NTIME_WINDOW = 90; // The window time to look for analogs surounding the date.
	int N_ANALOGS_PER_LAT_LON =75;//10; // Amount of analogs we'll consider.
	int ANALOG_DIMENSION = 3; // The dimensions of the block that contains the central grid for which the probabilities are being calculated.
							  // for instance, if ANALOG_DIMENSION=3, then probabilites are calculated for a central block in a 3x3 block region 
							  // (which is conformed by 4x4 points). Please note that the value has to be an odd number.

	int probability_map_width = 0; // width and height of the probability map
	int probability_map_height = 0;

	float **probability_map = NULL; // the probability map for each range
	float **mean_square_error_map = NULL; // the mean square error for the whole area
	
	
	ForecastReader *forecasts= NULL; // the forecast reader
	
	float **current_forecast_by_range = NULL; // current forecast by range
	float **historic_forecast_by_range = NULL; // history forecasts by range
    std::map<int, long> *historical_forecast_index_by_range_and_date; // given the date, we can retrieve the start of the information for said day,
                                                                      // as historic_forecast_by_range_and_date[range_index][date]

	AnalogIndex *analogs = NULL; // Analogs
	
	std::list<ThresholdRange> threshold_ranges; // the threshold ranges

	// Renders the ANALOGS block of ANALOG_DIMENSION panels surounding the desired (lat, lon) block.
	AnalogsResponse* RenderAnalogForecasts(float lat, float lon, int fhour);
	
	int nAccumulationRanges; // amount of accumulation ranges, for now, of 24 hours.
	
	int current_date;

	//private:

	RegionForecastCollection* GetCurrentForecast(float lat, float lon, int fhour);
	RegionForecastCollection* GetAnalogForecasts(float lat, float lon, int fhour);

	// Reads the historic forecasts from the forecasts netcdf.
	int ReadHistoricForecasts(int current_date);

	// Calculate and stores analogs for each point of the netcdf grid.
	int CalculateAnalogs();

	// Calculates probabilistic forecast.
	int CalculateProbabilisticForecast();

	// Some variables from the historic forecast database.
	int NFHOUR;
	int NLAT;
	int NLON;
	int NTIME;
	int NYEARS;

	Statistics *stats = NULL;


};

