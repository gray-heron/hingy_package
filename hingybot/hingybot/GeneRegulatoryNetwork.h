
#pragma once

#include <inttypes.h>
#include <vector>
#include <list>
#include <array>
#include <cmath>
#include <assert.h>

#include "EvolutionaryTrainer.h"
#include "Randomizer.h"

template <int D, typename scalartype=float>
class Protein{
private:
    std::array<scalartype, D> position;
    std::array<scalartype, D> enhancement;
    std::array<scalartype, D> inhibition;

    scalartype max_d;

    scalartype DistanceTo(std::array<scalartype, D>& other) {
        scalartype sum = 0;
        for (int i = 0; i < D; i++) {
            scalartype dst = position[i] - other[i];
            sum += dst * dst;
        }
        return std::sqrt(sum);
    }

public:
    Protein(scalartype max, Randomizer<scalartype>& generator) : max_d(max) {
        for (auto& scalar : position)
            scalar = generator.RandomScalar(0, max_d);

        for (auto& scalar : enhancement)
            scalar = generator.RandomScalar(0, max_d);

        for (auto& scalar : inhibition)
            scalar = generator.RandomScalar(0, max_d);
    };

    //binary constructor ftw!
    Protein(const char* data, scalartype max) : max_d(max) {
        memcpy(this, data, sizeof(Protein<D, scalartype>));
    };

    void Mutate(Randomizer<scalartype>& generator) {
        switch (generator.RandomInt(0, 2)) {
            case 0: //modify position
                for (auto& scalar : position)
                     scalar = generator.RandomScalar(0, max_d);
            break;
            case 1: //modify enhancement
                for (auto& scalar : enhancement)
                    scalar = generator.RandomScalar(0, max_d);
            break;
            default: //modify inhibition
                for (auto& scalar : inhibition)
                    scalar = generator.RandomScalar(0, max_d);
        }
    }

    scalartype EffectOn(Protein<D, scalartype>& other, scalartype beta) {
        scalartype e = (max_d - other.DistanceTo(enhancement));
        scalartype i = (max_d - other.DistanceTo(inhibition));
        return (e*e*e - i*i*i) * beta;
    }
};

template <int D, typename scalartype = float>
class GeneRegulatoryNetwork : public Trainable<scalartype> {
private:
    std::vector<scalartype> concentrations;

    uint32_t protein_density;
    scalartype beta = 1.0f, delta = 1.0f;

    uint32_t input_proteins_n;
    uint32_t output_proteins_n;

public:
    std::vector<Protein<D, scalartype>> proteins;

    virtual void Mutate(Randomizer<scalartype>& generator) {
        uint32_t chromosome = generator.RandomInt(0, 5);

        while (proteins.size() > 20) {
            uint32_t victim = generator.RandomInt(
                input_proteins_n, proteins.size() - output_proteins_n);
            proteins.erase(proteins.begin() + victim);
            concentrations.erase(concentrations.begin() + victim);
        }

        if (chromosome) {
            do {
                uint32_t mutation_type = generator.RandomInt(0, 3);

                if (mutation_type == 0 && proteins.size() < 30) { //create protein
                    proteins.insert(proteins.begin() + input_proteins_n,
                        Protein<D, scalartype>((float)protein_density, generator));
                    concentrations.push_back(generator.RandomScalar(0.0, 1.0));
                }
                else if (
                    mutation_type == 1 &&
                    proteins.size() - output_proteins_n - input_proteins_n > 0
                    )
                { //remove protein
                    uint32_t victim = generator.RandomInt(
                        input_proteins_n, proteins.size() - output_proteins_n);
                    proteins.erase(proteins.begin() + victim);
                    concentrations.erase(concentrations.begin() + victim);
                }
                else if (mutation_type == 2) { //mutate_protein
                    uint32_t protein_index = generator.RandomInt(0, proteins.size() - 1);
                    proteins[protein_index].Mutate(generator);
                }
            } while (generator.RandomScalar(0.0f, 1.0f) < 0.825f);
        }
        else {
            uint32_t parameter = generator.RandomInt(0, 1);

            if(parameter)
                delta = generator.RandomScalar(0.1f, 10.0f);
            else
                beta = generator.RandomScalar(0.1f, 10.0f);
        }
    }

