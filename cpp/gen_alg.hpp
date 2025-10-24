#include <random>
#include <vector>
#include "physarum.hpp"

const int NUM_GENERATIONS = 1000;
const int POPULATION_SIZE  = 200;
const int NUM_TRIES = 30;

const int NUM_STEPS = 100; 

const float ELITE_PROPORTION = 0.15f;
const float MUTATION_PROB = 0.005f;

const float INITIAL_ENERGY = 100.0f;

const int NUM_FOODS = 250;
const float FOOD_ENERGY = 10.0f;
const int MIN_FOOD_DIST = 1;
const int MAX_FOOD_DIST = 20;

// ------------------- RANDOM UTILITIES -------------------

static std::mt19937& globalRng() {
    static std::random_device rd;
    static std::mt19937 g(rd());
    return g;
}

int randInt(int a, int b) {
    std::uniform_int_distribution<int> dist(a, b);
    return dist(globalRng());
}

uint8_t randAction() {
    std::uniform_int_distribution<int> dist(0, ACTION_SPACE - 1);
    return static_cast<uint8_t>(dist(globalRng()));
}

int getRandomCoinFlip() {
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(globalRng());
}

vector<Food> getRandomizedFood() {
    std::uniform_int_distribution<int> dist(MIN_FOOD_DIST, MAX_FOOD_DIST);
    std::bernoulli_distribution signDist(0.5);
    vector<Food> foods;
    foods.reserve(NUM_FOODS);
    for (int i = 0; i < NUM_FOODS; ++i) {
        int dx = dist(globalRng());
        int dy = dist(globalRng());
        int sx = signDist(globalRng()) ? 1 : -1;
        int sy = signDist(globalRng()) ? 1 : -1;
        foods.push_back(Food{ dx * sx, dy * sy, FOOD_ENERGY });
    }
    return foods;
}