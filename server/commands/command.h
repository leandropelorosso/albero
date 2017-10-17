#include <string>
#include "../albero2.h"
#include "rapidjson/document.h"

namespace Commands{

    // Command Interface
    class Command
    {
    public:
        virtual std::string Execute(Albero2* albero, rapidjson::Document& document) = 0;
    };


    // Initialize
    class Initialize : public Command
    {
    public:
        std::string Execute(Albero2* albero, rapidjson::Document& document);
    };


    // Ping
    class Ping : public Command
    {
    public:
        std::string Execute(Albero2* albero, rapidjson::Document& document);
    };


    // Retrieve Observation Tile (get_observation)
    class GetObservation : public Command
    {
    public:
        std::string Execute(Albero2* albero, rapidjson::Document& document);
    };

    // Retrieve Probabilistic Forecast Tile (get_forecast)
    class GetProbabilisticForecast: public Command
    {
    public:
        std::string Execute(Albero2* albero, rapidjson::Document& document);
    };

    // Retrieve Numerical Forecast Tile (get_num_forecast)
    class GetNumericalForecast: public Command
    {
    public:
        std::string Execute(Albero2* albero, rapidjson::Document& document);
    };

    // Retrieve MSE Tile (get_mse)
    class GetMSE: public Command
    {
    public:
        std::string Execute(Albero2* albero, rapidjson::Document& document);
    };

    // Retrieve Analog Viewer (get_analog_viewer)
    class GetAnalogViewer: public Command
    {
    public:
        std::string Execute(Albero2* albero, rapidjson::Document& document);
    };
}
