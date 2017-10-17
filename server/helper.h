#pragma once
#undef max
#undef min
#include <iostream>  
#include <limits>
#include "color_schema.h"

using namespace std;

// Max and min float values.
static const float MAX_FLOAT = std::numeric_limits<float>::max();
static const float MIN_FLOAT = std::numeric_limits<float>::min();

// Lat-Pixel helpers.
double getLatFromPixel(int x, int zoom);
double getLngFromPixel(int y, int zoom);
int getPixelbyLat(double lat, int zoom);
int getPixelbyLng(double lng, int zoom);

float *Interpolate3(float *input, int input_width, int input_height, float input_init_lat, float input_init_lon, float input_end_lat, float input_end_lon, float init_lat, float init_lon, float end_lat, float end_lon, int amount_steps_lat, int amount_steps_lon, bool flip_vertical);
float *Interpolate8(float *input, int input_width, int input_height, float input_init_lat, float input_init_lon, float input_end_lat, float	input_end_lon, int init_lat_pixel, int init_lon_pixel, int end_lat_pixel, int end_lon_pixel, int amount_steps_lat, int amount_steps_lon, int zoom);

// Bilinear interpolation.
float BilinearInterpolation(float q11, float q12, float q21, float q22, float x1, float x2, float y1, float y2, float x, float y);

// Convertes a float* worth of numerical data to an image mapping values with colors
char* ImageFromValuesArray(ColorSchema*schema, float *input, int width, int height, int pixel_size, float min_value, float max_value);

// saves an image to a png file, from a RGB array
void WriteImage(char* input, int width, int height, string filename);

// Writes a values array to a PNG files. The given parameter are values, so they are mapped to colors before being written to the PNG.
void WriteImage(ColorSchema *schema, float* input, int width, int height, string filename, int pixel_size, float min_value, float max_value);

// Writes a values array to a PNG files. The given parameter are values, so they are mapped to colors before being written to the PNG.
void WriteImage(ColorSchema *schema, float* input, int width, int height, string filename, float min_value, float max_value);

// paints a pixel into a char array
void paint_pixel(char* img, int img_w, int img_h, int x, int y, int r, int g, int b, int a);

// paints a pixel into a char array
void paint_pixel(char* img, int img_w, int img_h, int x, int y, int r, int g, int b, int a, int pixel_size);

struct Vector2
{
	float x;
	float y;
    Vector2(){};
	Vector2(float x, float y){
		this->x = x;
		this->y = y;
	}
};

// Haversine distance between two points.
float Distance(float th1, float ph1, float th2, float ph2);
/*
struct FindAnalogsParameters{
public:
	size_t NLAT = 37;
	size_t NLON = 26;
	size_t NTIME = 11071;
	size_t NTIME_WINDOW = 90;
	size_t NFHOUR = 4;
	size_t N_ANALOGS_PER_LAT_LON = 5; // The amount of analogs per pair (lat,lon) that the kernel will retrieve.
	size_t NYEARS = 30;
};*/

/*
An index to one particular analog. The GPU will return a grid of multiple AnalogIndex per pair (Lat,Lon).
This AnalogIndex will be used to retrieve the analog forecast from the observed data.
*/
struct AnalogIndex{
public:
    int time = 0; // The day (value between 1 and NTIME)
	float mse = MAX_FLOAT;  // The mean squared error of the analog
	int lat = 0;
	int lon = 0;
	int year = 0;
	int date = 0;
};


/* Represents an analog image */
struct AnalogImage
{
public:
	int date;
	float mse;
	string filename;
};

/* The response to the analog forecasts for a point */
class AnalogsResponse{

private:
	int index; // index used to add images
	int nImages; // amount of images
public:
	float min_forecast_value; // min value in images
	float max_forecast_value; // max value in images
	float min_observation_value; // min value in images
	float max_observation_value; // max value in images
	float min_value; // total min value in images
	float max_value; // total max value in images
	float max_bias_value;
	float min_bias_value;

	float min_mean_observation_value;
	float max_mean_observation_value;
	float min_mean_forecast_value;
	float max_mean_forecast_value;

	float region_max_forecast_value;
	float region_min_forecast_value;

	AnalogImage **analog_images = NULL; // the images
	AnalogsResponse(int nImages); // constructor
	~AnalogsResponse(); // constructor
	void Add(string filename, int date, float mse);
	string ToJSON(void); // convert to json

};

// Forecast for a region (forecast and observation)
struct RegionForecast{
public:
	int date;
	float mse;
	float *forecast_values = NULL;
	float *observation_values = NULL;
	~RegionForecast();

	RegionForecast(){
		forecast_values = NULL;
		observation_values = NULL;
	}
};

// A collection of RegionForecast.
struct RegionForecastCollection{

	int nRegions = 0;
	RegionForecast **forecast_region = NULL;
	float max_forecast_value = 0;
	float min_forecast_value = MAX_FLOAT;
	float max_observation_value = 0;
	float min_observation_value = MAX_FLOAT;
	
	float *mean_forecast_values = NULL;
	float *mean_observation_values = NULL;

	float min_mean_observation_value;
	float max_mean_observation_value;
	float min_mean_forecast_value;
	float max_mean_forecast_value;

	RegionForecastCollection(int nRegions);
	~RegionForecastCollection();
};


// The threshold range. For instance, from 0 to 20 mm.
struct ThresholdRange{
	float from;
	float to;
	public:
	ThresholdRange(float from, float to){
		this->from = from;
		this->to = to;
	}
};
