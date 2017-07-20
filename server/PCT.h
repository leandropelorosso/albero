#pragma once
#include "Color.h"

struct ColorRange
{
	float min_value = 0;
	float max_value = 0;
	rgb color;
	ColorRange(float min_value, float max_value, int r, int g, int b);
	ColorRange(float min_value, float max_value, int r, int g, int b, int a);
	ColorRange();
};

class PCT
{
	static ColorRange **ranges;
	static int *nColors; 
	
	

public:
	static void Init();
	static void Dispose();

	static rgb GetColor(float value);
	static rgb GetColorFromRange(float value, float min, float max);
	static void SelectScale(int scale_index, bool transparent);
	static int current_scale;
	static bool transparency;
};

