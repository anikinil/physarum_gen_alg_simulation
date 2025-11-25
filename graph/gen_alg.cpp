#include "gen_alg.hpp"

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
        vector<unique_ptr<FoodSource>> foodSources = createRandomizedFoodSources();
        junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, INITIAL_ENERGY}));
        auto world = make_unique<World>(Genome());
        world->placeNewFoodSources(std::move(foodSources));
        world->placeNewJunctions(std::move(junctions));
        population.push_back(std::move(world));
    }

    return population;
}

void saveGenomeAndFitness(const Genome& genome, double fitness, double averageFitness, int generation) {
    std::ofstream file("data/genome_fitness.csv", std::ios::app);
    if (file.tellp() == 0) file << "generation;best_fitness;average_fitness;genome\n";

    file << generation << ";" << fitness << ";" << averageFitness << ";";

    // Serialize genome
    for (const auto& weight : genome.serialize()) {
        file << weight << " ";
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
    vector<double> p1_weights = parent1.serialize();
    vector<double> p2_weights = parent2.serialize();

    vector<double> child_weights;
    child_weights.reserve(p1_weights.size());

    // Single point crossover
    int crossover_point = Random::randint(0, p1_weights.size() - 1);
    for (int i = 0; i < p1_weights.size(); i++) {
        if (i < crossover_point) {
            child_weights.push_back(p1_weights[i]);
        } else {
            child_weights.push_back(p2_weights[i]);
        }
    }

    child.setGrowNetWights(child_weights);
}

vector<unique_ptr<World>> createNextGeneration(vector<unique_ptr<World>>& currentPopulation) {
    
    vector<unique_ptr<World>> nextGeneration;

    // Select elite individuals

    // Copy elites
    int numElite = POPULATION_SIZE * ELITE_PROPORTION;
    for (int i = 0; i < numElite; i++) {
        vector<unique_ptr<Junction>> junctions;
        junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, INITIAL_ENERGY}));
        vector<unique_ptr<FoodSource>> foodSources = createRandomizedFoodSources();
        nextGeneration.push_back(make_unique<World>(currentPopulation[i]->getGenome()));
        nextGeneration.back()->placeNewFoodSources(std::move(foodSources));
        nextGeneration.back()->placeNewJunctions(std::move(junctions));
    }

    int numCrossed = POPULATION_SIZE * CROSSED_PROPORTION;
    // Generate offspring through crossover and mutation
    while (nextGeneration.size() < numElite + numCrossed) {
        int parent1Idx = Random::randint(0, numElite - 1);
        int parent2Idx = Random::randint(0, numElite - 1);
        Genome childGenome;
        Genome test = currentPopulation[parent1Idx]->getGenome();
        // crossOverGenomes(currentPopulation[parent1Idx]->getGenome(),
                        //  currentPopulation[parent2Idx]->getGenome(),
                        //  childGenome);

        // Mutate child genome
        childGenome.mutate(DEFAULT_MUTATION_RATE, MUTATION_STRENGTH);

        // Create new World with child genome
        vector<unique_ptr<Junction>> junctions;
        junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, INITIAL_ENERGY}));
        vector<unique_ptr<FoodSource>> foodSources = createRandomizedFoodSources();

        auto childWorld = make_unique<World>(childGenome);
        childWorld->placeNewFoodSources(std::move(foodSources));
        childWorld->placeNewJunctions(std::move(junctions));
        nextGeneration.push_back(std::move(childWorld));
    }

    while (nextGeneration.size() < POPULATION_SIZE) {
        // Fill the rest of the population with mutated copies of elites
        int eliteIdx = Random::randint(0, numElite - 1);
        Genome mutatedGenome = currentPopulation[eliteIdx]->getGenome();
        mutatedGenome.mutate(DEFAULT_MUTATION_RATE, MUTATION_STRENGTH);

        vector<unique_ptr<Junction>> junctions;
        junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, INITIAL_ENERGY}));
        vector<unique_ptr<FoodSource>> foodSources = createRandomizedFoodSources();

        nextGeneration.push_back(make_unique<World>(mutatedGenome));
        nextGeneration.back()->placeNewFoodSources(std::move(foodSources));
        nextGeneration.back()->placeNewJunctions(std::move(junctions));
    }

    return nextGeneration;
}

