#include "observation_reader.h"
#include "helper.h"

#include <netcdf.h>
#include "iostream"
#include <string.h>
#include <iomanip>
#include <assert.h> 

#include <iostream>
#include <fstream>
#include <ctime>
#include <unordered_map>
#include "helper.h"
#include <math.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#if __WIN32__
#define stat64 _stat64
#endif

#ifdef WINDOWS
#define atoll(S) _atoi64(S)
#endif


/* Handle errors by printing an error message and exiting with a
* non-zero status. */
#define ERRCODE 2
#define ERR(e) {printf("ObservationReader.cpp, Error: %s\n ObservationReader.cpp", nc_strerror(e)); system("pause");}

/* The variables inside the netcdf. */
#define LAT_NAME "lat"
#define LON_NAME "lon"
#define TIME_NAME "time"
#define PRECIPITATION_NAME "cmorph_precip"
#define TIME_NAME "time"

using namespace std;

extern string cmorph_data_file;
extern string cmorph_index_file;

float *ObservationReader::GetInterpolatedValues(int date, int range_index, float init_lat, float init_lon, float end_lat, float end_lon, int amount_steps_lat, int amount_steps_lon)
{
	if (init_lat > end_lat){
		swap(init_lat, end_lat);
	}
	
	// just make sure the init and end coordinates are in the correct order.
	assert(init_lon < end_lon);
	assert(init_lat < end_lat);

	if (init_lon > end_lon || init_lat > end_lat) return NULL;

	// the start and count array we'll use to read the netcdf values
	size_t start[3], count[3];

	// the initial and end coordinates as netcfd indexes
	int init_lat_index = 0;
	int end_lat_index = 0;
	int init_lon_index = 0;
	int end_lon_index = 0;

	float bound_init_lat;
	float bound_end_lat;
	float bound_init_lon;
	float bound_end_lon;

	// the values we'll return
	float *value = NULL;
	
	// get observation filename
	string str_date = to_string(date);
	string year = str_date.substr(0, 4);
	string month = str_date.substr(4, 2);
	string day = str_date.substr(6, 2);

	int NLAT = file_lats;
	int NLON = file_lons;

	float deltaLon = (file_end_lon - file_init_lon) / (file_lons - 1);
	float deltaLat = (file_end_lat - file_init_lat) / (file_lats - 1);

	// the boundary of the required region (note that it can be smaller than the actual required region)
	bound_init_lat = (init_lat < file_init_lat) ? file_init_lat : init_lat;
	bound_end_lat = (end_lat > file_end_lat) ? file_end_lat : end_lat;
	bound_init_lon = (init_lon < file_init_lon) ? file_init_lon : init_lon;
	bound_end_lon = (end_lon > file_end_lon) ? file_end_lon : end_lon;

	// since LATS is decrescent, the calculation of the index changes
	init_lat_index = (int)floor(fabs((bound_init_lat - (file_init_lat)) / deltaLat));
	end_lat_index = (int)ceil(fabs((bound_end_lat - (file_init_lat)) / deltaLat));

	// since LONS are crescent, regular index calculation is used
	init_lon_index = (int)floor(fabs((bound_init_lon - (file_init_lon)) / deltaLon));
	end_lon_index = (int)ceil(fabs((bound_end_lon - (file_init_lon)) / deltaLon));

	bound_init_lat = file_init_lat + init_lat_index * deltaLat;
	bound_end_lat = file_init_lat + end_lat_index * deltaLat;

	bound_init_lon = file_init_lon + init_lon_index * deltaLon;
	bound_end_lon = file_init_lon + end_lon_index * deltaLon;

	// make sure the indexes have the right order, otherwise the index calculation failed.

	if (init_lon_index > end_lon_index || init_lat_index > end_lat_index) return NULL;

	assert(init_lon_index < end_lon_index);
	assert(init_lat_index < end_lat_index);


	// create the count and start array to read the netcdf.
	count[0] = 1; // AHORA VIENEN ACUMULADOS!
	count[1] = (end_lat_index - init_lat_index) + 1; // lat
	count[2] = (end_lon_index - init_lon_index) + 1; // lon
	start[0] = 0;
	start[1] = init_lat_index;
	start[2] = init_lon_index;


	float *all_hours_value_from_dat = ReadRangeFromFile(date, start, count, range_index);

	float *interpolated_values = Interpolate3(all_hours_value_from_dat, (int)count[2], (int)count[1], bound_init_lat, bound_init_lon, bound_end_lat, bound_end_lon,
		init_lat, init_lon, end_lat, end_lon, amount_steps_lat, amount_steps_lon, true);

	delete(all_hours_value_from_dat);

	return interpolated_values;
}

