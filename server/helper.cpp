#include <stdio.h>
#include <netcdf.h>
#include "iostream"
#include <chrono>
#include <assert.h>  
#include <fstream>
#include <string>     
#include <iomanip>
#include <assert.h> 
#include "Color.h"
#include "helper.h"
#include "PCT.h"
#include "HCL.h"
#include "math.h"

#include <IL/il.h>
#pragma comment( lib, "DevIL/DevIL.lib" )

using namespace std;

double Clip(double n, double minValue, double maxValue)
{
	return fminl(fmaxl(n, minValue), maxValue);
}

double getLatFromPixel(int pixelY, int zoom){
	double pi = 3.14159265358979323846;
	double mapSize = 256 << zoom;
	double y = 0.5 -(Clip(pixelY, 0, mapSize - 1) / mapSize);
	double latitude = 90 - 360 * atan(exp(-y * 2 * pi)) / pi;
	return (double)latitude;
}

double getLngFromPixel(int pixelX, int zoom){
	unsigned int offset = 256 << zoom;

	//I have to convert pixel int to float	
	double r = (double)pixelX;
	r /= offset;
	r *= 360;
	r -= 180;

	return r;
	

}


// Projection functionality
int getPixelbyLat(double lat, int zoom){
	unsigned int offset = 256 << zoom;

	double pi = 3.14159265358979323846;
	//return (int) (offset - offset/pi * log((1 + sin(lat * pi / 180)) / (1 - sin(lat * pi/ 180))) / 2);
	double sinLat = sin((lat * pi) / 180.0);

	double r = (1.0f + sinLat) / (1.0 - sinLat);
	r = log(r);
	r *= offset;
	r /= 4.0 * pi;
	return  (int)(offset / 2.0 - r);
}

int getPixelbyLng(double lng, int zoom){
	unsigned int offset = 256 << zoom;
	return (int)((lng + 180.0) / 360.0 * offset);
}


//https://helloacm.com/cc-function-to-compute-the-bilinear-interpolation/
float BilinearInterpolation(float q11, float q12, float q21, float q22, float x1, float x2, float y1, float y2, float x, float y)
{
	float x2x1, y2y1, x2x, y2y, yy1, xx1;
	x2x1 = x2 - x1;
	y2y1 = y2 - y1;
	x2x = x2 - x;
	y2y = y2 - y;
	yy1 = y - y1;
	xx1 = x - x1;
	return 1.0f / (x2x1 * y2y1) * (
		q11 * x2x * y2y +
		q21 * xx1 * y2y +
		q12 * x2x * yy1 +
		q22 * xx1 * yy1
		);
}


float *Interpolate3(float *input, int input_width, int input_height, float input_init_lat, float input_init_lon, float input_end_lat, float	input_end_lon,
	float init_lat, float init_lon, float end_lat, float end_lon, int amount_steps_lat, int amount_steps_lon, bool flip_vertical){

	// The result
	float *interpolation = new float[amount_steps_lat*amount_steps_lon];

	// find the delta step on the netcdf file.
	float deltaLon = (input_end_lon - input_init_lon) / (input_width - 1);
	float deltaLat = (input_end_lat - input_init_lat) / (input_height - 1);


	// the delta of the interpolation
	float interpolation_lat_delta = (end_lat - init_lat) / (float)(amount_steps_lat-1);
	float interpolation_lon_delta = (end_lon - init_lon) / (float)(amount_steps_lon-1);


	for (int iy = 0; iy < amount_steps_lat; iy++){

		for (int ix = 0; ix < amount_steps_lon; ix++){

			float x = ix * interpolation_lon_delta + init_lon;
			float y = iy * interpolation_lat_delta + init_lat;

			if ((x<input_init_lon) || (x>input_end_lon) || (y<input_init_lat) || (y> input_end_lat) ) {
				interpolation[((flip_vertical ? (amount_steps_lat - iy - 1) : (iy))*amount_steps_lon) + ix] = NAN;
				continue;
			}

			// q12 in absolute value coordinates
			int qy = (int)floor(fabs((y - input_init_lat) / deltaLat));
			int qx = (int)floor(fabs((x - input_init_lon) / deltaLon));

			if (qx == input_width - 1) qx = input_width - 2;
			if (qy == input_height - 1) qy = input_height - 2;
			// now get the index of q12 in our values matrix.
			int qindex = (qy)* input_width + (qx);

			float x1 = input_init_lon + qx * deltaLon;
			float x2 = x1 + deltaLon;
			float y2 = input_init_lat + qy * deltaLat;
			float y1 = y2 + deltaLat;

			float q11 = input[qindex + input_width];
			float q12 = input[qindex];
			float q22 = input[qindex + 1];
			float q21 = input[qindex + input_width + 1];


			float value = BilinearInterpolation(q11, q12, q21, q22, x1, x2, y1, y2, x, y);

			if (value < 0) value = 0;
			
			interpolation[((flip_vertical?(amount_steps_lat - iy - 1):(iy))*amount_steps_lon) + ix] = value;

		}
	}

	return interpolation;
}


