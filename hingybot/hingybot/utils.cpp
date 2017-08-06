
#include <cstring>
#include <fstream>

#include "utils.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

using std::string;
using namespace rapidxml;

bool parse_arguments(string name_prefix, char name_value_sep,
    int argc, char ** argv, stringmap& out)
{
    bool clean = true;

    for (int arg_i = 0; arg_i < argc; arg_i++) {
        string current_argument(argv[arg_i]), current_name, current_value;
        int cursor = name_prefix.length();

        if (name_prefix != "" && current_argument.find(name_prefix) != 0) {
            log_warning((string)"Wrong prefix on argument: " + current_argument + "!");
            clean = false;
            continue;
        }

        while (cursor < current_argument.length() &&
            current_argument[cursor] != name_value_sep)
        {
            current_name += current_argument[cursor];
            cursor++;
        }

        if (cursor == current_argument.length()) {
            log_warning((string)"No separator on argument: " + current_argument + "!");
            clean = false;
            continue;
        }

        if (out.find(current_name) != out.end()) {
            log_warning((string)"Argument repetition: " + current_name + "!");
            clean = false;
        }

        for (cursor += 1; cursor < current_argument.length(); cursor++)
            current_value += current_argument[cursor];

        out[current_name] = current_value;

    }

    return clean;
}

bool load_params_from_xml(string filename, string main_node_name,
    stringmap& out)
{
    if (file_exists(filename)) {
        int size = file_size(filename);
        char* buf = new char[size + 1];
        xml_document <> doc;
        FILE *f = fopen(filename.c_str(), "rb");

        if (f == nullptr)
            return false;

        while (ftell(f) != size) {
            fread(&buf[ftell(f)], size, 1, f);
        }

        fclose(f);
        buf[size] = '\0';

        doc.parse<0>(buf);
        auto main_node = doc.first_node();
        assert(strcmp(main_node->name(), main_node_name.c_str()) == 0);

        for (auto param_node = main_node->first_node(); param_node != nullptr;
            param_node = param_node->next_sibling())
        {
            string param_name = param_node->name();

            if (out.find(param_name) != out.end()) {
                log_info((string)"Ignoring " + param_name + " from the xml parameters!");
                continue;
            }

            out[param_name] = param_node->value();
        }

        return true;
    }

    return false;
}

bool file_exists(string name) {
    FILE * f = fopen(name.c_str(), "rb");
    if (f) {
        fclose(f);
        return true;
    }

    return false;
}

//https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
size_t file_size(std::string name)
{
    struct stat stat_buf;
    int rc = stat(name.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}
