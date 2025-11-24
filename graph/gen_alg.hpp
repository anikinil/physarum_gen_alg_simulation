#include <random>
#include <vector>
#include <memory>

#include "physarum.hpp"

using namespace std;

const int NUM_GENERATIONS = 10000;
const int POPULATION_SIZE = 50;
const int NUM_TRIES = 5;

const int NUM_STEPS = 300;

const float ELITE_PROPORTION = 0.3f;
const float CROSSED_PROPORTION = 0.2f;

const double DEFAULT_MUTATION_RATE = 0.05;
const double MUTATION_STRENGTH = 0.5;

const double INITIAL_ENERGY = MAX_JUNCTION_ENERGY;

const int TOTAL_FOOD_ENERGY = 2000;
const int NUM_FOOD_SOURCES = 600;


vector<unique_ptr<FoodSource>> createRandomizedFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    double min_energy = TOTAL_FOOD_ENERGY / NUM_FOOD_SOURCES * 0.5;
    double max_energy = TOTAL_FOOD_ENERGY / NUM_FOOD_SOURCES * 1.5;
    for (int i = 0; i < NUM_FOOD_SOURCES-1; ++i) {
        double x = Random::uniform(-200.0, 200.0);
        double y = Random::uniform(-200.0, 200.0);
        double energy = Random::uniform(min_energy, max_energy);
        double radius = 3.0 * sqrt(energy/3.14);
        foodSources.push_back(make_unique<FoodSource>(FoodSource{x, y, radius, energy}));
    }
    double energy = TOTAL_FOOD_ENERGY/NUM_FOOD_SOURCES;
    double radius = 3.0 * sqrt(energy/3.14);
    foodSources.push_back(make_unique<FoodSource>(FoodSource{0.0, 0.0, radius, energy}));
    return foodSources;
}

