/*
 * This is the class in charge of importing a day into
 * the main reforecast netcdf collection. It will import the source data for the
 * latitude and longitude range defined on the collection, and add
 * the new day into it.
 *
*/

#include "forecast_importer.h"
#include <stdio.h>
#include <netcdf>
#include "iostream"
#include <fstream>
#include <string>
#include <cstring>
#include <list>
#include <map>
#include <time.h>

/*
sudo apt-get install libnetcdf-c++4-dev
sudo apt-get install libnetcdf-c++
*/

using namespace std;
using namespace netCDF;
using namespace netCDF::exceptions;

/* Handle errors by printing an error message and exiting with a
* non-zero status. */
#define ERRCODE 2
#define ERR(e) {printf("ForecastReader.cpp. Error: %s\n", nc_strerror(e)); system("pause"); return;}

// Returns the index of a given value inside the array
int GetIndex(float* array, int size, float value){
    for(int i=0; i<size; i++){
        if(array[i]==value) return i;
    }
}

// converts a date of format 2016.03.27 00:00:00 UTC to struct tm
struct tm dateToTMDate(string date){
    struct tm tm;
    strptime(date.c_str(), "%Y.%m.%d 00:00:00 UTC", &tm);
    tm.tm_sec = 0;
    tm.tm_min = 0;
    tm.tm_hour = 0;
    return tm;
}

time_t mktimeUTC(struct tm* timeinfo)
{
    // *** enter in UTC mode
    char* oldTZ = getenv("TZ");
    putenv("TZ=UTC");

    // ***

    time_t ret = mktime ( timeinfo );

    // *** Restore previous TZ
    if(oldTZ == NULL)
    {
        putenv("TZ=");
    }
    else
    {
        char buff[255];
        sprintf(buff,"TZ=%s",oldTZ);
        putenv(buff);
    }
    //_tzset();
    // ***

    return ret;
}

// The Forecast Importer Class.
ForecastImporter::ForecastImporter()
{    
}

ForecastImporter::~ForecastImporter()
{
}

