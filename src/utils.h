#pragma once
#include <map>
#include <string>
#include <vector>

#include "main.h"

bool parse_arguments(std::string name_prefix, char name_value_sep,
    int argc, char ** argv, stringmap& out);

bool load_params_from_xml(std::string filename, std::string main_node,
    stringmap& out);

bool file_exists(std::string name);
size_t file_size(std::string name);