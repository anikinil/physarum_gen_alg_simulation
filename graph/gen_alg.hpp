#include <random>
#include <vector>
#include <memory>

#include "physarum.hpp"

using namespace std;

const int NUM_GENERATIONS = 10000;
const int POPULATION_SIZE = 30;
const int NUM_TRIES = 16;

const int NUM_STEPS = 200; // -> more over time (?)

const float ELITE_PROPORTION = 0.4f;
const float CROSSED_PROPORTION = 0.1f;

const double DEFAULT_MUTATION_RATE = 0.2;
const double MUTATION_STRENGTH = 0.5;

const double INITIAL_ENERGY = 1.1 * MAX_JUNCTION_ENERGY;

const int NUM_FOOD_SOURCES_LARGE = 30;

const double MAX_DIST_FROM_ORIG = 500.0;

vector<unique_ptr<FoodSource>> createLargeFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    for (int i = 0; i < NUM_FOOD_SOURCES_LARGE; ++i) {
        double x = Random::uniform(-MAX_DIST_FROM_ORIG, MAX_DIST_FROM_ORIG);
        double y = Random::uniform(-MAX_DIST_FROM_ORIG, MAX_DIST_FROM_ORIG);
        double energy = FOOD_ENERGY_ABSORB_RATE * NUM_STEPS;
        double radius = 0.5 * TUBE_LENGTH;
        foodSources.push_back(make_unique<FoodSource>(FoodSource{x, y, radius, energy}));
    }
    return foodSources;
}

vector<unique_ptr<FoodSource>> createRandomizedFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    vector<unique_ptr<FoodSource>> largeSources = createLargeFoodSources();
    foodSources.insert(foodSources.end(), std::make_move_iterator(largeSources.begin()), std::make_move_iterator(largeSources.end()));
    double energy = FOOD_ENERGY_ABSORB_RATE * NUM_STEPS; // avoids depletion during tests
    double radius = TUBE_LENGTH;
    foodSources.push_back(make_unique<FoodSource>(FoodSource{0.0, 0.0, radius, energy}));
    return foodSources;
}

