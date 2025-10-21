#include "physarum.hpp"
#include <iostream>
#include <random>
#include <cstdint>
#include <numeric>
using namespace std;

#include <fstream>

#include <filesystem>
#include <regex>

#include <chrono>


const int NUM_GENERATIONS = 300;
const int POPULATION_SIZE  = 200;
const int NUM_STEPS = 300;
const int NUM_TRIES = 10;

const float INITIAL_ENERGY = 10.0f;

const float ELITE_PROPORTION = 0.1f;
const float MUTATION_PROPORTION = 0.5f;

const float SIM_PUNISH_FACTOR = 0.f;

const int NUM_FOODS = 30;
const float FOOD_ENERGY = 4;

const int MIN_FOOD_DIST = 3;
const int MAX_FOOD_DIST = 10;

const int ACTION_SPACE = 25;


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
    vector<Food> foods;
    for (int i = 0; i < NUM_FOODS; i++) {
        int dx = dist(globalRng());
        int dy = dist(globalRng());
        foods.push_back(Food{
            getRandomCoinFlip() ? dx : -dx,
            getRandomCoinFlip() ? dy : -dy,
            FOOD_ENERGY
        });
    }
    return foods;
}

// ---------------------------- FITNESS ----------------------------

float totalCellEnergy(World& world) {
    float energy = 0;
    for (const Cell& cell: world.cells) {
        energy += cell.energy;
    }
    return energy;
}

float totalCells(World& world) {
    return static_cast<float>(world.cells.size());
}

float calculateFitness(World& world) {
    return totalCellEnergy(world) - 0.1 * totalCells(world);
}

void sortByFitness(vector<World>& population) {
    std::sort(population.begin(), population.end(),
        [](const World& a, const World& b) {
            return a.fitness > b.fitness;
        });
}

// ------------------- GENETIC ALGORITHM LOGGING -------------------


void saveBestRules(const vector<World>& population, int generation) {
    const World& best = population.front();

    std::ofstream file("best_individual.csv", std::ios::app);
    if (file.tellp() == 0) file << "generation;rules;foods\n";

    file << generation << ";";
    for (size_t i = 0; i < best.rules.size(); i++) {
        file << static_cast<int>(best.rules[i]);
        if (i < best.rules.size() - 1) file << " ";
    }
    file << ";";
    for (const auto& food : best.foods) {
        file << food.x << " " << food.y << " " << food.energy;
        if (&food != &best.foods.back()) file << ",";
    }
    file << "\n";   
}

void saveFitnessHistory(const vector<World>& population, int generation) {
    // Open file in append mode
    std::ofstream file("fitness_history.csv", std::ios::app);
    if (file.tellp() == 0) file << "generation;best;average\n";

    // Compute stats
    float best = population.front().fitness;  // assuming sorted
    float sum = 0.0f;
    for (const auto& w : population) sum += w.fitness;
    float average = sum / population.size();

    // Write generation, best, average
    file << generation << ";" << best << ";" << average << "\n";
}

// ------------------- GENETIC ALGORITHM OPERATIONS -------------------

void generateInitialPopulation(vector<World>& population) {
    for (int i = 0; i < POPULATION_SIZE; i++) {
        World world;
        world.cells.push_back(Cell{0, 0, INITIAL_ENERGY, 'n'});
        vector<Food> newFoods = getRandomizedFood();
        world.foods.insert(world.foods.end(), newFoods.begin(), newFoods.end());
        for (int s = 0; s < STATE_SPACE; s++) {
            world.rules.push_back(randAction());
        }
        population.push_back(world);
    }
}

vector<World> selectElite(vector<World> population) { 
    int numElite = POPULATION_SIZE * ELITE_PROPORTION;
    return vector<World>(population.begin(), population.begin() + numElite);
}

vector<World> crossover(const vector<World>& parents) {
    vector<World> children;

    int numParents = parents.size();
    for (int i = 0; i < numParents; i++) {
        const World& parent1 = parents[i];
        const World& parent2 = parents[(i + 1) % numParents]; // wrap-around pairing

        World child;
        child.rules.resize(STATE_SPACE);

        for (int j = 0; j < STATE_SPACE; j++) {
            // choose gene from parent1 or parent2
            child.rules[j] = getRandomCoinFlip() ? parent1.rules[j] : parent2.rules[j];
        }

        children.push_back(child);
    }

    return children;
}

