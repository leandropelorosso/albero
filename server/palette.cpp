#include "palette.h"
#include <string>
#include "color.h"
#include "math.h"
#include <IL/il.h>

#include <stdio.h>
#include "iostream"

#pragma comment( lib, "DevIL/DevIL.lib" )

using namespace std;

Palette::Palette(string file)
{
    this->from_file(file);
}

Palette::~Palette()
{
    delete(this->colors);
}

void Palette::from_file(std::string file){
    ILuint ImgId = 0;
	ilGenImages(1, &ImgId);
	ilBindImage(ImgId);

	ilLoadImage(file.c_str());
	ILuint  width = ilGetInteger(IL_IMAGE_WIDTH);
	ILuint  height = ilGetInteger(IL_IMAGE_HEIGHT);
	ILubyte * bytes = ilGetData();
	this->ncolors = width;
	this->colors = new rgb[width];
	for (int i = 0; i <=0; i++)
	{
		for (int j = 0; j < width; j++)
		{
			rgb color;
			color.a = 255;
			color.r = bytes[(i*width + j) * 3 + 0];
			color.g = bytes[(i*width + j) * 3 + 1];
			color.b = bytes[(i*width + j) * 3 + 2];
			colors[j] = color;
		}
	}
}

rgb Palette::get_color(float min, float max, float value, bool transparency){

    // discretize value if setted.
    value = this->discrete(value);

	if (transparency)
	{
		if (value == 0){
			rgb transparent = rgb();
			transparent.r = 255;
			transparent.b = 255;
			transparent.g = 255;
			transparent.a = 0;
			return transparent;
		}
	}

	if (isnan(value)){

		rgb transparent = rgb();
		transparent.r = 255;
		transparent.b = 255;
		transparent.g = 255;
		transparent.a = 0;
		return transparent;
	}

    rgb color;

    if (value == min) color = colors[0];
    if (value >= max) color = colors[ncolors-1];

    int index = (int)floor(((ncolors/ (max - min))*(value - min)));

    color = colors[index];

    if(color.r==255 & color.g ==255 & color.b==255) {
        rgb transparent = rgb();
        transparent.r = 255;
        transparent.b = 255;
        transparent.g = 255;
        transparent.a = 0;
        return transparent;
    }

    return color;
}