// Reads one line of the input stream, and retrieves key and value.
bool ReadLine(std::ifstream* input, std::string* key, std::string* value){
	std::string line;
	// if there is another line to read
	if (std::getline(*input, line)){
		// split the line and assign values
		size_t pos = line.find_first_of('=');
		*value = line.substr(pos + 1),
		*key = line.substr(0, pos);
		return true;
	}else 
		return false;
}

// Read float value ignoring key.
float ReadFloat(std::ifstream* input){
	std::string line;
	std::getline(*input, line);
	size_t pos = line.find_first_of('=');
	return stof(line.substr(pos + 1));
}

off_t fsize(const char *filename) {
	struct stat st;

	if (stat(filename, &st) == 0)
		return st.st_size;

	return -1;
}


void ObservationReader::Init(){
	
    this->file_lats = 154; // TODO : Retrieve from index file.
    this->file_lons = 106;

	// Read Data File
	cout << "[] Reading " << cmorph_data_file;

	FILE *file = fopen(cmorph_data_file.c_str(), "rb");
	if (file == NULL){
		cout << "Error reading dat_file" << endl;
	}

	off_t pos = fsize(cmorph_data_file.c_str());

    this->file_data = new float[pos / sizeof(float)];
    size_t read = fread(this->file_data, sizeof(float), pos / (sizeof(float)), file);

	fclose(file);

	// Read Index File

	std::ifstream input(cmorph_index_file);
	std::string line;

	std::string* key = new std::string();
	std::string* value = new std::string();

	// read file lat/lon configuration
	file_init_lat = ReadFloat(&input);
	file_end_lat = ReadFloat(&input);
	file_init_lon = ReadFloat(&input) - 360;
	file_end_lon = ReadFloat(&input) - 360;

	delta_lon = (file_end_lon - file_init_lon) / (file_lons - 1);
	delta_lat = (file_end_lat - file_init_lat) / (file_lats - 1);

	// iterate file index lines
	while (ReadLine(&input, key, value)) {
		// store index
		days_index_in_file.insert(std::make_pair(stoi(*key), atoll(value->c_str())));
	}

	delete(key);
	delete(value);

	input.close();

	cout << " OK" << endl;
}



float *ObservationReader::GetInterpolatedValuesLambert(int date, int range_index, int init_lat_pixel, int init_lon_pixel, int end_lat_pixel, int end_lon_pixel, int amount_steps_lat, int amount_steps_lon, int zoom)
{
	float init_lat = (float)getLatFromPixel(init_lat_pixel, zoom);
	float end_lat = (float)getLatFromPixel(end_lat_pixel, zoom);
	float init_lon = (float)getLngFromPixel(init_lon_pixel, zoom);
	float end_lon = (float)getLngFromPixel(end_lon_pixel, zoom);

	// LEA!!!!
	// pasale los pixeles, luego convertilos a latitud y longitud, y usa esos valores para calcular
	// el bounding :)
	// luego esos pixeles se los pasas al interpolate8 y ya.

	// just make sure the init and end coordinates are in the correct order.
	assert(init_lon < end_lon);
	assert(init_lat < end_lat);

	if (init_lon > end_lon || init_lat > end_lat) return NULL;

	// the start and count array we'll use to read the netcdf values
	size_t start[3], count[3];

	// the initial and end coordinates as netcfd indexes
	int init_lat_index = 0;
	int end_lat_index = 0;
	int init_lon_index = 0;
	int end_lon_index = 0;

	float bound_init_lat;
	float bound_end_lat;
	float bound_init_lon;
	float bound_end_lon;

	// the values we'll return
	float *value = NULL;

	// get observation filename
	string str_date = to_string(date);
	string year = str_date.substr(0, 4);
	string month = str_date.substr(4, 2);
	string day = str_date.substr(6, 2);

	int NLAT = file_lats;
	int NLON = file_lons;

	// the boundary of the required region (note that it can be smaller than the action required region)
	bound_init_lat = (init_lat < file_init_lat) ? file_init_lat : init_lat;
	bound_end_lat = (end_lat > file_end_lat) ? file_end_lat : end_lat;
	bound_init_lon = (init_lon < file_init_lon) ? file_init_lon : init_lon;
	bound_end_lon = (end_lon > file_end_lon) ? file_end_lon : end_lon;

	// the calculation of the indexes
	init_lat_index = (int)floor(fabs((bound_init_lat - (file_init_lat)) / delta_lat));
	end_lat_index = (int)ceil(fabs((bound_end_lat - (file_init_lat)) / delta_lat));

	init_lon_index = (int)floor(fabs((bound_init_lon - (file_init_lon)) / delta_lon));
	end_lon_index = (int)ceil(fabs((bound_end_lon - (file_init_lon)) / delta_lon));

	// the calculation of the real bounding
	bound_init_lat = file_init_lat + init_lat_index * delta_lat;
	bound_end_lat = file_init_lat + end_lat_index * delta_lat;

	bound_init_lon = file_init_lon + init_lon_index * delta_lon;
	bound_end_lon = file_init_lon + end_lon_index * delta_lon;

	// make sure the indexes have the right order, otherwise the index calculation failed.

	if (init_lon_index > end_lon_index || init_lat_index > end_lat_index) return NULL;

	// create the count and start array to read the netcdf.
	count[0] = 1; // AHORA VIENEN ACUMULADOS!
	count[1] = (end_lat_index - init_lat_index) + 1; // lat
	count[2] = (end_lon_index - init_lon_index) + 1; // lon
	start[0] = 0;
	start[1] = init_lat_index;
	start[2] = init_lon_index;

	float *all_hours_value_from_dat = ReadRangeFromFile(date, start, count, range_index);

	float *interpolated_values = Interpolate8(all_hours_value_from_dat, (int)count[2], (int)count[1], bound_init_lat, bound_init_lon, bound_end_lat, bound_end_lon,
		init_lat_pixel, init_lon_pixel, end_lat_pixel, end_lon_pixel, amount_steps_lat, amount_steps_lon, zoom);

	delete(all_hours_value_from_dat);

	return interpolated_values;
}


