#pragma once
#include "color.h"
#include "color_schema.h"


class PCT
{

public:
    static void SelectScale(int scale_index, bool transparent);
    static int current_scale;
    static bool transparency;

    // Color schemas
    static ColorSchema* numerical_forecast_schema;
    static ColorSchema* probabilistic_forecast_schema;
    static ColorSchema* mse_schema;
    static ColorSchema* bias_schema;
};

