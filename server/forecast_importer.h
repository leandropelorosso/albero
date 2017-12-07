#include <string>
#include <math.h>
#include <map>
#include <list>

#pragma once
class ForecastImporter
{
public:
    ForecastImporter();
    ~ForecastImporter();

    void Import(std::string day_file, std::string collection_file);
};
