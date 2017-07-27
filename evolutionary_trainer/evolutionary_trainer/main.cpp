// GRNTrainer.cpp : Defines the entry point for the console application.
//

#include <array>
#include <fstream>
#include <sstream>
#include <string>
#include <boost/range/combine.hpp>
#include <boost/algorithm/string.hpp>

#include "utils.h"
#include "EvolutionaryTrainer.h"
#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"


using namespace rapidxml;
using std::string;using std::string;

enum ParamsNames {
    ParamMutationChance = 0, MutationPower
};

const string tester_path = "../../TORCSTester/TORCSTester/";
const string tester_executable = "bin/Debug/TORCSTester.exe";
const string tester_profile = "cases/learn.xml";
const string tester_options = " 3";

const string hingybot_path = "../../hingybot/hingybot/";

const string individual_being_evaluated = "params/current.xml";
const string individual_best = "params/best.xml";
const string individual_initial = "params/initial.xml";

class ParamSeeker : public Trainable<float> {
public:
    std::vector<float> end_params;
    std::vector<float> evo_params;

    virtual void Mutate(Randomizer<float>& generator) override
    {
        for (int i = 0; i < end_params.size(); i++) {
            if (generator.RandomScalar(0.0f, end_params.size() / evo_params[ParamsNames::ParamMutationChance]) < 1.0f)
                end_params[i] *= generator.RandomScalar(1.0f - evo_params[ParamsNames::MutationPower], 1.0f + evo_params[ParamsNames::MutationPower]);
        }
    }

    virtual vector<shared_ptr<Trainable<float>>> Crossover(Trainable<float>& other, Randomizer<float>& generator) override
    {
        std::vector<std::shared_ptr<Trainable<float>>> ret;

        try {
            ParamSeeker& other_seeker = dynamic_cast<ParamSeeker&>(other);
            auto child1 = std::make_shared<ParamSeeker>();
            auto child2 = std::make_shared<ParamSeeker>();

            for (const auto& tup : boost::combine(end_params, other_seeker.evo_params)) {
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

            ret.push_back(std::dynamic_pointer_cast<Trainable<float>>(child1));
            ret.push_back(std::dynamic_pointer_cast<Trainable<float>>(child2));
        }
        catch (std::bad_cast exception) {
            fprintf(stderr, "You can not crossover a GRN with a non-GRN trainable!\n");
        }

        return ret;
    }

    virtual vector<float> Step(vector<float> inputs) override
    {
        return vector<float>();
    }

    virtual void Reset() override
    {
    }

    virtual void Configure(uint32_t inputs_n, uint32_t outputs_n, vector<float>& parameters, Randomizer<float>& generator) override
    {
        evo_params.resize(2);
        end_params.resize(parameters.size() - 2);
        std::copy(parameters.begin(), parameters.begin() + 2, evo_params.begin());
        std::copy(parameters.begin() + 2, parameters.end(), end_params.begin());
    }

    virtual int GetMaxSerializedBufferSize() const override
    {
        return end_params.size() * sizeof(float);
    }

    virtual uint32_t Serialize(char * buf) const override
    {
        uint32_t params_n = end_params.size();
        memcpy(buf, &params_n, sizeof(uint32_t));
        memcpy(buf + sizeof(uint32_t), end_params.data(), GetMaxSerializedBufferSize());
        return GetMaxSerializedBufferSize() + sizeof(uint32_t);
    }

    virtual uint32_t Deserialize(const char * buf) override
    {
        uint32_t params_n;
        memcpy(&params_n, buf, sizeof(uint32_t));
        end_params.resize(params_n);
        memcpy(end_params.data(), buf + sizeof(uint32_t), params_n * sizeof(uint32_t));
        return params_n * sizeof(uint32_t) + sizeof(uint32_t);
    }
};

std::string exec() {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen((tester_path + tester_executable + " " + tester_path + " " + tester_profile + " " + tester_options ).c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != NULL)
            result += buffer.data();
    }
    return result;
}

