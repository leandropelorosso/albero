/*********************
A color palette, read from a png file.
**********************/

#pragma once

#include "stdio.h"
#include <string>
#include "Color.h"
#include "ColorSchema.h"

class Palette : public ColorSchema
{
public:
    Palette(string file);
	~Palette();
    rgb get_color(float min_value, float max_value, float value, bool transparency);
private:
	void from_file(std::string file);
	rgb* colors = NULL;
	int ncolors = 0;
};