    virtual std::vector<std::shared_ptr<Trainable<scalartype>>> Crossover(Trainable<scalartype>& other, Randomizer<scalartype>& generator) {
        std::vector<std::shared_ptr<Trainable<scalartype>>> ret;

        try {
            GeneRegulatoryNetwork<D, scalartype>& other_grn =
                dynamic_cast<GeneRegulatoryNetwork<D, scalartype>&>(other);

            uint32_t crossover_marker1 = generator.RandomInt(input_proteins_n,
                proteins.size() - output_proteins_n);
            uint32_t crossover_marker2 = generator.RandomInt(input_proteins_n,
                other_grn.proteins.size() - output_proteins_n);

            typename std::vector<Protein<D, scalartype>>::iterator crossover_point1 =
                proteins.begin() + crossover_marker1;
            typename std::vector<Protein<D, scalartype>>::iterator crossover_point2 =
                other_grn.proteins.begin() + crossover_marker2;

            ret.reserve(2);

            auto child1 = std::make_shared<GeneRegulatoryNetwork<D, scalartype>>
                (input_proteins_n, output_proteins_n, protein_density, generator, false);
            auto child2 = std::make_shared<GeneRegulatoryNetwork<D, scalartype>>
                (input_proteins_n, output_proteins_n, protein_density, generator, false);

            child1->proteins.insert(child1->proteins.end(),
                proteins.begin(), crossover_point1);
            child1->proteins.insert(child1->proteins.end(),
                    crossover_point2, other_grn.proteins.end());

            child2->proteins.insert(child2->proteins.end(),
                other_grn.proteins.begin(), crossover_point2);
            child2->proteins.insert(child2->proteins.end(),
                    crossover_point1, proteins.end());

            if (generator.RandomInt(0, 2)) child1->beta = this->beta; else child1->beta = other_grn.beta;
            if (generator.RandomInt(0, 2)) child2->beta = this->beta; else child2->beta = other_grn.beta;
            if (generator.RandomInt(0, 2)) child1->delta = this->beta; else child1->delta = other_grn.delta;
            if (generator.RandomInt(0, 2)) child2->delta = this->beta; else child2->delta = other_grn.delta;

            child1->concentrations.resize(child1->proteins.size());
            child2->concentrations.resize(child2->proteins.size());

            ret.push_back(std::dynamic_pointer_cast<Trainable<scalartype>>(child1));
            ret.push_back(std::dynamic_pointer_cast<Trainable<scalartype>>(child2));
        }
        catch (std::bad_cast exception) {
            fprintf(stderr, "You can not crossover a GRN with a non-GRN trainable!\n");
        }

        return ret;
    };

    virtual std::vector<scalartype> Step(std::vector<scalartype> inputs) {
        std::vector<scalartype> new_concentrations;
        float theta = 0;
        new_concentrations.reserve(concentrations.size());
        new_concentrations.resize(input_proteins_n);

        if (inputs.size() != input_proteins_n) {
            fprintf(stderr, "Wrong number of input protein concenrations given!\n");
        }
        std::copy(inputs.begin(), inputs.end(), concentrations.begin());
        std::copy(inputs.begin(), inputs.end(), new_concentrations.begin());

        //don't change concetrations of the input proteins
        for (auto protein = proteins.begin() + input_proteins_n; protein != proteins.end(); ++protein) {
            scalartype change = 0;
            uint32_t protein_id = protein - proteins.begin();

            //don't accect output proteins as efectors
            for (auto efector = proteins.begin(); efector != proteins.end() - output_proteins_n; ++efector) {
                change += efector->EffectOn(*protein, beta) *
                    concentrations[efector - proteins.begin()] * delta;
            }

            float new_concentration = concentrations[protein_id] + change;
            if(new_concentration < 0.0f)
                new_concentration = 0.0f;

            new_concentrations.push_back(new_concentration);

            theta += new_concentration;
        }

        //do not normalize input proteins' concentrations
        if(theta != 0.0f)
            for(auto concentration = new_concentrations.begin() + input_proteins_n;
                concentration != new_concentrations.end(); ++concentration
            ){
                *concentration /= theta;
            }

        concentrations = new_concentrations;
        return std::vector<scalartype>
            (concentrations.begin() + (proteins.size() - output_proteins_n), concentrations.end());
    }

