#include "gen_alg.hpp"
#include "utils.hpp"
#include <iostream>
#include <random>
#include <cstdint>
#include <numeric>
using namespace std;

#include <fstream>

#include <filesystem>
#include <regex>

#include <chrono>

vector<World> generateInitialPopulation() {
    vector<World> population;
    for (int i = 0; i < POPULATION_SIZE; i++) {


        vector<Junction> junctions = {Junction{0.0f, 0.0f, 100}};
        vector<FoodSource> foodSources = {FoodSource{0.0f, 0.0f, 10.0f, 100}};
    }
}


void runGeneticAlgorithm() {
    vector<World> population = generateInitialPopulation();

    // For timing
    vector<chrono::duration<double>> gen_durations;

    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {
    }
}

int main() {

    cout << "Number of generations: " << NUM_GENERATIONS << endl;
    cout << "Population size: " << POPULATION_SIZE << endl;
    cout << "Number of steps: " << NUM_STEPS << endl;
    cout << "=======================================" << endl;

    runGeneticAlgorithm();
}