// Paints a pixel into a char array
void paint_pixel(char* img, int img_w, int img_h, int x, int y, int r, int g, int b, int a)
{
	y = img_h - y - 1;
	img[(y*img_w + x) * 3 + 0] = r;
	img[(y*img_w + x) * 3 + 1] = g;
	img[(y*img_w + x) * 3 + 2] = b;
	img[(y*img_w + x) * 3 + 3] = a;

}


// Paints a pixel into a char array
// All the attributes are pretending that the pixel size equals 1.
// So if the real output image is 10*pixel_size, img_w will be 10 and x and y will be between 0 and 9.
void paint_pixel(char* img, int img_w, int img_h, int x, int y, int r, int g, int b, int a, int pixel_size)
{
	y = img_h - y - 1;

	img_w *= pixel_size;
	img_h *= pixel_size;
	x *= pixel_size;
	y *= pixel_size;

	for (int _x = x; _x < x + pixel_size; _x++){
		for (int _y = y; _y < y + pixel_size; _y++){
			img[(_y*img_w + _x) * 4 + 0] = r;
			img[(_y*img_w + _x) * 4 + 1] = g;
			img[(_y*img_w + _x) * 4 + 2] = b;
			img[(_y*img_w + _x) * 4 + 3] = a;
		}
	}

	

}

// Writes a char array to a PNG file. Assumes the char* contains R,G and B channels.
bool ilInitialized = false;
int imageID;

void WriteImage(char* image, int width, int height, string filename)
{
	if (!ilInitialized){
		ilInit();
		ilInitialized = true;
		imageID = ilGenImage();
		ilBindImage(imageID);
		ilEnable(IL_FILE_OVERWRITE);
	}

	ilTexImage(
		width,
		height,
		1,  // OpenIL supports 3d textures!  but we don't want it to be 3d.  so we just set this to be 1
		4,  // 3 channels:  one for R , one for G, one for B
		IL_RGBA,  // duh, yeah use rgb!  coulda been rgba if we wanted trans
		IL_UNSIGNED_BYTE,  // the type of data the imData array contains (next)
		image
		);
	
	ilSave(IL_PNG,filename.c_str());
	
}

char* ImageFromValuesArray(ColorSchema* schema, float *input, int width, int height, int pixel_size, float min_value, float max_value)
{
	char* image = new char[width * height * 4 * pixel_size * pixel_size];
	if (!input) return NULL;
	for (int y = 0; y <height; y++){
		for (int x = 0; x<width; x++){
			int i = y * width + x;
            float value = input[i];
            rgb c=schema->get_color(min_value, max_value, value, PCT::transparency);
            paint_pixel(image, width, height, x, y, c.r, c.g, c.b, c.a, pixel_size);
		}
	}
	return image;
}

// Writes a values array to a PNG files. The given parameter are values, so they are mapped to colors before being written to the PNG.
void WriteImage(ColorSchema *schema, float* input, int width, int height, string filename, int pixel_size, float min_value, float max_value)
{
    char *image = ImageFromValuesArray(schema, input, width, height, pixel_size, min_value, max_value);
	WriteImage(image, width*pixel_size, height*pixel_size, filename);
	delete(image);
}

// Writes a values array to a PNG files. The given parameter are values, so they are mapped to colors before being written to the PNG.
void WriteImage(ColorSchema *schema, float* input, int width, int height, string filename, float min_value, float max_value)
{
    char *image = ImageFromValuesArray(schema, input, width, height, 1, min_value, max_value);
	WriteImage(image, width, height, filename);
	delete(image);
}

// Distance between two coordinates.
#define R 6371
#define TO_RAD (3.1415926536f / 180.0f)
float Distance(float th1, float ph1, float th2, float ph2)
{
	float dx, dy, dz;
	ph1 -= ph2;
	ph1 *= TO_RAD, th1 *= TO_RAD, th2 *= TO_RAD;

	dz = sin(th1) - sin(th2);
	dx = cos(ph1) * cos(th1) - cos(th2);
	dy = sin(ph1) * cos(th1);
	return asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * R;
}


AnalogsResponse::AnalogsResponse(int nImages){
	this->analog_images = new AnalogImage*[nImages];
	this->index = 0;
}

AnalogsResponse::~AnalogsResponse(){
	for (int i = 0; i < this->index; i++){
		delete(this->analog_images[i]);
	}
	delete(this->analog_images);
}

void AnalogsResponse::Add(string filename, int date, float mse){
	this->analog_images[this->index] = new AnalogImage();
	this->analog_images[this->index]->date = date;
	this->analog_images[this->index]->filename = filename;
	this->analog_images[this->index]->mse = mse;
	this->index++;
}

