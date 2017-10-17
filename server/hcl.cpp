#include "hcl.h"
#include "math.h"
#include "iostream"
#include "helper.h"

struct float3{
	float x, y, z;
	float3(float _x, float _y, float _z){
		x = _x;
		y = _y;
		z = _z;
	}
};




float3 hcl2lab(float c, float s, float l) {
	float L, TAU, a, angle, b, r;
	c /= 360.0f;
	TAU = 6.283185307179586476925287f;
	L = l * 0.61f + 0.09f;
	angle = TAU / 6.0f - c * TAU;
	r = (l * 0.311f + 0.125f) * s;
	a = sin(angle) * r;
	b = cos(angle) * r;
	return float3(L, a, b);
};




float finv(float t) {
	if (t > (6.0 / 29.0)) {
		return t * t * t;
	}
	else {
		return 3.0f * (6.0f / 29.0f) * (6.0f / 29.0f) * (t - 4.0f / 29.0f);
	}
};


/*
Convert from L*a*b* doubles to XYZ doubles
Formulas drawn from http://en.wikipedia.org/wiki/Lab_color_spaces
*/
float3 lab2xyz(float l, float a, float b) {
	float sl, x, y, z;
	sl = (l + 0.16f) / 1.16f;
	y = 1.00000f * finv(sl);
	x = 0.96421f * finv(sl + (a / 5.0f));
	z = 0.82519f * finv(sl - (b / 2.0f));
	return float3(x, y, z);
};


/*
*/
float correct_cl(float cl) {
	float a;
	a = 0.055f;
	if (cl <= 0.0031308f) {
		return 12.92f * cl;
	}
	else {
		return (1.0f + a) * pow(cl, 1.0f / 2.4f) - a;
	}
};


float min3f(float a, float b, float c){
	return(fmin(a, fmin(b, c)));
}

float max3f(float a, float b, float c){
	return(fmax(a, fmax(b, c)));
}

/*
Convert from XYZ doubles to sRGB bytes
Formulas drawn from http://en.wikipedia.org/wiki/Srgb
*/
float3 xyz2rgb(float x, float y, float z) {
	float b, bl, clip, g, gl, r, rl;

	rl = 3.2406f * x - 1.5372f * y - 0.4986f * z;
	gl = -0.9689f * x + 1.8758f * y + 0.0415f * z;
	bl = 0.0557f * x - 0.2040f * y + 1.0570f * z;
	clip = min3f(rl, gl, bl) < -0.001f || max3f(rl, gl, bl) > 1.001f;
	if (clip) {
		rl = rl < 0.0 ? 0.0f : rl > 1.0 ? 1.0f : rl;
		gl = gl < 0.0 ? 0.0f : gl > 1.0 ? 1.0f : gl;
		bl = bl < 0.0 ? 0.0f : bl > 1.0 ? 1.0f : bl;
	}
	if (clip) {
		float3 _ref3 = float3(0, 0, 0);
		rl = _ref3.x;
		gl = _ref3.y;
		bl = _ref3.z;
	}

	r = round(255.0f * correct_cl(rl));
	g = round(255.0f * correct_cl(gl));
	b = round(255.0f * correct_cl(bl));
	return float3(r, g, b);
};

/*
Convert from LAB doubles to sRGB bytes
(just composing the above transforms)
*/

float3 lab2rgb(float l, float a, float b) {
	float x, y, z;
	float3 _ref4 = lab2xyz(l, a, b);
	x = _ref4.x;
	y = _ref4.y;
	z = _ref4.z;
	return xyz2rgb(x, y, z);
};


float3 hcl2rgb(float c, float s, float l) {
	float L, a, b;
	float3 _ref2 = hcl2lab(c, s, l);
	L = _ref2.x;
	a = _ref2.y;
	b = _ref2.z;
	return lab2rgb(L, a, b);
};

hcl::hcl(){
}

hcl::hcl(Vector2 from_color, Vector2 to_color)
{	
    this->from_color = from_color;
    this->to_color = to_color;
}


hcl::~hcl()
{
}


rgb hcl::get_color(float min_value, float max_value, float value, bool transparency){

    // discretize the value if setted
    value = discrete(value);

	if (isnan(value)){

		rgb transparent = rgb();
		transparent.r = 255;
		transparent.b = 255;
		transparent.g = 255;
		transparent.a = 0;
		return transparent;
	}
	
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

		// if the value is 0 (remember it's discretized), then we returned transparent. 
		// if the value is higher than 0, then we we still want it to start from the lower color, so we reduce it by one.
		value -= 1;
	}

	if (value < min_value) value = min_value;
	if (value > max_value) value = max_value;

	float to_x = to_color.x;
	float to_y = to_color.y;
	
	float from_x = from_color.x;
	float from_y = from_color.y;

	float v = value - min_value;

	float steps = (max_value - min_value);

	float x = from_x + ((to_x - from_x) / steps)*v;
	float y = from_y + ((to_y - from_y) / steps)*v;
		
	float3 c = hcl2rgb(x, 1, y);

//	std::cout << c.x << " " << c.y << " " << c.z << endl;

	rgb r;
	r.r = (int)c.x;
	r.g = (int)c.y;
	r.b = (int)c.z;
	r.a = 255;

	

	return r;
}

// esto es para calcular colores con H constante. 
rgb hcl::get_color_constant_H(Vector2 from_color, Vector2 to_color, float H, float min_value, float max_value, float value, bool transparency){

	if (isnan(value)){

		rgb transparent = rgb();
		transparent.r = 255;
		transparent.b = 255;
		transparent.g = 255;
		transparent.a = 0;
		return transparent;
	}

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

		// if the value is 0 (remember it's discretized), then we returned transparent. 
		// if the value is higher than 0, then we we still want it to start from the lower color, so we reduce it by one.
		value -= 1;
	}

	if (value < min_value) value = min_value;
	if (value > max_value) value = max_value;

	float to_x = to_color.x;
	float to_y = to_color.y;

	float from_x = from_color.x;
	float from_y = from_color.y;

	float v = value - min_value;

	float steps = (max_value - min_value);

	float x = from_x + ((to_x - from_x) / steps)*v;
	float y = from_y + ((to_y - from_y) / steps)*v;

	float3 c = hcl2rgb(H, x, y);

	//	std::cout << c.x << " " << c.y << " " << c.z << endl;

	rgb r;
	r.r = (int)c.x;
	r.g = (int)c.y;
	r.b = (int)c.z;



	return r;
}


HCLDiverging::HCLDiverging(){
}

// get_color_blue_red_diverging
rgb HCLDiverging::get_color(float min_value, float max_value, float value, bool transparency){

    // discretize the value if setted
    value = discrete(value);

    float h, c, l;
    float fabs_value = fabs(value);

    if (value<0) h = 254.0f; else h = 0;

    float v1 = 2.1607142857142856f;
    float v3 = 0.4634523809523809f;
    float v2 = 0.053571428571428575f;
    float v4 = 1.2689285714285714f;


    c = ((v1 - v2) / max_value)*fabs(value) + v2;
    l = ((v3 - v4) / max_value)*fabs(value) + v4;


    float3 color = (hcl2rgb(h, c, l));

    rgb r;
    r.r = (int)color.x;
    r.g = (int)color.y;
    r.b = (int)color.z;
    r.a = 255;
    return r;

}

