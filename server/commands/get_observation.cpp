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

std::string Commands::GetObservation::Execute(Albero2* albero2, rapidjson::Document &document)
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
    int date = (albero2->current_date);
    if(document.HasMember(("date"))){
        string s_date = document["date"].GetString();
        date = ::atoi(s_date.c_str());
    }

    int init_lat_pixel = (y + 1) * 256;
    int end_lat_pixel = y * 256;
    int init_lon_pixel = x * 256;
    int end_lon_pixel = (x + 1) * 256;

    cout << "REQ>>NumForecast>> " << x << ", " << y << ", " << z << endl;

    /* Grabamos los valores interpolados */
    float *observation_values = ObservationReader::GetInterpolatedValuesLambert(date, range_index, init_lat_pixel, init_lon_pixel, end_lat_pixel, end_lon_pixel, 256, 256, z);
    if (observation_values == NULL) {
        return "none";
    }
    PCT::SelectScale(-3, true);


    string probabilistic_forecast_image_filename = albero_images_path + "probabilistic_forecast_" + to_string((int)x) + "_" + to_string((int)y) + "_" + to_string((int)z) + ".png";
    WriteImage(PCT::numerical_forecast_schema, observation_values, 256, 256, probabilistic_forecast_image_filename, 0.0f, 60.0f);

    delete(observation_values);

    return probabilistic_forecast_image_filename;
}