void mutate(vector<World>& inds, float& similarityPunishment) {
    std::random_device rd;
    std::mt19937 generate(rd());
    std::uniform_int_distribution<uint8_t> actionDist(0, 24);


    int numMutations = STATE_SPACE * MUTATION_PROPORTION * similarityPunishment;
    std::vector<int> v(STATE_SPACE);
    iota(v.begin(), v.end(), 0); // Fill with [0, 1, ..., size-1]
    std::mt19937 rng(std::random_device{}());
    for (World & ind : inds) {
        std::shuffle(v.begin(), v.end(), rng); // Shuffle the range
        vector<int> indexes(v.begin(), v.begin() + numMutations);
        for (int idx : indexes) { 
            ind.rules[idx] = actionDist(generate);
        }
    }
}

vector<World> generateNextPopulation(const vector<World>& population) {
    vector<World> offsprings;
    vector<World> elite = selectElite(population);
    offsprings.insert(offsprings.end(), elite.begin(), elite.end());
    vector<World> eliteChildren = crossover(elite);

    float similarity = population[0].fitness / population[POPULATION_SIZE / 2].fitness;
    float similarityPunishment = std::max(1.0f, similarity * SIM_PUNISH_FACTOR);

    cout << "Similarity punishment: " << similarityPunishment << "\n";

    mutate(eliteChildren, similarityPunishment);
    offsprings.insert(offsprings.end(), eliteChildren.begin(), eliteChildren.end());
    int leftoverSize = POPULATION_SIZE * (1 - ELITE_PROPORTION*2);

    vector<World> leftover(population.end() - leftoverSize, population.end());

    mutate(leftover, similarityPunishment);
    offsprings.insert(offsprings.end(), leftover.begin(), leftover.end());
    for (World & o : offsprings) { 
        o.cells = { {0, 0, INITIAL_ENERGY, 'n'} };
        o.foods = getRandomizedFood();
    }
    return offsprings;
}

// ------------------- GENETIC ALGORITHM -------------------

void runGeneticAlgorithm() {
    vector<World> population;
    generateInitialPopulation(population);

    // For timing
    vector<chrono::duration<double>> gen_durations;

    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {
        
        auto gen_start = std::chrono::high_resolution_clock::now();

        cout << "Generation " << gen+1 << "/" << NUM_GENERATIONS << "\n";
        cout << "-----------------------------------\n";
        
        // ==== 1. Evaluate population (simulate + fitness) ====
        for (int ind = 0; ind < POPULATION_SIZE; ind++) {
            float totalFitness = 0.0f;
            for (int t = 0; t < NUM_TRIES; t++) {
                World base = population[ind];   // keep rules/genome
                base.cells.clear();
                base.cells.push_back(Cell{0, 0, INITIAL_ENERGY, 'n'});
                base.foods.clear();
                vector<Food> newFoods = getRandomizedFood();
                base.foods.insert(base.foods.end(), newFoods.begin(), newFoods.end());
                World worldCopy = base;

                // Run full simulation
                for (int step = 0; step < NUM_STEPS; step++) {
                    worldCopy = worldCopy.step();
                }

                // accumulate fitness over tries
                totalFitness += calculateFitness(worldCopy);
            }
            // assign summed fitness
            population[ind].fitness = totalFitness / static_cast<float>(NUM_TRIES);
        }

        // ==== 2. Sort and log ====
        sortByFitness(population);

        cout << "Population sorted by fitness.\n";
        for (size_t i = 0; i < population.size(); i++) {
            cout << " " << population[i].fitness;
            if (i < population.size() - 1) cout << ", ";
        }
        cout << "\nBest fitness: " << population[0].fitness << endl;
        cout << "Average fitness: "
            << accumulate(population.begin(), population.end(), 0.0f,
                        [](float sum, const World& w) { return sum + w.fitness; })
                    / population.size()
            << endl;

        saveBestRules(population, gen);
        saveFitnessHistory(population, gen);
        
        // ==== 4. Next generation ====
        population = generateNextPopulation(population);

        // ==== 5. Timing and ETA ====
        // Measure generation time
        auto gen_end = chrono::high_resolution_clock::now();
        gen_durations.push_back(gen_end - gen_start);
        // cout << "Generation time: " << gen_durations.back().count() << " seconds.\n";

        // Estimate remaining time
        if (gen > 0) {
            double avg_gen_time = std::accumulate(gen_durations.begin(), gen_durations.end(), 0.0, [](double sum, const auto& d) { return sum + d.count(); }) / gen_durations.size();
            cout << "Estimated time remaining: "
                 << (NUM_GENERATIONS - gen - 1) * avg_gen_time / 60.0
                 << " minutes\n";
        }

        cout << "------------------------\n";
    }
}

// ------------------- MAIN -------------------

int main() {

    cout << "Number of generations: " << NUM_GENERATIONS << endl;
    cout << "Population size: " << POPULATION_SIZE << endl;
    cout << "Number of steps: " << NUM_STEPS << endl;
    cout << "=======================================" << endl;

    runGeneticAlgorithm();
}