#pragma once
#include "EvolutionaryTrainer.h"

class ParamSeeker : public Trainable<float> {
public:
    std::vector<float> end_params;
    std::vector<float> evo_params;

    virtual void Mutate(Randomizer<float>& generator) override;

    virtual vector<shared_ptr<Trainable<float>>> Crossover(Trainable<float>& other, Randomizer<float>& generator) override;

    virtual vector<float> Step(vector<float> inputs) override
    {
        return vector<float>();
    }

    virtual void Reset() override
    {
    }

    virtual void Configure(uint32_t inputs_n, uint32_t outputs_n, vector<float>& parameters, Randomizer<float>& generator) override;
    virtual int GetMaxSerializedBufferSize() const override;
    virtual std::vector<uint8_t> Serialize() const override;
    virtual void Deserialize(const std::vector<uint8_t>&) override;

    void WriteParams(std::string name);
    void ReadParams(std::string name);
};