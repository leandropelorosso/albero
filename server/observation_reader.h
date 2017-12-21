#ifndef OBSERVATION_READER 
#define OBSERVATION_READER

#pragma once

#include <stddef.h>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "helper.h"

class ObservationReader
{
public:
    float *GetInterpolatedValues(int date, int range_index, float init_lat, float init_lon, float end_lat, float end_lon, int amount_steps_lat, int amount_steps_lon);
    float *GetInterpolatedValuesLambert(int date, int range_index, int init_lat_pixel, int init_lon_pixel, int end_lat_pixel, int end_lon_pixel, int amount_steps_lat, int amount_steps_lon, int zoom);
    float *GetRawValues(int date, int range_index);

    float* ReadRangeFromFile(int date, size_t start[], size_t count[], int range_index);
    bool HasDate(int date);

    std::unordered_map<int, size_t> days_index_in_file;
    void Init();

    size_t NTIME;

    int file_lats;
    int file_lons;
    float file_init_lon;
    float file_end_lon;
    float file_init_lat;
    float file_end_lat;
    float delta_lat;
    float delta_lon;

    FILE* file;

    float* file_data;

    std::vector<AccumulationRange> accumulation_ranges; // the accumulation ranges

    void Dispose();
};

#endif
