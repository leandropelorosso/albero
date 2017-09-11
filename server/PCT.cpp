#include "PCT.h"
#include "Color.h"
#include <math.h>
#include "iostream"

using namespace std;

int PCT::current_scale;
bool PCT::transparency;

void PCT::SelectScale(int scale_index, bool transparency){
    PCT::current_scale = scale_index;
    PCT::transparency = transparency;
    Color::SetScale(abs(scale_index)-1);
}
