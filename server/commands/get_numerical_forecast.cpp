#include "command.h"
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filestream.h"
#include "../albero2.h"
#include "../observation_reader.h"
#include "../pct.h"

extern string albero_images_path;

std::string Commands::GetNumericalForecast::Execute(Albero2* albero2, rapidjson::Document &document)
{
    string s_lat = document["y"].GetString();
    float y = ::atof(s_lat.c_str());

    string s_lon = document["x"].GetString();
    float x = ::atof(s_lon.c_str());

    string s_zoom = document["z"].GetString();
    float z = ::atof(s_zoom.c_str());

    string s_range_index = document["range_index"].GetString();
    int range_index = ::atoi(s_range_index.c_str());

    // obtenemos la fecha deseada si existe
    int current_date = (albero2->current_date);
    int requested_date = current_date;

    if(document.HasMember(("date"))){
      string s_date = document["date"].GetString();
      requested_date = ::atoi(s_date.c_str());
    }

    int init_lat_pixel = (y + 1) * 256;
    int end_lat_pixel = y * 256;
    int init_lon_pixel = x * 256;
    int end_lon_pixel = (x + 1) * 256;

    cout << "REQ>>NumForecast>> " << x << ", " << y << ", " << z << endl;

    // Grabamos los valores interpolados

    // Retrieve the forecast for the selected date [NLATxNLON]
    float* forecast = &albero2->forecasts->forecasts_by_range[range_index][albero2->forecasts->forecast_index[requested_date]];

    float* interpolated_values = Interpolate8(forecast, (int)albero2->forecasts->NLON, (int)albero2->forecasts->NLAT,
            albero2->forecasts->lats[0],
            albero2->forecasts->lons[0],
            albero2->forecasts->lats[albero2->forecasts->NLAT - 1],
            albero2->forecasts->lons[albero2->forecasts->NLON - 1],
            init_lat_pixel, init_lon_pixel, end_lat_pixel, end_lon_pixel, 256, 256, z);


    PCT::SelectScale(-3, true);
    string probabilistic_forecast_image_filename = albero_images_path + "num_forecast_" + to_string((int)x) + "_" + to_string((int)y) + "_" + to_string((int)z) + ".png";
    cout << probabilistic_forecast_image_filename << endl;

    WriteImage(PCT::numerical_forecast_schema, interpolated_values, 256, 256, probabilistic_forecast_image_filename, 0,60);
    delete(interpolated_values);

    return probabilistic_forecast_image_filename;

}