void printETA(int currentGeneration, const vector<chrono::duration<double>>& gen_durations) {
    if (gen_durations.empty()) return;
    double avg_gen_time = std::accumulate(gen_durations.begin(), gen_durations.end(), 0.0, [](double sum, const auto& d) { return sum + d.count(); }) / gen_durations.size();
    double remaining_seconds = (NUM_GENERATIONS - currentGeneration - 1) * avg_gen_time;
    long long remaining = static_cast<long long>(remaining_seconds + 0.5); // round to nearest second
    long long hours = remaining / 3600;
    long long minutes = (remaining % 3600) / 60;
    cout << "Estimated time remaining: " << hours << " hours " << minutes << " minutes" << endl;
}

void runGeneticAlgorithm() {
    vector<std::unique_ptr<World>> population = generateInitialPopulation();

    // For timing
    vector<chrono::duration<double>> gen_durations;

    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {

        auto gen_start = std::chrono::high_resolution_clock::now();

        cout << "-----------------------------------" << endl;
        cout << "Generation " << gen+1 << "/" << NUM_GENERATIONS << endl;
        cout << "-----------------------------------" << endl;
        
        int count = 0;
        for (const auto& ind : population) {

            cout << "Individual " << count+1 << "/" << POPULATION_SIZE << endl;
            count++;

            vector<double> ind_fitnesses;
            
            for (int t = 0; t < NUM_TRIES; t++) {

                cout << " Try " << t+1 << "/" << NUM_TRIES << endl;

                ind->run(NUM_STEPS, false);
                ind->calculateFitness();
                ind_fitnesses.push_back(ind->fitness);

                if (t < NUM_TRIES - 1) {
                    // Reset world for next try
                    ind->fitness = 0.0;
                    ind->food_consumed = 0.0;

                    ind->junctions.clear();
                    ind->tubes.clear();
                    ind->foodSources.clear();

                    vector<unique_ptr<Junction>> junctions;
                    junctions.push_back(make_unique<Junction>(Junction{0.0, 0.0, INITIAL_ENERGY}));
                    vector<unique_ptr<FoodSource>> foodSources = createRandomizedFoodSources();
                    ind->junctions = std::move(junctions);
                    ind->foodSources = std::move(foodSources);
                }

                cout << "\033[A\33[2K\r"; // erase try line
            }
            std::sort(ind_fitnesses.begin(), ind_fitnesses.end(), std::less<double>());
            // cout << " Individual fitnesses: ";
            // for (size_t i = 0; i < ind_fitnesses.size(); i++) {
            //     cout << ind_fitnesses[i];
            //     if (i < ind_fitnesses.size() - 1) cout << ", ";
            // }
            // cout << endl;
            ind->fitness = *std::next(ind_fitnesses.begin(), ind_fitnesses.size() * 0.33); // fitness is the 33rd percentile
            // ind->fitness = *min_element(ind_fitnesses.begin(), ind_fitnesses.end()); // worst fitness

            cout << "\033[A\33[2K\r"; // erase ind line
        }

        sortByFitness(population);
        
        double bestFitness = population.front()->fitness;
        double averageFitness = accumulate(population.begin(), population.end(), 0.0, [](double sum, const unique_ptr<World>& w) { return sum + w->fitness; }) / population.size();
        
        saveGenomeAndFitness(population.front()->getGenome(), bestFitness, averageFitness, gen);

        // plot
        system("python3 plot.py");

        cout << "Population sorted by fitness:" << endl;
        for (size_t i = 0; i < population.size(); i++) {
            cout << " " << population[i]->fitness;
            if (i < population.size() - 1) cout << ", ";
        }
        cout << endl;

        cout << "-----------------------------------" << endl;
        cout << "Best fitness: " << bestFitness << endl;
        cout << "Average fitness: " << averageFitness << endl;

        population = createNextGeneration(population);

        // ==== Timing and ETA ====
        auto gen_end = chrono::high_resolution_clock::now();
        gen_durations.push_back(gen_end - gen_start);
        cout << "Generation time: " << gen_durations.back().count() << " seconds.\n";

        printETA(gen, gen_durations);
    }
}

int main() {

    runGeneticAlgorithm();
}