#include <random>
#include <vector>
#include <memory>

#include "physarum.hpp"

using namespace std;

const int NUM_GENERATIONS = 10000;
const int POPULATION_SIZE = 40;
const int NUM_TRIES = 5;

const int NUM_STEPS = 300;

const float ELITE_PROPORTION = 0.15f;

const double DEFAULT_MUTATION_RATE = 0.05;
const double MUTATION_STRENGTH = 0.2;

const double INITIAL_ENERGY = 2 * MAX_JUNCTION_ENERGY;

const int NUM_FOOD_SOURCES = 30;


vector<unique_ptr<FoodSource>> createRandomizedFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    for (int i = 0; i < NUM_FOOD_SOURCES-1; ++i) {
        double x = Random::uniform(-300.0, 300.0);
        double y = Random::uniform(-300.0, 300.0);
        double energy = Random::uniform(500.0, 1000.0);
        double radius = sqrt(energy/3.14); // area proportional to energy
        foodSources.push_back(make_unique<FoodSource>(FoodSource{x, y, radius, energy}));
    }
    return foodSources;
}

