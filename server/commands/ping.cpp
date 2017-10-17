#include "command.h"
#include <string>
#include "rapidjson/document.h"
#include "../albero2.h"

using namespace Commands;

std::string Ping::Execute(Albero2* albero, rapidjson::Document& document){
    return "pong";
}

