#include "PCT.h"
#include "Color.h"
#include <math.h>
#include "iostream"

using namespace std;

ColorRange::ColorRange(float min_value, float max_value, int r, int g, int b, int a){
	this->min_value = min_value;
	this->max_value = max_value;
	this->color = rgb();
	this->color.r = r;
	this->color.g = g;
	this->color.b = b;
	this->color.a = a;
}

ColorRange::ColorRange(float min_value, float max_value, int r, int g, int b){
	this->min_value = min_value;
	this->max_value = max_value;
	this->color = rgb();
	this->color.r = r;
	this->color.g = g;
	this->color.b = b;
	this->color.a = 255;
}

ColorRange::ColorRange(){}

ColorRange** PCT::ranges;
int* PCT::nColors;
int PCT::current_scale;
bool PCT::transparency;

void PCT::SelectScale(int scale_index, bool transparency){
	PCT::current_scale = scale_index;
	PCT::transparency = transparency;
	Color::SetScale(abs(scale_index)-1);
}

void PCT::Init(){
	
	PCT::current_scale = 0;
	PCT::nColors = new int[4];
	PCT::ranges = new ColorRange*[4];

	// ACCUMULATED PRECIPITATION
	PCT::nColors[0] = 21;
	PCT::ranges[0] = new ColorRange[nColors[0]];
	ranges[0][0] = ColorRange(160.0f, 200.0f, 97, 48, 120);
	ranges[0][1] = ColorRange(130.0f, 160.0f, 0x80, 0x40, 0xa0);
	ranges[0][2] = ColorRange(95.0f, 130.0f, 0xa0, 0x60, 0xc0);
	ranges[0][3] = ColorRange(70.0f, 95.0f, 0xff, 0x00, 0xff);
	ranges[0][4] = ColorRange(55.0f, 70.0f, 0xff, 0xc0, 0xff);
	ranges[0][5] = ColorRange(40.0f, 55.0f, 0xb0, 0x00, 0x00);
	ranges[0][6] = ColorRange(35.0f, 50.0f, 0xd0, 0x00, 0x00);
	ranges[0][7] = ColorRange(30.0f, 35.0f, 0xff, 0x00, 0x00);
	ranges[0][8] = ColorRange(25.0f, 30.0f, 0xff, 0x90, 0x00);
	ranges[0][9] = ColorRange(20.0f, 25.0f, 0xdf, 0xc0, 0x00);
	ranges[0][10] = ColorRange(15.0f, 20.0f, 0xff, 0xff, 0x00);
	ranges[0][11] = ColorRange(12.0f, 15.0f, 0xff, 0xff, 0xa0);
	ranges[0][12] = ColorRange(10.0f, 12.0f, 0x00, 0x90, 0x00);
	ranges[0][13] = ColorRange(8.0f, 10.0f, 0x00, 0xc0, 0x00);
	ranges[0][14] = ColorRange(6.0f, 8.0f, 0x00, 0xff, 0x00);
	ranges[0][15] = ColorRange(4.0f, 6.0f, 0xc0, 0xff, 0xc0);
	ranges[0][16] = ColorRange(2.0f, 4.0f, 0x00, 0x00, 0xf0);
	ranges[0][17] = ColorRange(1.0f, 2.0f, 0x00, 0x90, 0xf0);
	ranges[0][18] = ColorRange(0.5f, 1.0f, 0x00, 0xe8, 0xe8);
	ranges[0][19] = ColorRange(0.01f, 0.5f, 0xc0, 0xff, 0xff);
	ranges[0][20] = ColorRange(0.0f, 0.01f, 0xc0, 0xff, 0xff, 0);

	/*
	ranges[0][0] = ColorRange(70, 80, 0xff, 0xff, 0xff);
	ranges[0][1] = ColorRange(60, 70, 0x80, 0x40, 0xa0);
	ranges[0][2] = ColorRange(55, 60, 0xa0, 0x60, 0xc0);
	ranges[0][3] = ColorRange(50, 55, 0xff, 0x00, 0xff);
	ranges[0][4] = ColorRange(45, 50, 0xff, 0xc0, 0xff);
	ranges[0][5] = ColorRange(40, 45, 0xb0, 0x00, 0x00);
	ranges[0][6] = ColorRange(35, 40, 0xd0, 0x00, 0x00);
	ranges[0][7] = ColorRange(30, 35, 0xff, 0x00, 0x00);
	ranges[0][8] = ColorRange(25, 30, 0xff, 0x90, 0x00);
	ranges[0][9] = ColorRange(20, 25, 0xdf, 0xc0, 0x00);
	ranges[0][10] = ColorRange(15, 20, 0xff, 0xff, 0x00);
	ranges[0][11] = ColorRange(12, 15, 0xff, 0xff, 0xa0);
	ranges[0][12] = ColorRange(10, 12, 0x00, 0x90, 0x00);
	ranges[0][13] = ColorRange(8.0, 10, 0x00, 0xc0, 0x00);
	ranges[0][14] = ColorRange(6.0, 8.0, 0x00, 0xff, 0x00);
	ranges[0][15] = ColorRange(4.0, 6.0, 0xc0, 0xff, 0xc0);
	ranges[0][16] = ColorRange(2.0, 4.0, 0x00, 0x00, 0xf0);
	ranges[0][17] = ColorRange(1.0, 2.0, 0x00, 0x90, 0xf0);
	ranges[0][18] = ColorRange(0.5, 1.0, 0x00, 0xe8, 0xe8);
	ranges[0][19] = ColorRange(0.01, 0.5, 0xc0, 0xff, 0xff);
	ranges[0][20] = ColorRange(0.0, 0.01, 0xc0, 0xff, 0xff, 0);
	*/
	
	/*
	for (int i = 0; i < PCT::nColors[0]; i++){
		float val = ranges[0][i].min_value;
		cout << val << endl;
	}
	*/

	// PROBABILITY
	nColors[1] = 11;
	PCT::ranges[1] = new ColorRange[nColors[1]];
	ranges[1][0] = ColorRange(90.0f, 100.0f, 0x29, 0x00, 0x9f);
	ranges[1][2] = ColorRange(80.0f, 90.0f, 0x3c, 0x27, 0xb4);
	ranges[1][3] = ColorRange(70.0f, 80.0f, 0x17, 0x81, 0xbf);
	ranges[1][4] = ColorRange(60.0f, 70.0f, 0x28, 0x96, 0xd5);
	ranges[1][5] = ColorRange(50.0f, 60.0f, 0x5e, 0xbe, 0xfa);
	ranges[1][6] = ColorRange(40.0f, 50.0f, 0x0d, 0xa1, 0x0f);
	ranges[1][7] = ColorRange(30.0f, 40.0f, 0x38, 0xd2, 0x3e);
	ranges[1][8] = ColorRange(20.0f, 30.0f, 0x75, 0xf7, 0x71);
	ranges[1][9] = ColorRange(10.0f, 20.0f, 0xb5, 0xfa, 0xaa);
	ranges[1][10] = ColorRange(0.0f, 10.0f, 0xff, 0xff, 0xff, 0);
	

	// ERROR
	nColors[2] = 19;
	PCT::ranges[2] = new ColorRange[nColors[2]];
	ranges[2][18] = ColorRange(-60.0f, -50.0f, 30, 92, 179);
	ranges[2][17] = ColorRange(-50.0f, -40.0f, 23, 111, 193);
	ranges[2][16] = ColorRange(-40.0f, -30.0f, 11, 142, 216);
	ranges[2][15] = ColorRange(-30.0f, -20.0f, 4, 161, 230);
	ranges[2][14] = ColorRange(-20.0f, -10.0f, 25, 181, 241);
	ranges[2][13] = ColorRange(-10.0f, -0.5f, 51, 188, 207);
	ranges[2][12] = ColorRange(-5.0f, -2.0f, 102, 204, 206);
	ranges[2][11] = ColorRange(-2.0f, -1.0f, 153, 219, 184);
	ranges[2][10] = ColorRange(-1.0f, -0.5f, 192, 229, 136);
	ranges[2][9] = ColorRange(-0.5f, 0.5f, 0,0,255);
	ranges[2][8] = ColorRange(0.5f, 1.0f, 204, 230, 75);
	ranges[2][7] = ColorRange(1.0f, 2.0f, 243, 240, 29);
	ranges[2][6] = ColorRange(2.0f, 5.0f, 254, 222, 39);
	ranges[2][5] = ColorRange(5.0f, 10.0f, 252, 199, 7);
	ranges[2][4] = ColorRange(10.0f, 20.0f, 248, 157, 14);
	ranges[2][3] = ColorRange(20.0f, 30.0f, 245, 114, 21);
	ranges[2][2] = ColorRange(30.0f, 40.0f, 241, 71, 28);
	ranges[2][1] = ColorRange(40.0f, 50.0f, 219, 30, 38);
	ranges[2][0] = ColorRange(50.0f, 60.0f, 20, 164, 38, 44);


	// MSE
	nColors[3] = 11;
	PCT::ranges[3] = new ColorRange[nColors[3]];
	ranges[3][10] = ColorRange(0.0f, 0.0f, 158, 1, 66);
	ranges[3][9] = ColorRange(0.0f, 0.0f, 213, 62, 79);
	ranges[3][8] = ColorRange(0.0f, 0.0f, 244, 109, 67);
	ranges[3][7] = ColorRange(0.0f, 0.0f, 253, 174, 97);
	ranges[3][6] = ColorRange(0.0f, 0.0f, 254, 224, 139);
	ranges[3][5] = ColorRange(0.0f, 0.0f, 255, 255, 191);
	ranges[3][4] = ColorRange(0.0f, 0.0f, 230, 245, 152);
	ranges[3][3] = ColorRange(0.0f, 0.0f, 171, 221, 164);
	ranges[3][2] = ColorRange(0.0f, 0.0f, 102, 194, 165);
	ranges[3][1] = ColorRange(0.0f, 0.0f, 50, 136, 189);
	ranges[3][0] = ColorRange(0.0f, 0.0f, 94, 79, 162);
}

rgb PCT::GetColorFromRange(float value, float min, float max){
	
	ColorRange* range = PCT::ranges[current_scale];
	int nColors = PCT::nColors[current_scale];

	if (value == min) return range[nColors/2].color;

	if (value - min == 0) return range[(int)(nColors / 2)].color;
	int index = (int)floor(((nColors / (max - min))*(value - min)));
	return range[index].color;
}

rgb PCT::GetColor(float value)
{

	ColorRange* range = PCT::ranges[current_scale];
	int nColors = PCT::nColors[current_scale];

	if (value >= range[0].max_value) return range[0].color;
	if (value < range[nColors - 1].min_value) {
		
		rgb transparent = rgb();
		transparent.r = 255;
			transparent.b = 255;
			transparent.g = 255;
		transparent.a = 0;
		return transparent;
	}

	for (int i = 0; i < nColors; i++){
		if (value >= range[i].min_value && value < range[i].max_value){
			return range[i].color;
		}
	}

	return rgb();
}

void PCT::Dispose()
{
	for (int i = 0; i < 4; i++){
		delete(ranges[i]);
	}
	delete(ranges);
	delete(nColors);
}

