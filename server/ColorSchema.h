#ifndef COLORSCHEMA_H
#define COLORSCHEMA_H

#include "Color.h"

class ColorSchema{
public:
    // returns the corresponding rgb color
    virtual rgb get_color(float min, float max, float value, bool transparency) = 0;
    // render schema color scale into a file
    void render_scale(int width, int height, float min_value, float max_value, string filename);
    // sets discretization value
    ColorSchema* discretization(float discretization_step);
protected:
    // the discretization step value
    float discretization_step;
    // processes the value according to the discretization value
    float discrete(float);
};

#endif // COLORSCHEMA_H