    virtual void Configure(uint32_t input_proteins_n, uint32_t output_proteins_n,
        std::vector<scalartype>& params, Randomizer<scalartype>& generator) {
        this->output_proteins_n = output_proteins_n;
        this->input_proteins_n = input_proteins_n;
        this->protein_density = (uint32_t)params[0];

        concentrations.resize(input_proteins_n + output_proteins_n);
        proteins.reserve(input_proteins_n + output_proteins_n);
        for (unsigned int i = 0; i < input_proteins_n + output_proteins_n; i++) {
            proteins.push_back(Protein<D, scalartype>(params[0], generator));
        }
    }

    virtual void Reset() {
        for (auto& concentration : concentrations) {
            concentration = 0.5f;
        }
    }

    GeneRegulatoryNetwork(uint32_t input_proteins_n, uint32_t output_proteins_n,
        uint32_t protein_density, Randomizer<scalartype>& generator,
	    bool auto_generate_io_proteins = true
    ){
        std::vector<scalartype> params;
        params.push_back((float)protein_density);

        Configure(input_proteins_n, output_proteins_n, params, generator);

        if(!auto_generate_io_proteins) {
            proteins.clear();
            concentrations.clear();
        }
    }

    virtual int GetMaxSerializedBufferSize() const {
        return proteins.size() * sizeof(Protein<D, scalartype>) +
            2 * sizeof(scalartype) + //delta, beta
            5 * sizeof(uint32_t); //density, inputs_n, outputs_n, regulatory_n, D
    }

    virtual std::vector<uint8_t> Serialize() const {
        std::vector<uint8_t> data_vector;
        data_vector.resize(GetMaxSerializedBufferSize());
        uint8_t* cursor = data_vector.data();

        uint32_t d = D;
        uint32_t regulatory_proteins_n = proteins.size() - input_proteins_n - output_proteins_n;

        memcpy(cursor, (void*)&input_proteins_n,      sizeof(uint32_t)); cursor += sizeof(uint32_t);
        memcpy(cursor, (void*)&output_proteins_n,     sizeof(uint32_t)); cursor += sizeof(uint32_t);
        memcpy(cursor, (void*)&regulatory_proteins_n, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        memcpy(cursor, (void*)&protein_density,       sizeof(uint32_t)); cursor += sizeof(uint32_t);
        memcpy(cursor, (void*)&d,                     sizeof(uint32_t)); cursor += sizeof(uint32_t);

        memcpy(cursor, (void*)&delta, sizeof(uint32_t)); cursor += sizeof(scalartype);
        memcpy(cursor, (void*)&beta, sizeof(uint32_t));  cursor += sizeof(scalartype);

        memcpy(cursor, proteins.data(), proteins.size() * sizeof(Protein<D, scalartype>));
        cursor += proteins.size() * sizeof(Protein<D, scalartype>);

        assert(cursor - data_vector.data() <= GetMaxSerializedBufferSize());
        data_vector.resize(cursor - data_vector.data());

        return data_vector;
    }

    virtual void Deserialize(const std::vector<uint8_t>& data_vector) {
        uint32_t d;
        uint32_t regulatory_proteins_n;
        const uint8_t* cursor = data_vector.data();

        memcpy((void*)&input_proteins_n,      cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        memcpy((void*)&output_proteins_n,     cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        memcpy((void*)&regulatory_proteins_n, cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        memcpy((void*)&protein_density,       cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        memcpy((void*)&d,                     cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

        memcpy((void*)&delta, cursor, sizeof(uint32_t)); cursor += sizeof(scalartype);
        memcpy((void*)&beta,  cursor, sizeof(uint32_t)); cursor += sizeof(scalartype);

        assert(d == D);

        concentrations.resize(input_proteins_n + output_proteins_n + regulatory_proteins_n);
        for (int i = 0; i < concentrations.size(); i++) {
            proteins.push_back(Protein<D, scalartype>((const char*)cursor, protein_density));
            cursor += sizeof(Protein<D, scalartype>);
        }
    }

    GeneRegulatoryNetwork() {}
};
