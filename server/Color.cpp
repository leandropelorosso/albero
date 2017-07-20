#include "Color.h"
#include <math.h>  
#include <fstream>  
#include <stdio.h>  
#include "iostream"

using namespace std;


rgb Color::from_palette(float min, float max, float value)
{

	if (min == max) {
		max = min + 0.001f;
	}
	if (isnan(value)){
		rgb c;
		c.r = 255; 
		c.g = 0;
		c.b = 0;
		return c;
	}

	if (enable_zero_transparency){
		if (fabs(min-value)<0.001f){
			rgb c;
			c.r = 0;
			c.g = 0;
			c.b = 0;
			c.a = 0;
			return c;
			}
	}

	if (value > max) value = max;
	if (value < min) value = min;
	int index = int(((Color::nColors[Color::current_selection] - 1) / (max - min)) * (value - min));
    if(Color::enable_invert_palette) index = 20-index-1;
	unsigned char *color = &(memblock[Color::current_selection][index * 3]);
	unsigned char r = color[0];
	unsigned char g = color[1];
	unsigned char b = color[2];
	rgb c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = 255;
	
	return c;
}

void Color::Dispose(){
	delete(nColors);
	
	for (int i = 0; i < Color::nFiles; i++){
		delete(memblock[i]);
	}
	delete(memblock);

}

void Color::Init(int nFiles){
	Color::nColors = new int[nFiles];
	Color::memblock = new unsigned char*[nFiles];
	Color::current_selection = 0;

	Color::enable_invert_palette = false;
	Color::enable_zero_transparency = false;
}

void Color::SetScale(int file_index)
{
	Color::current_selection = file_index;
}

bool Color::read_file(string filename, int nColors)
{
	streamoff filesize;
	ifstream act("d:\\" + filename, ios::binary | ios::ate);
	if (act.is_open())
	{
		filesize = act.tellg();
		act.seekg(0);
		memblock[Color::nFiles] = new unsigned char[filesize];
		Color::nColors[Color::nFiles] = nColors;
		act.read((char*)memblock[Color::nFiles], filesize);
		act.close();
		cout << "Color Table loaded to memory." << endl;
		Color::nFiles++;
		return true;
	}
	else
	{
		cout << "Failed to open file." << endl;
		return false;
	}
}

unsigned char** Color::memblock;
int Color::current_selection;
int Color::nFiles;
int* Color::nColors;
bool Color::enable_invert_palette;
bool Color::enable_zero_transparency;

rgb Color::hsv2rgb(hsv in)
{
	float      hh, p, q, t, ff;
	long        i;
	rgb         out;

	if (in.s <= 0.0f) {       // < is bogus, just shuts up warnings
		if (isnan(in.h)) {   // in.h == NAN
			out.r = (int)in.v;
			out.g = (int)in.v;
			out.b = (int)in.v;
			return out;
		}
		// error - should never happen
		out.r = 0;
		out.g = 0;
		out.b = 0;
		return out;
	}
	hh = in.h;
	if (hh >= 360.0f) hh = 0.0f;
	hh /= 60.0f;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0f - in.s);
	q = in.v * (1.0f - (in.s * ff));
	t = in.v * (1.0f - (in.s * (1.0f - ff)));

	switch (i) {
	case 0:
		out.r = (int)in.v;
		out.g = (int)t;
		out.b = (int)p;
		break;
	case 1:
		out.r = (int)q;
		out.g = (int)in.v;
		out.b = (int)p;
		break;
	case 2:
		out.r = (int)p;
		out.g = (int)in.v;
		out.b = (int)t;
		break;

	case 3:
		out.r = (int)p;
		out.g = (int)q;
		out.b = (int)in.v;
		break;
	case 4:
		out.r = (int)t;
		out.g = (int)p;
		out.b = (int)in.v;
		break;
	case 5:
	default:
		out.r = (int)in.v;
		out.g = (int)p;
		out.b = (int)q;
		break;
	}
	return out;
}


// Returns color from temperature.
// A: lower temperature limit
// B: higher temperature limit
// C: lower hsv degree limit
// D: higher hsv degree limit
// t: temperature
rgb Color::get_color(float A, float B, float C, float D, float t)
{
	float X = t;
	float Y = (X - A) / (B - A) * (D - C) + C;

	hsv c_hsv;
	c_hsv.h = Y;
	c_hsv.s = 1.0f;
	c_hsv.v = 1.0f;

	rgb c_rgb;
	c_rgb = hsv2rgb(c_hsv);

	//Color c;
	c_rgb.r = c_rgb.r * 255;
	c_rgb.g = c_rgb.g * 255;
	c_rgb.b = c_rgb.b * 255;

	return c_rgb;
}






