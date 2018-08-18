#pragma once

#include <map>
#include <string>

typedef std::map<std::string, std::string> stringmap;

void log_error(std::string msg);
void log_warning(std::string msg);
void log_info(std::string msg);