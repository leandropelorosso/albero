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

std::string Commands::GetAnalogViewer::Execute(Albero2* albero2, rapidjson::Document &document)
{
    string s_lat = document["lat"].GetString();
    float lat = ::atof(s_lat.c_str());

    string s_lon = document["lon"].GetString();
    float lon = ::atof(s_lon.c_str());

    string s_range_index = document["range_index"].GetString();
    int range_index = ::atoi(s_range_index.c_str());


    if (lat < albero2->forecasts->lats[0] + 1 ||
        lat > albero2->forecasts->lats[albero2->forecasts->NLAT - 1] - 1 ||
        lon < albero2->forecasts->lons[0] + 1 ||
        lon > albero2->forecasts->lons[albero2->forecasts->NLON - 1] - 1) {
        return "[]";
    }

    AnalogsResponse* analogs = albero2->RenderAnalogForecasts(lat, lon, range_index);

    string result = "[" + analogs->ToJSON() + "]";

    delete(analogs);

    // return the result
    return result;
}
