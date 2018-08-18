#pragma once

#include <algorithm>
#include <assert.h>
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <vector>

#include "Randomizer.h"

#define THREADS_N 1
#define PPP 0.35f
#define POPULATION_OVERHEAD 2
#define INITIAL_MUTATIONS_N 2

using std::vector;
using std::pair;
using std::function;
using std::shared_ptr;

template <typename scalartype> class Trainable
{
  public:
    virtual void Mutate(Randomizer<scalartype> &generator) = 0;
    virtual vector<shared_ptr<Trainable<scalartype>>>
    Crossover(Trainable<scalartype> &other,
              Randomizer<scalartype> &generator) = 0;

    virtual vector<scalartype> Step(vector<scalartype> inputs) = 0;
    virtual void Reset() = 0;

    virtual void Configure(uint32_t inputs_n, uint32_t outputs_n,
                           vector<scalartype> &parameters,
                           Randomizer<scalartype> &generator) = 0;

    virtual int GetMaxSerializedBufferSize() const = 0;
    virtual std::vector<uint8_t> Serialize()
        const = 0; // after using Serialize(), Configure() not is required
    virtual void Deserialize(const std::vector<uint8_t> &) = 0;
};

template <class TTrainable, typename scalartype = float> class Trainer
{
    static_assert(std::is_base_of<Trainable<scalartype>, TTrainable>::value,
                  "You can only train Trainables :)");

    typedef typename vector<
        pair<shared_ptr<Trainable<scalartype>>, scalartype>>::iterator marker_t;
    typedef pair<marker_t, marker_t> population_range_t;

    int population_size, inputs_n, outputs_n, population_per_thread;
    int alphas_n; // breeding individuals count
    float mutation_rate, crossover_rate;
    scalartype max_fitness, mean_fitness;

    vector<scalartype> params;
    function<scalartype(shared_ptr<Trainable<scalartype>>)> fitness_function;

    vector<pair<shared_ptr<Trainable<scalartype>>, scalartype>> population;

    static int
    CompareFitness(pair<shared_ptr<Trainable<scalartype>>, scalartype> a,
                   pair<shared_ptr<Trainable<scalartype>>, scalartype> b)
    {
        return a.second > b.second;
    }

  public:
    shared_ptr<TTrainable> GetLeader()
    {
        return std::dynamic_pointer_cast<TTrainable>(population[0].first);
    }

    // returns leader's fitness
    scalartype Generation(Randomizer<scalartype> &generator)
    {
        vector<std::thread> threads;
        vector<vector< // each thread produces children into these subvectors
            pair<shared_ptr<Trainable<scalartype>>, scalartype>>>
            newborns;

        newborns.resize(THREADS_N);

        auto worker_function = [this, &newborns, &generator](
            int thread_id, population_range_t range) mutable -> void {
            for (auto i = range.first; i != range.second; i++)
            {
                if (crossover_rate >= generator.RandomScalar(0.0, 1.0))
                {
                    auto partner_id =
                        generator.RandomInt(0, this->population.size());
                    auto &partner = this->population[partner_id].first;

                    for (const auto &child :
                         i->first->Crossover(*partner, generator))
                    {
                        child->Reset();
                        newborns[thread_id].push_back(
                            std::make_pair<shared_ptr<Trainable<scalartype>>,
                                           scalartype>(
                                std::shared_ptr<Trainable<scalartype>>(child),
                                fitness_function(child)));
                    }
                }

                if (mutation_rate >= generator.RandomScalar(0.0, 1.0))
                {
                    std::shared_ptr<Trainable<scalartype>> new_child =
                        std::make_shared<TTrainable>(
                            dynamic_cast<TTrainable &>(*i->first));
                    new_child->Mutate(generator);

                    new_child->Reset();
                    newborns[thread_id].push_back(
                        std::make_pair<shared_ptr<Trainable<scalartype>>,
                                       scalartype>(
                            std::shared_ptr<Trainable<scalartype>>(new_child),
                            fitness_function(new_child)));
                }
            }

            sort(newborns[thread_id].begin(), newborns[thread_id].end(),
                 CompareFitness);
        };

        for (int thread_id = 0; thread_id < THREADS_N; thread_id++)
        {
            threads.push_back(std::thread(
                worker_function, thread_id,
                std::make_pair<marker_t, marker_t>(
                    population.begin() + ((thread_id)*population_per_thread),
                    population.begin() +
                        ((thread_id + 1) * population_per_thread))));
        }

        for (int i = 0; i < THREADS_N; i++)
            threads[i].join();

        auto children_deploy_cursor = population.rbegin();
        for (const auto &children : newborns)
        {
            for (auto child = children.begin();
                 //                      \/ ------ ensure we won't substitute
                 //                      the alphas ---------
                 child != children.end(); child++)
            {
                *children_deploy_cursor = *child;
                if (children_deploy_cursor == population.rend())
                    break;
                ++children_deploy_cursor;
            }
        }

        std::sort(population.begin(), population.end(), CompareFitness);

        return population.begin()->second;
    }

    void Purge(scalartype alphas_f, Randomizer<scalartype> &generator)
    {
        vector<std::thread> threads;
        uint32_t alphas_n = int(scalartype(population_size) * alphas_f);
        uint32_t to_wipe_out_n = population_size - alphas_n;
        uint32_t newborns_per_thread =
            int(float(to_wipe_out_n) / float(THREADS_N));

        auto worker_function =
            [this, generator](population_range_t range) mutable -> void {
            for (auto i = range.first; i != range.second; i++)
            {
                i->first = std::make_shared<TTrainable>();
                i->first->Configure(inputs_n, outputs_n, params, generator);
                i->second = fitness_function(i->first);
            }
        };

        for (int thread_id = 0; thread_id < THREADS_N - 1; thread_id++)
        {
            threads.push_back(
                std::thread(worker_function,
                            std::make_pair<marker_t, marker_t>(
                                population.begin() + alphas_n +
                                    ((thread_id)*newborns_per_thread),
                                population.begin() + alphas_n +
                                    ((thread_id + 1) * newborns_per_thread))));
        }

        worker_function(std::make_pair<marker_t, marker_t>(
            population.begin() + alphas_n +
                (THREADS_N - 1) * newborns_per_thread,
            population.end()));

        for (int i = 0; i < THREADS_N - 1; i++)
            threads[i].join();

        std::sort(population.begin(), population.end(), CompareFitness);
    }

    Trainer(uint32_t population_size, uint32_t inputs_n, uint32_t outputs_n,
            float mutation_rate, float crossover_rate,
            function<scalartype(shared_ptr<Trainable<scalartype>>)>
                fitness_function,
            vector<scalartype> &params, Randomizer<scalartype> &generator)
        : inputs_n(inputs_n), outputs_n(outputs_n),
          crossover_rate(crossover_rate), mutation_rate(mutation_rate),
          population_size(population_size), fitness_function(fitness_function),
          params(params)
    {
        population.reserve(population_size * POPULATION_OVERHEAD);

        for (unsigned int i = 0; i < population_size; i++)
        {
            population.push_back(
                std::make_pair<shared_ptr<Trainable<scalartype>>, scalartype>(
                    std::make_shared<TTrainable>(), 0.0));

            (population.end() -
             1)->first->Configure(inputs_n, outputs_n, params, generator);

            for (int i = 0; i < INITIAL_MUTATIONS_N; i++)
                (population.end() - 1)->first->Mutate(generator);

            (population.end() - 1)->second =
                fitness_function((population.end() - 1)->first);
        }

        std::sort(population.begin(), population.end(), CompareFitness);

        population_per_thread =
            int(float(population.size()) / float(THREADS_N) * PPP);
        alphas_n = int(float(population.size()) * PPP);
    }

    Trainer(char *buf, function<scalartype(shared_ptr<Trainable<scalartype>>)>
                           fitness_function)
        : fitness_function(fitness_function)
    {
        uint32_t cursor = 0;

        memcpy((void *)&population_size, &buf[cursor], sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy((void *)&inputs_n, &buf[cursor], sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        memcpy((void *)&outputs_n, &buf[cursor], sizeof(uint32_t));
        cursor += sizeof(uint32_t);

        memcpy((void *)&mutation_rate, &buf[cursor], sizeof(scalartype));
        cursor += sizeof(scalartype);
        memcpy((void *)&crossover_rate, &buf[cursor], sizeof(scalartype));
        cursor += sizeof(scalartype);

        population.reserve(population_size);

        for (int i = 0; i < population_size; i++)
        {
            population.push_back(
                std::make_pair<shared_ptr<Trainable<scalartype>>, scalartype>(
                    std::make_shared<TTrainable>(), 0.0));

            cursor += (population.end() - 1)->first->Deserialize(&buf[cursor]);
            memcpy((void *)&(population.end() - 1)->second, &buf[cursor],
                   sizeof(scalartype));
            cursor += sizeof(scalartype);
        }

        std::sort(population.begin(), population.end(), CompareFitness);

        population_per_thread =
            int(float(population.size()) / float(THREADS_N) * PPP);
        alphas_n = int(float(population.size()) * PPP);

        assert(population_size % THREADS_N == 0);
    }

    // FIXME rewrite the binary serialization
    /*
    virtual uint32_t Serialize(char* buf) const {
        uint32_t cursor = 0;

        memcpy(&buf[cursor], (void*)&population_size, sizeof(uint32_t)); cursor
    += sizeof(uint32_t);
        memcpy(&buf[cursor], (void*)&inputs_n, sizeof(uint32_t)); cursor +=
    sizeof(uint32_t);
        memcpy(&buf[cursor], (void*)&outputs_n, sizeof(uint32_t)); cursor +=
    sizeof(uint32_t);

        memcpy(&buf[cursor], (void*)&mutation_rate, sizeof(scalartype)); cursor
    += sizeof(scalartype);
        memcpy(&buf[cursor], (void*)&crossover_rate, sizeof(scalartype)); cursor
    += sizeof(scalartype);

        for (const auto& i : population) {
            cursor += i.first->Serialize(&buf[cursor]);
            memcpy(&buf[cursor], (void*)&i.second, sizeof(scalartype)); cursor
    += sizeof(scalartype);
        }

        return cursor;
    }

    virtual int GetMaxSerializedBufferSize() const {
        uint32_t ret = 0;
        for (const auto& i : population) {
            ret += i.first->GetMaxSerializedBufferSize();
        }

        ret += sizeof(uint32_t); //population size
        ret += 2 * sizeof(uint32_t); //inputs, outputs _n
        ret += 2 * sizeof(float); // rates

        return ret;
    }
    */
};
