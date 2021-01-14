#pragma once

#include "json11/json11.hpp"

using namespace json11;

void parse_browser_message(const std::string &name, Json &json);