// Imports day_file into the collection file.
void ForecastImporter::Import(std::string day_file, std::string collection_file){

    try{
        cout << "[] Opening destination file \"" << collection_file << "\"." << endl;

        // Open Destination (the reforecast collection) file.
        NcFile dst_file(collection_file,  NcFile::write);

        cout << "[] Retrieving file information." << endl;

        // Get destination Latitudes
        NcDim dst_dim_lat = dst_file.getDim("lat");
        float* dst_lat = new float[dst_dim_lat.getSize()];
        dst_file.getVar("lat").getVar(dst_lat);

        // Get destination Longitudes
        NcDim dst_dim_lon = dst_file.getDim("lon");
        float* dst_lon = new float[dst_dim_lon.getSize()];
        dst_file.getVar("lon").getVar(dst_lon);

        // Get destination Forecast Hours
        NcDim dst_dim_fhour = dst_file.getDim("fhour");
        float* dst_fhour = new float[dst_dim_fhour.getSize()];
        dst_file.getVar("fhour").getVar(dst_fhour);

        // Get destination Time
        NcDim dst_dim_time = dst_file.getDim("time");
        float* dst_time = new float[dst_dim_time.getSize()+1];
        dst_file.getVar("time").getVar(dst_time);

        cout << "[] Opening source file \"" << day_file << "\"." << endl;

        // Open Source (the single day) file.
        NcFile src_file(day_file,  NcFile::read);

        cout << "[] Retrieving file information." << endl;

        // Get Source Latitudes
        NcDim src_dim_lat = src_file.getDim("latitude");
        float* src_lat = new float[src_dim_lat.getSize()];
        src_file.getVar("latitude").getVar(src_lat);

        // Get Source Longitudes
        NcDim src_dim_lon = src_file.getDim("longitude");
        float* src_lon = new float[src_dim_lon.getSize()];
        src_file.getVar("longitude").getVar(src_lon);

        // Get Source Time
        NcDim src_dim_time = src_file.getDim("time");

        // Get reference time
        string src_reference_date;
        src_file.getVar("time").getAtt("reference_date").getValues(src_reference_date);
        tm src_reference_tm_date = dateToTMDate(src_reference_date);

        cout << "[] File reference date: ";
        cout << (src_reference_tm_date.tm_mon + 1) << "/" << src_reference_tm_date.tm_mday << "/" << (src_reference_tm_date.tm_year+1900) << endl;

        // Get all hours
        float* src_fhour = new float[src_dim_time.getSize()];
        src_file.getVar("time").getVar(src_fhour);

        // We know we need times on the form: 0, 3, 6, 9, etc, so let's calculate that.
        long src_first_fhour =  src_fhour[0];
        for(int i=0; i< src_dim_time.getSize(); i++) {
            src_fhour[i] = (int)round((src_fhour[i]-src_first_fhour) / 3600);
        }

        cout << "[] Calculating source reading bounds." << endl;

        // Get Source Edges indexes on Source file, according to Destination Edges
        size_t lat_init_index = GetIndex(src_lat, src_dim_lat.getSize(), dst_lat[0]);
        size_t lat_end_index = GetIndex(src_lat, src_dim_lat.getSize(), dst_lat[dst_dim_lat.getSize()-1]);

        size_t lon_init_index = GetIndex(src_lon, src_dim_lon.getSize(), dst_lon[0]);
        size_t lon_end_index = GetIndex(src_lon, src_dim_lon.getSize(), dst_lon[dst_dim_lon.getSize()-1]);

        size_t lat_count = lat_end_index - lat_init_index + 1;
        size_t lon_count = lon_end_index - lon_init_index + 1;

        cout << "[] Reading source information." << endl;

        // Now we retrieve the rectangle [[lat_init_index, lon_init_index], [lat_end_index, lon_end_index]] from the source, for each hour we need.
        std::vector<size_t> start(3);
        start[0] = 0; // fhour
        start[1] = lat_init_index;
        start[2] = lon_init_index;

        std::vector<size_t> count(3);
        count[0] = 1;
        count[1] = lat_count;
        count[2] = lon_count;

        // The new day were we will store the new information.
        float *new_day = new float[dst_dim_lat.getSize() * dst_dim_lon.getSize() * dst_dim_fhour.getSize()];

        // For each hour on the destination
        for(int i=0; i<dst_dim_fhour.getSize(); i++){

            // Get hour index on source file.
            start[0] = GetIndex(src_fhour, src_dim_time.getSize(), dst_fhour[i]);

            // Read the hour information
            src_file.getVar("APCP_surface").getVar(start, count, &new_day[lat_count * lon_count * i]);
        }

        // Calculate new time variable value

        cout << "[] Calculating minutes form 1800/01/01." << endl;

        tm start_of_time_tm_date = dateToTMDate("1800.01.01 00:00:00 UTC");
        time_t t1 = mktimeUTC(&start_of_time_tm_date);
        time_t t2 = mktimeUTC(&src_reference_tm_date);
        long seconds = difftime(t2,t1);
        long minutes = seconds / 3600;

        cout << "[] Minutes: " << minutes << endl;

        cout << "[] Verifying date does not alrady exist." << endl;

        for(int i=0; i < dst_dim_time.getSize(); i++){
            if(dst_time[i] == minutes) {
              cout << "[] Date already exists." << endl;
              cout << "[] Abort." << endl;
              return;
            }
        }

        cout << "[] Updating Time Dimension." << minutes << endl;

        // Save Time Variable

        std::vector<size_t> start_time(1);
        std::vector<size_t> count_time(1);

        cout << "[] Before: " << dst_dim_time.getSize() << " Days." << endl;

        start_time[0] = dst_dim_time.getSize();
        count_time[0] = 1;

        long *times = new long[1];
        times[0] = minutes;
        dst_file.getVar("time").putVar(start_time, count_time, times);

        cout << "[] After: " << dst_dim_time.getSize() << " Days." << endl;

        // Add the new day to the reforecast collection

        std::vector<size_t> start_save_data(4);
        std::vector<size_t> count_save_data(4);

        start_save_data[0] = dst_dim_time.getSize() - 1; // time
        start_save_data[1] = 0; //dst_dim_fhour.getSize(); // fhour
        start_save_data[2] = 0; //dst_dim_lat.getSize(); // lat
        start_save_data[3] = 0; //dst_dim_lon.getSize(); // lon

        count_save_data[0] = 1; // time
        count_save_data[1] = dst_dim_fhour.getSize(); // fhour
        count_save_data[2] = dst_dim_lat.getSize(); // lat
        count_save_data[3] = dst_dim_lon.getSize(); // lon

        cout << "[] Writing Total_precipitation Variable." << endl;

        dst_file.getVar("Total_precipitation").putVar(start_save_data, count_save_data, new_day);

        cout << "[] Cleaning Resources." << endl;

        // Clean after ourselves.

        delete(dst_lat);
        delete(dst_lon);
        delete(dst_fhour);
        delete(dst_time);
        delete(src_lat);
        delete(src_fhour);
        delete(new_day);
        delete(times);

        cout << "[] Done." << endl;

    }
    catch (const std::exception& e){
        cout << "There was an error importing new Netcdf information." << endl << e.what() << endl;
        cout << "[] Error." << endl;
    }
}



