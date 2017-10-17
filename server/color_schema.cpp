#include "color.h"
#include "color_schema.h"
#include "helper.h"
#include "math.h"

// Renders a PNG scale of 1px by height into the file.
// width: height of the scale
// height: height of the scale
// min_value: min data value
// max_value: max data value
void ColorSchema::render_scale(int width, int height, float min_value, float max_value, string filename){

    float *values = new float[width*height];

    float diff = max_value - min_value;

    for (int y = 0; y < height; y++){

        //float v = (diff / height) * y;
        float v = (max_value - min_value) / height*y + min_value;

        for (int x = 0; x < width; x++){

            values[(height-y-1)*width + x] = v;

        }
    }

    char* img = ImageFromValuesArray(this, values, width, height, 1, min_value, max_value);
    WriteImage(img, width, height, filename);
    delete(values);
    delete(img);

}

// Sets the discretization step to use when retrieving the color.
ColorSchema* ColorSchema::discretization(float discretization_step){
    this->discretization_step = discretization_step;
    return this;
}

// process the value according to the discretization value.
float ColorSchema::discrete(float value){
    if(discretization_step!=0) return ((floor(value / discretization_step))*discretization_step);
    return value;
}
