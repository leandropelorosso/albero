#include "command.h"
#include <string>
#include "rapidjson/document.h"
#include "../albero2.h"

using namespace Commands;

extern string reforecast_file_path;

// Returns the available reforecast dates in JSON format.
std::string GetAvailableDays::Execute(Albero2* albero, rapidjson::Document& document){

    // Create forecast reader and initialize it
    ForecastReader *forecasts = new ForecastReader();
    forecasts->Initialize(reforecast_file_path);

    std::cout << forecasts->NTIME << endl;

    // Create JSON from days
    std::string result = "[";
    for(int i=0; i<forecasts->NTIME; i++){
            result += "\"" + to_string(forecasts->forecast_days[i]) + "\"";
            if(i<forecasts->NTIME-1) result += ",";
    }
    result += "]";

    // Delete forecasts
    delete(forecasts);

    // Return result
    return result;

}

