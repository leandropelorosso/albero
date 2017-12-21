#include <string>    
#include <math.h>
#include <map>
#include <vector>
#include <list>
#include "helper.h"

using namespace std;

#pragma once
class ForecastReader
{
public:
	ForecastReader();
	~ForecastReader();

	/* Netcdf Dimensions*/
	size_t NLAT;
	size_t NLON;
	size_t NTIME;
	size_t NFHOUR;

	/* These program variables hold the latitudes and longitudes. */
	float *lats = NULL;
	float *lons= NULL;
	
    // some statistics
    float min_value = 999999;
    float max_value = -999999;

    /* list of days stored on the netcdf file */
	int *forecast_days = NULL;

    std::vector<AccumulationRange> accumulation_ranges; // the accumulation ranges

    // the forecasts by range [range_index]=>[N_DAYSxNLATxNLON]
    // N_DAYS is the amount of days picked for a given date,
    // it is not all the days on the netcdf file.
    float** forecasts_by_range = NULL;

    std::map<int, long> forecast_index;  // given the day as integer, retrieves the index of
                                         // the date on the forecast_by_range structure.
                                         // forecast_by_range[0][forecast_index[2002120800]]

    std::list<int> forecast_date; // a list of dates of the chosen forecasts inside the read window

    // Opens the netcdf file and sets some definition variables (like NLAT, NLON, etc)
	int Initialize(string filename);

    // Reads the forecast, by range, for all dates surounding the given date +-45 days for all years.
    // Grouping the forecasts by accumulation range. For instance, 3 ranges of 24 hs.
    int Read(int date/*, int accumulation_range_hs, int accumulation_ranges*/);
	
	// Returns the time index for a particular date.
	int GetDateIndex(int date);
	
	/* This will be the netCDF ID for the file and data variable. */
    int ncid = -1;

	/* Variable and dimensions IDs */
	int lat_varid, lon_varid, precipitation_varid, time_varid, fhour_varid, ens_varid, inttime_varid;
	int lat_dimid, lon_dimid, time_dimid, fhour_dimid, ens_dimid;

    // Imports a file and adds it to the current collection
    void ImportDay(string filename);

    // Closes netcfd file.
    void CloseFile();
};

