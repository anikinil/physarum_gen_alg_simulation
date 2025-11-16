#include <random>
#include <vector>
#include <memory>

#include "physarum.hpp"

using namespace std;

const int NUM_GENERATIONS = 10000;
const int POPULATION_SIZE = 100;
const int NUM_TRIES = 9;

const int NUM_STEPS = 200;

const float ELITE_PROPORTION = 0.15f;

const double DEFAULT_MUTATION_RATE = 0.3;
const double MUTATION_STRENGTH = 1;

const double INITIAL_ENERGY = MAX_JUNCTION_ENERGY;

const int NUM_FOOD_SOURCES = 80;


vector<unique_ptr<FoodSource>> createRandomizedFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    for (int i = 0; i < NUM_FOOD_SOURCES-1; ++i) {
        double x = Random::uniform(-200.0, 200.0);
        double y = Random::uniform(-200.0, 200.0);
        double energy = Random::uniform(90.0, 100.0);
        double radius = sqrt(energy/3.14); // area proportional to energy
        foodSources.push_back(make_unique<FoodSource>(FoodSource{x, y, radius, energy}));
    }
    foodSources.push_back(make_unique<FoodSource>(FoodSource{0.0, 0.0, 5.0, 200.0})); // Add a food source at the origin with zero energy and radius
    return foodSources;
}

