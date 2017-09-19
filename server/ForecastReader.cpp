#include "ForecastReader.h"
#include <stdio.h>
#include <netcdf.h>
#include "iostream"
#include <fstream>
#include <string>
#include <cstring>
#include <list>
#include <map>

using namespace std;

/* Handle errors by printing an error message and exiting with a
* non-zero status. */
#define ERRCODE 2
#define ERR(e) {printf("ForecastReader.cpp. Error: %s\n", nc_strerror(e)); system("pause"); return(0);}
#define LAT_NAME "lat"
#define LON_NAME "lon"
#define TIME_NAME "time"
#define PRECIPITATION_NAME "Total_precipitation"
#define TIME_NAME "time"
#define FHOUR_NAME "fhour"
#define INTTIME_NAME "intTime"

ForecastReader::ForecastReader()
{
}


ForecastReader::~ForecastReader()
{
	delete(forecast_days);
	delete(lats);
    delete(lons);
    if(forecasts_by_range) {
        for(int i=0; i<this->accumulation_ranges;i++){
            delete(forecasts_by_range[i]);
        }
        delete(forecasts_by_range);
    }
}

// converts a date of format YYYYMMDD00 to struct tm
struct tm intToTMDate(string date){
    struct tm tm;
    strptime(date.c_str(), "%Y%m%d00", &tm);
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 0;
    return tm;
}

int ForecastReader::Initialize(string filename){

    // Slean auxiliary structures
    this->forecast_date.clear();
    this->forecast_index.clear();

	/* Loop indexes, and error handling. */
	int retval;

	/* Open the file. NC_NOWRITE tells netCDF we want read-only access
	* to the file.*/
	if ((retval = nc_open(filename.c_str(), NC_NOWRITE, &ncid)))
		ERR(retval);

	/* Get the varids of the latitude and longitude coordinate
	* variables. */
	if ((retval = nc_inq_varid(ncid, LAT_NAME, &lat_varid)))
		ERR(retval);
	if ((retval = nc_inq_varid(ncid, LON_NAME, &lon_varid)))
		ERR(retval);
	if ((retval = nc_inq_varid(ncid, TIME_NAME, &time_varid)))
		ERR(retval);
	if ((retval = nc_inq_varid(ncid, PRECIPITATION_NAME, &precipitation_varid)))
		ERR(retval);
	if ((retval = nc_inq_varid(ncid, FHOUR_NAME, &fhour_varid)))
		ERR(retval);
	if ((retval = nc_inq_varid(ncid, INTTIME_NAME, &inttime_varid)))
		ERR(retval);

	/* Get the varids of the latitude and longitude coordinate
	* dimensions. */
	if ((retval = nc_inq_dimid(ncid, LAT_NAME, &lat_dimid)))
		ERR(retval);
	if ((retval = nc_inq_dimid(ncid, LON_NAME, &lon_dimid)))
		ERR(retval);
	if ((retval = nc_inq_dimid(ncid, TIME_NAME, &time_dimid)))
		ERR(retval);
	if ((retval = nc_inq_dimid(ncid, FHOUR_NAME, &fhour_dimid)))
		ERR(retval);

	/* Get the dimensions size */
	nc_inq_dimlen(ncid, lat_dimid, &NLAT);
	nc_inq_dimlen(ncid, lon_dimid, &NLON);
	nc_inq_dimlen(ncid, time_dimid, &NTIME);
    nc_inq_dimlen(ncid, fhour_dimid, &NFHOUR);

	/* These program variables hold the latitudes and longitudes. */
	lats = new float[NLAT];
	lons = new float[NLON];
	
	/* dado un time del netcdf de reforecasts, devuelve un int con la fecha del día. */
	forecast_days = new int[NTIME];

	/* Read the coordinate variable data. */
	if ((retval = nc_get_var_float(ncid, lat_varid, &lats[0])))
		ERR(retval);
	if ((retval = nc_get_var_float(ncid, lon_varid, &lons[0])))
		ERR(retval);
	/* Read forecast time */
	if ((retval = nc_get_var_int(ncid, inttime_varid, forecast_days)))
		ERR(retval);

    for (int i = 0; i < NLON; i++) lons[i] -= 360/*+0.125f*/;

	return 1;
}

// Returns the time index for a particular date.
int ForecastReader::GetDateIndex(int date){

    date *= 100; // because dates end with 00 (for instance: 2002120800)
	for (int i = (int)NTIME - 1; i >= 0; i--){
		if (forecast_days[i] == date){
			return i;
		}
	}
	return -1;
}


// Tells us if the TM struct X is higher or equal than Y, month and day.
bool operator >=(const tm& x, const tm& y) {
   return (
           x.tm_mon > y.tm_mon ||
           x.tm_mon == y.tm_mon && x.tm_mday >= y.tm_mday);
}
// Tells us if the TM struct X is higher than Y, month and day.
bool operator >(const tm& x, const tm& y) {
   return (
           x.tm_mon > y.tm_mon ||
           x.tm_mon == y.tm_mon && x.tm_mday > y.tm_mday);
}

// Returns true if the date has month and day between the days and month of the limits.
bool DateInWindow(struct tm min, struct tm max, struct tm date){
    if(max>=min){
        return date>=min && max>=date;
    }else{
        return !(date>max && min>date);
    }
}

