#include <random>
#include <vector>
#include <memory>

#include "physarum.hpp"

using namespace std;

const int NUM_GENERATIONS = 10000;
const int POPULATION_SIZE = 20;
const int NUM_TRIES = 16;

const int NUM_STEPS = 1000; // -> more over time (?)

const float ELITE_PROPORTION = 0.4f;
const float CROSSED_PROPORTION = 0.1f;

const double DEFAULT_MUTATION_RATE = 0.2;
const double MUTATION_STRENGTH = 0.5;

const double INITIAL_ENERGY = MAX_JUNCTION_ENERGY;

// const int TOTAL_FOOD_ENERGY_SMALL = 5000;
// const int NUM_FOOD_SOURCES_SMALL = 1500;
// const int TOTAL_FOOD_ENERGY_LARGE = 5000;
const int NUM_FOOD_SOURCES_LARGE = 3;

// vector<unique_ptr<FoodSource>> createSmallFoodSources() {
//     vector<unique_ptr<FoodSource>> foodSources;
//     double energy_per_source = TOTAL_FOOD_ENERGY_SMALL / NUM_FOOD_SOURCES_SMALL;
//     for (int i = 0; i < NUM_FOOD_SOURCES_SMALL; ++i) {
//         double x = Random::uniform(-500.0, 500.0);
//         double y = Random::uniform(-500.0, 500.0);
//         double radius = 6.0;
//         foodSources.push_back(make_unique<FoodSource>(FoodSource{x, y, radius, energy_per_source}));
//     }
//     return foodSources;
// }

vector<unique_ptr<FoodSource>> createLargeFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    double energy_per_source = FOOD_ENERGY_ABSORB_RATE * NUM_STEPS;
    for (int i = 0; i < NUM_FOOD_SOURCES_LARGE; ++i) {
        double x = Random::uniform(-500.0, 500.0);
        double y = Random::uniform(-500.0, 500.0);
        double radius = 10.0;
        foodSources.push_back(make_unique<FoodSource>(FoodSource{x, y, radius, energy_per_source}));
    }
    return foodSources;
}

vector<unique_ptr<FoodSource>> createRandomizedFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    // vector<unique_ptr<FoodSource>> smallSources = createSmallFoodSources();
    vector<unique_ptr<FoodSource>> largeSources = createLargeFoodSources();
    // foodSources.insert(foodSources.end(), std::make_move_iterator(smallSources.begin()), std::make_move_iterator(smallSources.end()));
    foodSources.insert(foodSources.end(), std::make_move_iterator(largeSources.begin()), std::make_move_iterator(largeSources.end()));
    double energy = FOOD_ENERGY_ABSORB_RATE * NUM_STEPS; // avoids depletion during tests
    double radius = TUBE_LENGTH;
    foodSources.push_back(make_unique<FoodSource>(FoodSource{0.0, 0.0, radius, energy}));
    return foodSources;
}