float *ObservationReader::GetRawValues(int date, int range_index){

	size_t count[3];
	size_t start[3];
	
	// create the count and start array to read the netcdf.
	count[0] = 1; // AHORA VIENEN ACUMULADOS!
    count[1] = this->file_lats;
    count[2] = this->file_lons;
	start[0] = 0;
	start[1] = 0;
	start[2] = 0;

	float *all_hours_value_from_dat = ReadRangeFromFile(date, start, count, range_index);

	return all_hours_value_from_dat;

}


ObservationReader::~ObservationReader(){
	cout << "[] Cleaning Observation Reader...";
	delete(file_data);
	cout << " OK" << endl;
}

// returns true if the date exists in the observations collection
bool ObservationReader::HasDate(int date){
    std::unordered_map<int,size_t>::const_iterator got = days_index_in_file.find (date/100);
    return ( got != days_index_in_file.end());
}

float* ObservationReader::ReadRangeFromFile(int date, size_t start[], size_t count[], int range_index)
{
	// amount of (6 hour) slots per range, for instance:
	// a 24 hours range has 4 times
	// a 6 hours range has 1 times
	// a 48 hours range has 8 times

	// amounts of units a date has (4 because it is every 6 hours, 24/6 = 4)
	int times_per_day = 4;

	// Where we'll store the result
	float* result = new float[count[1] * count[2]];
	memset(result, 0, count[1] * count[2] * sizeof(float));

	float* lon_hours = new float[count[2] * times_per_day];  // the "units in range" for each of the required longitudes in one latitude (read from file)

	// lets get the start of the day in the file
    size_t date_index = 0;

    std::unordered_map<int,size_t>::const_iterator got = days_index_in_file.find (date/100);

     if ( got == days_index_in_file.end() )
       std::cout << date/100 <<  " not found";
     else
        date_index = ((size_t)got->second) / sizeof(float);

    // Iterate the exact times we need for the required range
    for (int time = this->accumulation_ranges[range_index].from; time < this->accumulation_ranges[range_index].to; time++)
    {
		// based on the time, and the date index, lets find the current day index
		size_t day_index = date_index + ((time / times_per_day) * (file_lats * file_lons * times_per_day));

		for (size_t lat = start[1]; lat < start[1] + count[1]; lat++){

			// find the point where we'll read from
			size_t file_start = (day_index)+((lat * file_lons * times_per_day) + start[2] * times_per_day); // where to start reading

			for (int lon = 0; lon < count[2]; lon++)
			{

				size_t result_index = (lat - start[1]) * count[2] + lon;
				size_t lon_hour_index = lon * times_per_day + time;
				float value = (file_data[file_start + lon_hour_index]);

				result[result_index] += (value * 3);
			}
	
        }

	}
	delete(lon_hours);
	return result;
}
