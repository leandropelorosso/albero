#include "command.h"
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filestream.h"
#include "../Albero2.h"
#include "../ObservationReader.h"
#include "../PCT.h"

extern string albero_images_path;

std::string Commands::GetProbabilisticForecast::Execute(Albero2* albero2, rapidjson::Document &document)
{
    string s_lat = document["y"].GetString();
    float y = ::atof(s_lat.c_str());

    string s_lon = document["x"].GetString();
    float x = ::atof(s_lon.c_str());

    string s_zoom = document["z"].GetString();
    float z = ::atof(s_zoom.c_str());

    string s_threshold_index = document["threshold_index"].GetString();
    int threshold_index = ::atoi(s_threshold_index.c_str());

    string s_range_index = document["range_index"].GetString();
    int range_index = ::atoi(s_range_index.c_str());

    float init_lat_pixel = (y + 1) * 256;
    float end_lat_pixel = y * 256;
    float init_lon_pixel = x * 256;
    float end_lon_pixel = (x + 1) * 256;

    cout << "Procesando pedido de " << x << ", " << y << ", " << z << endl;

    /* Grabamos los valores interpolados */
    size_t probability_map_index = range_index * albero2->threshold_ranges.size() + threshold_index;

    float* interpolated_values = Interpolate8(albero2->probability_map[probability_map_index], albero2->probability_map_width, albero2->probability_map_height,
        albero2->forecasts->lats[0],
        albero2->forecasts->lons[0],
        albero2->forecasts->lats[albero2->forecasts->NLAT - 1],
        albero2->forecasts->lons[albero2->forecasts->NLON - 1],
        init_lat_pixel, init_lon_pixel, end_lat_pixel, end_lon_pixel, 256, 256, z);

    PCT::SelectScale(-7, true);

    string probabilistic_forecast_image_filename = albero_images_path + "probabilistic_forecast_" + to_string((int)x) + "_" + to_string((int)y) + "_" + to_string((int)z) + ".png";
    WriteImage(PCT::probabilistic_forecast_schema, interpolated_values, 256, 256, probabilistic_forecast_image_filename, 0,/*1.0f/ albero2->N_ANALOGS_PER_LAT_LON*/ 100.0f);
    delete(interpolated_values);

    return probabilistic_forecast_image_filename;
}