void WriteParams(std::vector<float>& params, string name) {
    std::fstream fs;
    fs.open((hingybot_path + name).c_str(), std::fstream::out | std::fstream::trunc);

    xml_document <> doc;
    xml_node <> * main_node = doc.allocate_node(node_element, "hingybot_params");

    std::vector<string> tmp;
    for (auto& param : params) {
        tmp.push_back(std::to_string(param));
    }

    main_node->append_node(doc.allocate_node(node_element, "sa",            tmp[0].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "sb",            tmp[1].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "sc",            tmp[2].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "speed_base",    tmp[3].c_str()));
    main_node->append_node(doc.allocate_node(node_element, "speed_factor",  tmp[4].c_str()));

    doc.append_node(main_node);
    fs << doc;
    fs.close();
}

void ReadParams(std::vector<float>& params) {
    int size = file_size(hingybot_path + individual_initial);
    char* buf = new char[size + 1];
    xml_document <> doc;
    FILE *f = fopen((hingybot_path + individual_initial).c_str(), "rb");

    while (ftell(f) != size) {
        /* printf("%d\n", */fread(&buf[ftell(f)], size, 1, f);
    }

    fclose(f);
    buf[size] = '\0';

    doc.parse<0>(buf);
    auto main_node = doc.first_node();
    assert(strcmp(main_node->name(), "hingybot_params") == 0);
    auto param_node = main_node->first_node();

    params.resize(5);

    while (param_node != nullptr) {
        if (strcmp(param_node->name(), "sa") == 0)
            params[0] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "sb")) == 0)
            params[1] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "sc")) == 0)
            params[2] = atof(param_node->value());
        else if ((strcmp(param_node->name(), "speed_base")) == 0)
            params[3] = atoi(param_node->value());
        else if ((strcmp(param_node->name(), "speed_factor")) == 0)
            params[4] = atoi(param_node->value());
        else if ((strcmp(param_node->name(), "fshift")) == 0) {}
        else if ((strcmp(param_node->name(), "speed_base")) == 0) {}
        else if ((strcmp(param_node->name(), "speed_factor")) == 0) {}
        else
            assert(false);

        param_node = param_node->next_sibling();
    }

    delete[] buf;
}

float best = 0.0;

float ParamsFitness(std::shared_ptr<Trainable<float>> seeker) {
    float fitness;
    auto tmp_seeker = std::dynamic_pointer_cast<ParamSeeker>(seeker);
    WriteParams(tmp_seeker->end_params, individual_being_evaluated);
    string out = exec();

    vector<string> lines;
    boost::algorithm::split(lines, out, boost::is_any_of("\n"));

    for (auto& line : lines) {
        if (strncmp(line.c_str(), "Mean ref:", 7) == 0)
            sscanf(line.c_str(), "Mean ref: %f", &fitness);
    }

    if (-fitness > best) {
        best = -fitness;
        WriteParams(tmp_seeker->end_params, individual_best);
        printf("New best! %f\n", -fitness);
    }

    return -fitness;
}


int main()
{
    vector<float> params;
    ReadParams(params);
    vector<float> full_params;
    full_params.resize(7);
    full_params[ParamsNames::MutationPower] = 0.3f;
    full_params[ParamsNames::ParamMutationChance] = 2.0f;
    std::copy(params.begin(), params.end(), full_params.begin() + 2);

    auto gen = Randomizer<float>(2);
    auto trainer = new Trainer<ParamSeeker>(12, 0, 0, 1.0f, 1.0f, ParamsFitness, full_params, gen) ;

    while (true) {
        printf("Gene %f\n", trainer->Generation(gen));
    }

    printf("%s", exec().c_str());
    return 0;
}

void log_error(std::string msg)
{
    fprintf(stderr, "[ERROR]   %s\n", msg.c_str());
    exit(1);
}

void log_warning(std::string msg)
{
    fprintf(stdout, "[WARNING] %s\n", msg.c_str());
}

void log_info(std::string msg)
{
    fprintf(stdout, "[INFO]\t  %s\n", msg.c_str());
}
