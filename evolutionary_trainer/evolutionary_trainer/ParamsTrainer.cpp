#include <array>
#include <fstream>
#include <sstream>
#include <string>

#include <boost/range/combine.hpp>

#include "utils.h"
#include "EvolutionaryTrainer.h"
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"

#include "ParamsTrainer.h"

using namespace rapidxml;
using std::string;

enum ParamsNames {
    ParamMutationChance = 0, MutationPower
};


void ParamSeeker::Mutate(Randomizer<float>& generator)
{
    for (int i = 0; i < end_params.size(); i++) {
        if (generator.RandomScalar(0.0f, end_params.size() / evo_params[ParamsNames::ParamMutationChance]) < 1.0f)
            end_params[i] *= generator.RandomScalar(1.0f - evo_params[ParamsNames::MutationPower], 1.0f + evo_params[ParamsNames::MutationPower]);
    }
}

vector<shared_ptr<Trainable<float>>> ParamSeeker::Crossover(Trainable<float>& other, Randomizer<float>& generator)
{
    std::vector<std::shared_ptr<Trainable<float>>> ret;

    try {
        ParamSeeker& other_seeker = dynamic_cast<ParamSeeker&>(other);
        auto child1 = std::make_shared<ParamSeeker>();
        auto child2 = std::make_shared<ParamSeeker>();

        for (const auto& tup : boost::combine(end_params, other_seeker.end_params)) {
            if (generator.RandomScalar(0.0f, 1.0f) < 0.5f) {
                child1->end_params.push_back(boost::get<0>(tup));
                child2->end_params.push_back(boost::get<1>(tup));
            }
            else
            {
                child1->end_params.push_back(boost::get<1>(tup));
                child2->end_params.push_back(boost::get<0>(tup));
            }
        }

        child1->evo_params = std::vector<float>(evo_params);
        child2->evo_params = std::vector<float>(other_seeker.evo_params);
        child1->Mutate(generator);
        child2->Mutate(generator);

        ret.push_back(std::dynamic_pointer_cast<Trainable<float>>(child1));
        ret.push_back(std::dynamic_pointer_cast<Trainable<float>>(child2));
    }
    catch (std::bad_cast exception) {
        fprintf(stderr, "You can not crossover a GRN with a non-GRN trainable!\n");
    }

    return ret;
}

void ParamSeeker::WriteParams(string name) {
    std::fstream fs;
    fs.open(name.c_str(), std::fstream::out | std::fstream::trunc);

    xml_document <> doc;
    xml_node <> * main_node = doc.allocate_node(node_element, "hingybot_params");

    std::vector<string> tmp;
    for (auto& param : end_params) {
        tmp.push_back(std::to_string(param));
    }

    main_node->append_node(doc.allocate_node(node_element, "sa", tmp[0].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "sb", tmp[1].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "sc", tmp[2].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "speed_base", tmp[3].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "speed_factor", tmp[4].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "master_output_factor", tmp[5].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "steering_factor", tmp[6].c_str()));

    doc.append_node(main_node);
    fs << doc;
    fs.close();
}

void  ParamSeeker::ReadParams(string name) {
    int size = file_size(name);
    char* buf = new char[size + 1];
    xml_document <> doc;
    FILE *f = fopen((name).c_str(), "rb");

    while (ftell(f) != size) {
        /* printf("%d\n", */fread(&buf[ftell(f)], size, 1, f);
    }

    fclose(f);
    buf[size] = '\0';

    doc.parse<0>(buf);
    auto main_node = doc.first_node();
    assert(strcmp(main_node->name(), "hingybot_params") == 0);
    auto param_node = main_node->first_node();

    end_params.resize(7);

    while (param_node != nullptr) {
        if (strcmp(param_node->name(), "sa") == 0)
            end_params[0] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "sb")) == 0)
            end_params[1] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "sc")) == 0)
            end_params[2] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "speed_base")) == 0)
            end_params[3] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "speed_factor")) == 0)
            end_params[4] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "master_output_factor")) == 0)
            end_params[5] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "steering_factor")) == 0)
            end_params[6] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "fshift")) == 0) {}
        else if ((strcmp(param_node->name(), "speed_base")) == 0) {}
        else if ((strcmp(param_node->name(), "speed_factor")) == 0) {}
        else
            assert(false);

        param_node = param_node->next_sibling();
    }

    delete[] buf;
}

void  ParamSeeker::Configure(uint32_t inputs_n, uint32_t outputs_n, vector<float>& parameters, Randomizer<float>& generator)
{
    evo_params.resize(2);
    end_params.resize(parameters.size());
    std::copy(parameters.begin(), parameters.end(), end_params.begin());

    evo_params[ParamsNames::MutationPower] = 0.02f;
    evo_params[ParamsNames::ParamMutationChance] = 2.5f;
}

int ParamSeeker::GetMaxSerializedBufferSize() const
{
    return end_params.size() * sizeof(float);
}

std::vector<uint8_t> ParamSeeker::Serialize() const
{
    std::vector<uint8_t> data_vector;
    data_vector.resize(GetMaxSerializedBufferSize());

    uint32_t params_n = end_params.size();
    memcpy(data_vector.data(), &params_n, sizeof(uint32_t));
    memcpy(data_vector.data() + sizeof(uint32_t), end_params.data(), 
        GetMaxSerializedBufferSize());

    return data_vector;
}

void ParamSeeker::Deserialize(const std::vector<uint8_t>& data_vector)
{
    uint32_t params_n;
    memcpy(&params_n, data_vector.data(), sizeof(uint32_t));
    end_params.resize(params_n);
    memcpy(end_params.data(), data_vector.data() + sizeof(uint32_t), params_n * sizeof(uint32_t));
}