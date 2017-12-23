#include "command.h"
#include <string>
#include "rapidjson/document.h"
#include "../albero2.h"

using namespace Commands;

//extern int times_in_range; // from the configuration, how many times (6 hour slot) in a range
//extern int accumulation_ranges; // amount of accumulation ranges
extern string albero_images_path;

std::string Initialize::Execute(Albero2* albero, rapidjson::Document& document){

    string analog_range_from = document["configuration"]["analog-range-from"].GetString();
    string analog_range_to = document["configuration"]["analog-range-to"].GetString();
    string date = document["configuration"]["date"].GetString();
    string analogs_amount = document["configuration"]["analogs-amount"].GetString();
    albero->N_ANALOGS_PER_LAT_LON = stoi(analogs_amount);
    albero->threshold_ranges.clear();
    albero->accumulation_ranges.clear();

    // Read Accumulation Ranges
    if (document["configuration"].HasMember("accumulation-ranges")) {
        const rapidjson::Value& ranges = document["configuration"]["accumulation-ranges"];
        for (int i = 0; i < (int)ranges.Size(); i++) {
            const rapidjson::Value& range = ranges[i];
            albero->accumulation_ranges.push_back(AccumulationRange(stoi(range[0].GetString()), stoi(range[1].GetString())));
        }
    }


    if (document["configuration"].HasMember("threshold-ranges")) {

        const rapidjson::Value& ranges = document["configuration"]["threshold-ranges"];

        for (int i = 0; i < (int)ranges.Size(); i++) {
            albero->threshold_ranges.push_back(ThresholdRange(stoi(ranges[i].GetString()), 100));
        }
    }

    albero->Initialize(stoi(date)*100);

    return albero->stats->ToJSON();
}

