#include "ForecastReader.h"
#include <stdio.h>
#include <netcdf.h>
#include "iostream"
#include <fstream>
#include <string>     

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
}


int ForecastReader::Initialize(string filename){


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

	// Now let's process historic forecast in groups of 90 days per year
	NYEARS = (int)((float)NTIME / 365.25f);

	// Display some information
	/*cout << "NLAT = " << NLAT << endl;
	cout << "NLON = " << NLON << endl;
	cout << "NTIME = " << NTIME << endl;
	cout << "NFHOUR = " << NFHOUR << endl;
	cout << "NYEARS = " << NFHOUR << endl;*/

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
	//for (int i = 0; i < NLAT; i++) lats[i] += 0.125f;

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