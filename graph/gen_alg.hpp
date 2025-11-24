#include <random>
#include <vector>
#include <memory>

#include "physarum.hpp"

using namespace std;

const int NUM_GENERATIONS = 2;
const int POPULATION_SIZE = 5;
const int NUM_TRIES = 3;

const int NUM_STEPS = 10;

const float ELITE_PROPORTION = 0.3f;
const float CROSSED_PROPORTION = 0.2f;

const double DEFAULT_MUTATION_RATE = 0.05;
const double MUTATION_STRENGTH = 0.5;

const double INITIAL_ENERGY = MAX_JUNCTION_ENERGY;

const int NUM_FOOD_SOURCES = 20;


vector<unique_ptr<FoodSource>> createRandomizedFoodSources() {
    vector<unique_ptr<FoodSource>> foodSources;
    for (int i = 0; i < NUM_FOOD_SOURCES-1; ++i) {
        double x = Random::uniform(-200.0, 200.0);
        double y = Random::uniform(-200.0, 200.0);
        double energy = Random::uniform(10.0, 20.0);
        double radius = 5.0 * sqrt(energy/3.14);
        foodSources.push_back(make_unique<FoodSource>(FoodSource{x, y, radius, energy}));
    }
    foodSources.push_back(make_unique<FoodSource>(FoodSource{0.0, 0.0, 5.0, 20.0}));
    return foodSources;
}

