#ifndef OBSERVATION_READER 
#define OBSERVATION_READER

#pragma once

#include <stddef.h>
#include <fstream>
#include <unordered_map>

class ObservationReader
{
public:
	static float *GetInterpolatedValues(int date, int range_index, float init_lat, float init_lon, float end_lat, float end_lon, int amount_steps_lat, int amount_steps_lon);
	static float *GetInterpolatedValuesLambert(int date, int range_index, int init_lat_pixel, int init_lon_pixel, int end_lat_pixel, int end_lon_pixel, int amount_steps_lat, int amount_steps_lon, int zoom);
	static float *GetRawValues(int date, int range_index);

	static float* ReadRangeFromFile(int date, size_t start[], size_t count[], int range_index);


	static std::unordered_map<int, size_t> days_index_in_file;
	static void Init();
	/*
	static size_t NLAT;
	static size_t NLON;*/
	static size_t NTIME;

	static int file_lats;
	static int file_lons;
	static float file_init_lon;
	static float file_end_lon;
	static float file_init_lat;
	static float file_end_lat;
	static float delta_lat;
	static float delta_lon;

	static FILE* file;


	static float* file_data;

	static void Dispose();

};

#endif