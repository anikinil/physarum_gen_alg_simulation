#include "gen_alg.hpp"
#include "physarum.hpp"

#include <vector>
#include <iostream>
#include <random>
#include <cstdint>
#include <numeric>

#include <fstream>

#include <filesystem>
#include <regex>

#include <chrono>
#include <algorithm>

using namespace std;

vector<unique_ptr<World>> generateInitialPopulation() {
    vector<unique_ptr<World>> population;

    for (int i = 0; i < POPULATION_SIZE; i++) {

        vector<unique_ptr<Junction>> junctions;
        junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, 50.0}));
        vector<unique_ptr<FoodSource>> foodSources;
        foodSources.push_back(make_unique<FoodSource>(FoodSource{500.0, 0.0, 100.0, 100.0}));

        auto world = make_unique<World>(Genome(), std::move(junctions), std::move(foodSources));
        population.push_back(std::move(world));
    }

    return population;
}

void saveGenomeAndFitness(const Genome& genome, double fitness, double averageFitness, int generation) {
    std::ofstream file("data/genome_fitness.csv", std::ios::app);
    if (file.tellp() == 0) file << "generation;best_fitness;average_fitness;genome\n";

    file << generation << ";" << fitness << ";" << averageFitness << ";";

    // Serialize genome14
    for (const auto& layer_weights : genome.growNetWeights) {
        for (const auto& weight : layer_weights) {
            file << weight << " ";
        }
    }
    for (const auto& layer_weights : genome.flowNetWeights) {
        for (const auto& weight : layer_weights) {
            file << weight << " ";
        }
    }
    file << "\n";
}

void sortByFitness(vector<unique_ptr<World>>& population) {
    std::sort(population.begin(), population.end(),
    [](const unique_ptr<World>& a, const unique_ptr<World>& b) {
        return a->fitness > b->fitness;
    });
}

void crossOverGenomes(const Genome& parent1, const Genome& parent2, Genome& child) {

    float crossoverPointGrowth = Random::uniform(0.0f, 1.0f);
    float crossoverPointFlow = Random::uniform(0.0f, 1.0f);

    // Crossover growNetWeights by calculating the average of parents' weights for each weight
    child.growNetWeights.clear();
    for (size_t i = 0; i < parent1.growNetWeights.size(); ++i) {
        const auto& layer1 = parent1.growNetWeights[i];
        const auto& layer2 = parent2.growNetWeights[i];
        vector<double> childLayer;
        for (size_t j = 0; j < layer1.size(); ++j) {
            childLayer.push_back((crossoverPointGrowth * layer1[j] + (1 - crossoverPointGrowth) * layer2[j]));
        }
        child.growNetWeights.push_back(childLayer);
    }

    // Crossover flowNetWeights by calculating the average of parents' weights for each weight
    child.flowNetWeights.clear();
    for (size_t i = 0; i < parent1.flowNetWeights.size(); ++i) {
        const auto& layer1 = parent1.flowNetWeights[i];
        const auto& layer2 = parent2.flowNetWeights[i];
        vector<double> childLayer;
        for (size_t j = 0; j < layer1.size(); ++j) {
            childLayer.push_back((crossoverPointFlow * layer1[j] + (1 - crossoverPointFlow) * layer2[j]));
        }
        child.flowNetWeights.push_back(childLayer);
    }
}

vector<unique_ptr<World>> createNextGeneration(vector<unique_ptr<World>>& currentPopulation) {
    
    vector<unique_ptr<World>> nextGeneration;

    // Select elite individuals

    // Copy elites
    int numElite = POPULATION_SIZE * ELITE_PROPORTION;
    for (int i = 0; i < numElite; i++) {
        vector<unique_ptr<Junction>> junctions;
        junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, 100.0}));
        vector<unique_ptr<FoodSource>> foodSources;
        foodSources.push_back(make_unique<FoodSource>(FoodSource{500.0, 0.0, 100.0, 100.0}));

        nextGeneration.push_back(make_unique<World>(currentPopulation[i]->getGenome(), std::move(junctions), std::move(foodSources)));
    }

    // Generate offspring through crossover and mutation
    while (nextGeneration.size() < POPULATION_SIZE) {
        int parent1Idx = Random::randint(0, numElite - 1);
        int parent2Idx = Random::randint(0, numElite - 1);
        Genome childGenome;
        Genome test = currentPopulation[parent1Idx]->getGenome();
        crossOverGenomes(currentPopulation[parent1Idx]->getGenome(),
                         currentPopulation[parent2Idx]->getGenome(),
                         childGenome);

        // Mutate child genome
        childGenome.mutate(DEFAULT_MUTATION_RATE);

        // Create new World with child genome
        vector<unique_ptr<Junction>> junctions;
        junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, 100.0}));
        vector<unique_ptr<FoodSource>> foodSources;
        foodSources.push_back(make_unique<FoodSource>(FoodSource{500.0, 0.0, 100.0, 100.0}));

        auto childWorld = make_unique<World>(childGenome, std::move(junctions), std::move(foodSources));
        nextGeneration.push_back(std::move(childWorld));
    }

    return nextGeneration;
}

void runGeneticAlgorithm() {
    vector<std::unique_ptr<World>> population = generateInitialPopulation();

    // For timing
    vector<chrono::duration<double>> gen_durations;

    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {

        double averageFitness = 0.0f;

        cout << "-----------------------------------\n";
        cout << "Generation " << gen+1 << "/" << NUM_GENERATIONS << "\n";
        cout << "-----------------------------------\n";
        
        for (const auto& ind : population) {

            vector<double> fitnesses;
            
            for (int t = 0; t < NUM_TRIES; t++) {

                ind->run(NUM_STEPS, false);
                double fitness = ind->calculateFitness();
                ind->fitness = fitness;
                fitnesses.push_back(fitness);

                // Reset world for next try
                ind->junctions.clear();
                ind->tubes.clear();
                ind->foodSources.clear();

                vector<unique_ptr<Junction>> junctions;
                junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, 50.0}));
                vector<unique_ptr<FoodSource>> foodSources;
                foodSources.push_back(make_unique<FoodSource>(FoodSource{500.0, 0.0, 100.0, 100.0}));
                ind->junctions = std::move(junctions);
                ind->foodSources = std::move(foodSources);
            }

            double min_fitness = *min_element(fitnesses.begin(), fitnesses.end());
        }

        averageFitness = accumulate(population.begin(), population.end(), 0.0, [](double sum, const unique_ptr<World>& w) { return sum + w->fitness; }) / population.size();

        sortByFitness(population);
        saveGenomeAndFitness(population.front()->growthDecisionNet.genome, population.front()->fitness, averageFitness, gen);

        cout << "Population sorted by fitness.\n";
        for (size_t i = 0; i < population.size(); i++) {
            cout << " " << population[i]->fitness;
            if (i < population.size() - 1) cout << ", ";
        }
        cout << "\nBest fitness: " << population[0]->fitness << endl;
        cout << "Average fitness: " << averageFitness << endl;

        population = createNextGeneration(population);
    }
}

int main() {


    runGeneticAlgorithm();
}