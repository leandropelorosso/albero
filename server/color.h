#pragma once

#include <stdio.h>
#include <string>    
using namespace std;

// ------------------------------------------------------------------------------
// Type definitions
// ------------------------------------------------------------------------------


typedef struct hsv{
	float h;       // angle in degrees
	float s;       // percent
	float v;       // percent
} hsv;

typedef struct rgb{
	int r;       // percent
	int g;       // percent
	int b;       // percent
	int a;
} rgb;

class Color
{
public:
	

	static int current_selection;
	static rgb get_color(float A, float B, float C, float D, float t);
	static Color get_blue(float A, float B, float t);
	static bool read_file(string filename, int nColors);
	static void Init(int nFiles);
	static void Dispose();

	static void SetScale(int file_index);

	static int* nColors; // amount of colors in scale[i]
	static int nFiles; // amount of files

	static rgb from_palette(float min, float max, float value);
	static unsigned char** memblock;

	static bool enable_invert_palette;
	static bool enable_zero_transparency;

	float r;
	float g;
	float b;

private:
	static rgb hsv2rgb(hsv in);

};

