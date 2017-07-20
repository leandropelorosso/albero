#include <string>    

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
	size_t NYEARS;

	/* These program variables hold the latitudes and longitudes. */
	float *lats = NULL;
	float *lons= NULL;
	
	/* dado un time del netcdf de reforecasts, devuelve un int con la fecha del día. */
	int *forecast_days = NULL;

	int Initialize(string filename);
	
	// Returns the time index for a particular date.
	int GetDateIndex(int date);
	
	/* This will be the netCDF ID for the file and data variable. */
	int ncid;

	/* Variable and dimensions IDs */
	int lat_varid, lon_varid, precipitation_varid, time_varid, fhour_varid, ens_varid, inttime_varid;
	int lat_dimid, lon_dimid, time_dimid, fhour_dimid, ens_dimid;

};

