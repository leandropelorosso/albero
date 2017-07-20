#pragma once

#include "Color.h"
#include "helper.h"

class hcl
{
public:
	hcl();
	~hcl();

	// returns a color from the hlc scale, using the segment described here:  https://vis4.net/blog/posts/avoid-equidistant-hsv-colors/
	static rgb get_color(Vector2 from_color, Vector2 to_color, float min_value, float max_value, float value, bool transparency);

	static rgb get_color_constant_H(Vector2 from_color, Vector2 to_color, float H, float min_value, float max_value, float value, bool transparency);

	static rgb get_color_blue_red_diverging(float min_value, float max_value, float value);

	static void render_scale(int width, int height, float min_value, float max_value, string filename);

};

