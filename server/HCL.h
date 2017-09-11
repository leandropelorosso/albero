#pragma once

#include "Color.h"
#include "helper.h"
#include "ColorSchema.h"


// HCL Color schema
class hcl : public ColorSchema
{
   Vector2 from_color;
   Vector2 to_color;

public:
    hcl();
    hcl(Vector2 from_color, Vector2 to_color);
	~hcl();

	// returns a color from the hlc scale, using the segment described here:  https://vis4.net/blog/posts/avoid-equidistant-hsv-colors/
    rgb get_color(float min_value, float max_value, float value, bool transparency);

	static rgb get_color_constant_H(Vector2 from_color, Vector2 to_color, float H, float min_value, float max_value, float value, bool transparency);
};

// Diverging HCL Color scheme
class HCLDiverging : public hcl{
public:
    HCLDiverging();
    //get_color_blue_red_diverging
    rgb get_color(float min_value, float max_value, float value, bool transparency);
};