string AnalogsResponse::ToJSON(){
	// Build the result, something like: [{"date":"232323232", "filename" : "fdsfdsffsfsfsf"}]
	string result = "{\"images\":[";
	for (int i = 0; i < this->index; i++){
		result += "{\"date\":\"" + to_string(this->analog_images[i]->date) + (string)"\",\"filename\":\"" + this->analog_images[i]->filename + "\",\"mse\":\"" + to_string(this->analog_images[i]->mse) + "\"}";
		if (i != this->index - 1) result += ',';
	}
	result += "], \"min_forecast_value\":\"" + to_string(this->min_forecast_value) + "\"" +
			   ", \"max_forecast_value\":\"" + to_string(this->max_forecast_value) + "\"" +
			   ", \"min_observation_value\":\"" + to_string(this->min_observation_value) + "\"" +
			   ", \"max_observation_value\":\"" + to_string(this->max_observation_value) + "\"" +
			   ", \"min_value\":\"" + to_string(this->min_value) + "\"" +
			   ", \"max_value\":\"" + to_string(this->max_value) + "\"" +
			   ", \"max_bias_value\":\"" + to_string(this->max_bias_value) + "\"" +
			   ", \"min_bias_value\":\"" + to_string(this->min_bias_value) + "\"" +
			   ", \"min_mean_forecast_value\":\"" + to_string(this->min_mean_forecast_value) + "\"" +
			   ", \"max_mean_forecast_value\":\"" + to_string(this->max_mean_forecast_value) + "\"" +
			   ", \"min_mean_observation_value\":\"" + to_string(this->min_mean_observation_value) + "\"" +
			   ", \"max_mean_observation_value\":\"" + to_string(this->max_mean_observation_value) + "\"" +
			   ", \"region_max_forecast_value\":\"" + to_string(this->region_max_forecast_value) + "\"" +
			   ", \"region_min_forecast_value\":\"" + to_string(this->region_min_forecast_value) + "\"" +

		"}";
	return result;
}


RegionForecast::~RegionForecast(){
	if (forecast_values!=NULL) delete(this->forecast_values);
	if (observation_values!=NULL) delete(this->observation_values);
}

RegionForecastCollection::RegionForecastCollection(int nRegions){
	this->nRegions = nRegions;
	forecast_region = new RegionForecast*[nRegions];
}

RegionForecastCollection::~RegionForecastCollection(){
	delete(this->mean_forecast_values);
	delete(this->mean_observation_values);
	for (int i = 0; i < nRegions; i++){
		delete(this->forecast_region[i]);
	}
	delete(this->forecast_region);
}



float *Interpolate8(float *input, int input_width, int input_height, float input_init_lat, float input_init_lon, float input_end_lat, float	input_end_lon,
	int init_lat_pixel, int init_lon_pixel, int end_lat_pixel, int end_lon_pixel, int amount_steps_lat, int amount_steps_lon, int zoom){


	// The result
	float *interpolation = new float[amount_steps_lat*amount_steps_lon];

	// find the delta step on the netcdf file.
	float deltaLon = (input_end_lon - input_init_lon) / (input_width - 1);
	float deltaLat = (input_end_lat - input_init_lat) / (input_height - 1);

	if (init_lat_pixel > end_lat_pixel){
		int c = end_lat_pixel;
		end_lat_pixel = init_lat_pixel;
		init_lat_pixel = c;
	}


	for (int iy = init_lat_pixel; iy < end_lat_pixel; iy++){

		for (int ix = init_lon_pixel; ix < end_lon_pixel; ix++){

			//float x = ix * interpolation_lon_delta + init_lon;
			//float y = iy * interpolation_lat_delta + init_lat;

			float y = (float)getLatFromPixel(iy, zoom);
			float x = (float)getLngFromPixel(ix, zoom);

			if ( (x<input_init_lon) || (x>input_end_lon) || (y<input_init_lat) || (y>input_end_lat) /*|| (((x - 360 < -75) || (x - 360 > -52) || (y<-56) || (y>-21)))*/) {
				interpolation[(iy - init_lat_pixel)*amount_steps_lon + (ix - init_lon_pixel)] = NAN;
				continue;
			}

			// q12 in absolute value coordinates
			int qy = (int)floor(fabs((y - input_init_lat) / deltaLat));
			int qx = (int)floor(fabs((x - input_init_lon) / deltaLon));

			if (qx == input_width - 1) qx = input_width - 2;
			if (qy == input_height - 1) qy = input_height - 2;
			// now get the index of q12 in our values matrix.
			int qindex = (qy)* input_width + (qx);

			float x1 = input_init_lon + qx * deltaLon;
			float x2 = x1 + deltaLon;
			float y2 = input_init_lat + qy * deltaLat;
			float y1 = y2 + deltaLat;

			float q11 = input[qindex + input_width];
			float q12 = input[qindex];
			float q22 = input[qindex + 1];
			float q21 = input[qindex + input_width + 1];


			float value = BilinearInterpolation(q11, q12, q21, q22, x1, x2, y1, y2, x, y);

			if (value < 0) value = 0;
			interpolation[(iy - init_lat_pixel)*amount_steps_lon + (ix - init_lon_pixel)] = value;

		}
	}

	return interpolation;
}