// Reads the forecast, by range, for all dates surounding the given date +-45 days for all years.
// Grouping the forecasts by accumulation range. For instance, 3 ranges of 24 hs.
// accumulation_range_hs: size of one range in hours (example 24hs)
// accumulation_ranges: amount of ranges (for instance, 3, for a total forecast of 72hs)
int ForecastReader::Read(int date, int accumulation_range_hs, int accumulation_ranges){

    // save the accumulation ranges count, for later use
    this->accumulation_ranges = accumulation_ranges;

    size_t const NDIMS = 4;
    int times_in_range = accumulation_range_hs / 6; // how many times do we have in a range? Each time are 6 hours.

    // Get dates limit
    // the current date
    string current_date = to_string(date);
    struct tm current_tm = intToTMDate(current_date);

    // substract and add 45 days
    time_t min_window_time_t = timegm(&current_tm) - 45 * (24 * 60 * 60);
    time_t max_window_time_t = timegm(&current_tm) + 45 * (24 * 60 * 60);

    // get tm structures from limit
    struct tm min_window_tm = *localtime ( &min_window_time_t );
    struct tm max_window_tm = *localtime ( &max_window_time_t );


    cout << (current_tm.tm_year+1900) << "/" << (current_tm.tm_mon+1) << "/" << current_tm.tm_mday << endl;
    cout << (min_window_tm.tm_year+1900) << "/" << (min_window_tm.tm_mon+1) << "/" << min_window_tm.tm_mday << endl;
    cout << (max_window_tm.tm_year+1900) << "/" << (max_window_tm.tm_mon+1) << "/" << max_window_tm.tm_mday << endl;


    // iterate all the days on the file and check if it falls inside the time windows we need.
    // here we will retrieve a list of days indexes of days we will need to read from the netcdf.
    std::list<int> days;

    for (int i = 0; i< (int)NTIME; i++){
        // get the date as an int (for instance: 2012051000), and convert date to TM structure
        string local_date = to_string(forecast_days[i]);
        struct tm local_tm = intToTMDate(local_date);
        // now using the TM structure, we can see if it falls inside the range.
        // (for instance, 15 Jan - 1 March)
        if(DateInWindow(min_window_tm, max_window_tm, local_tm)){
            days.push_back(i); // save the index on the list of days we will retrieve
        }
    }

    // allocate the forecasts by range structure
    forecasts_by_range = new float*[accumulation_ranges];
    for(int range=0; range<accumulation_ranges;range++){
        forecasts_by_range[range] = new float[NLAT * NLON * days.size()];
        memset(forecasts_by_range[range], 0, sizeof(float) * NLAT * NLON * days.size());
    }

    // display how many days we will read (how many NTIME)
    cout << ">> Reading " << days.size() << " days... " << endl;

    // the auxiliary forecast we will use to read one day from the netcdf.
    float *forecast = new float[NFHOUR * NLAT * NLON];

    // now we will do the same, but we will read the forecast from the netcdf.
    // day_index: the index of the day in the netcdf

    int loop_index=0; // so we know how many days we printed so far

    for (int day_index : days) {

        // What are we going to read the forecast, just the one day, from the netcdf.
        size_t start[NDIMS], count[NDIMS];

        count[0] = 1; // time
        count[1] = NFHOUR; // hour
        count[2] = NLAT; // lat
        count[3] = NLON; // lon
        start[0] = day_index;
        start[1] = 0;
        start[2] = 0;
        start[3] = 0;

        // Let's read the current forecast (the one on the i-th day)
        int retval;
        if ((retval = nc_get_vara_float(ncid, precipitation_varid, start, count, forecast)))
            ERR(retval);

        // get the day as an integer (for instance: 2012051000)
        int day = forecast_days[start[0]];

        // get the index for the start of the day for which we will write the accumulation, and associate with the forecast date.
        long write_index =  (loop_index * NLAT * NLON);
        this->forecast_index[day] = write_index;
        // add the forecast date to the list of days
        this->forecast_date.push_back(day);

        cout << "Forecast for Date:" << day << " | " << write_index << endl;


        // forecast contains one forecast for multiple hours (every 6 hours), so we need to add by accumulation group
        for (int range = 0; range < accumulation_ranges; range++){

            // (remember, Total precipitation (kg m?2, i.e., mm) in last 6?h period (00, 06, 12, 18 UTC) or in last 3?h period(03, 09, 15, 21 UTC)[Y])
            int hour_start = range * times_in_range + 1;
            int hour_end = hour_start + times_in_range;

            for (int hour = hour_start; hour < hour_end; hour ++ )
            {
                // get the start index for that hour in the forecast from the netcd
                int read_index = (hour * NLAT * NLON);

                // now acumulate on the forecasts by range structure.
                for (int lat = 0; lat < NLAT; lat++){
                    for (int lon = 0; lon < NLON; lon++){
                        int i = lat*NLON + lon;
                        float value = forecast[read_index + i];
                        forecasts_by_range[range][i+write_index] += value;
                    }
                }
            }
        }
        loop_index++; // so we know how many days we printed so far
    }

    // delete the auxiliary forecast
    delete(forecast);

    return 1;

}



