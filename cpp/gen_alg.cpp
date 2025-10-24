#include "gen_alg.hpp"
#include <iostream>
#include <random>
#include <cstdint>
#include <numeric>
using namespace std;

#include <fstream>

#include <filesystem>
#include <regex>

#include <chrono>

// ---------------------------- FITNESS ----------------------------

float energyCentrality(World& world) {
    if (world.cells.empty()) return 0.0f;

    float totalDist = 0.0f;
    for (const Cell& cell : world.cells) {
        totalDist += std::sqrt(cell.x * cell.x + cell.y * cell.y);
    }

    float avgDist = totalDist / (world.cells.size());
    return 1.0f / (avgDist + 1.0f);
}

float spread(World& world) {
    if (world.cells.empty()) return 0.0f;

    int minX = world.cells[0].x;
    int maxX = world.cells[0].x;
    int minY = world.cells[0].y;
    int maxY = world.cells[0].y;

    for (const Cell& cell : world.cells) {
        if (cell.x < minX) minX = cell.x;
        if (cell.x > maxX) maxX = cell.x;
        if (cell.y < minY) minY = cell.y;
        if (cell.y > maxY) maxY = cell.y;
    }

    float spread = static_cast<float>((maxX - minX) + (maxY - minY));
    return spread;
}

float totalCellsEnergy(World& world) {
    float energy = 0;
    for (const Cell& cell: world.cells) {
        energy += cell.energy;
    }
    return energy;
}

float totalAcquiredEnergy(World& world) {
    float totalFoodEnergy = NUM_FOODS * FOOD_ENERGY;
    float energy = 0;
    for (const Food& food: world.foods) {
        energy += food.energy;
    }
    return std::round((totalFoodEnergy - energy) / totalFoodEnergy * 100.0f) / 100.0f;
}

float totalCells(World& world) {
    return static_cast<float>(world.cells.size());
}

float droughtResistance(World& world) {
    int fullCells = 0;
    for (const Cell& cell : world.cells) {
        if (cell.energy >= MIN_GROWTH_ENERGY) fullCells++;
    }
    return static_cast<float>(fullCells) / world.cells.size();
}

void sortByFitness(vector<World>& population) {
    std::sort(population.begin(), population.end(),
    [](const World& a, const World& b) {
        return a.fitness > b.fitness;
    });
}

float calculateFitness(World& world) {
    return 
        totalAcquiredEnergy(world)
        + 0.1f * energyCentrality(world)
        - 0.5f * droughtResistance(world);
    // return spreadFitness(world);
    // return energyCentralityFitness(world);
}

// ------------------- GENETIC ALGORITHM LOGGING -------------------


void saveBestRules(const vector<World>& population, int generation) {
    const World& best = population.front();

    std::ofstream file("best_individual.csv", std::ios::app);
    if (file.tellp() == 0) file << "generation;rules\n";

    file << generation << ";";
    for (size_t i = 0; i < best.rules.size(); i++) {
        file << static_cast<int>(best.rules[i]);
        if (i < best.rules.size() - 1) file << " ";
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
            std::bernoulli_distribution takeFromP1(0.5);
            child.rules[j] = takeFromP1(globalRng()) ? parent1.rules[j] : parent2.rules[j];
        }

        children.push_back(child);
    }

    return children;
}

void mutate(vector<World>& inds, const float& mutationProb) {
    std::random_device rd;
    std::mt19937 generate(rd());
    std::uniform_int_distribution<uint8_t> actionDist(0, 24);
    std::mt19937 rng(std::random_device{}());
    for (World & ind : inds) {
        for (uint8_t & r : ind.rules) { 
            if (getRandomCoinFlip() < mutationProb) {
                r = actionDist(rng);
            }
        }
    }
}

vector<World> generateNextPopulation(const vector<World>& population) {
    vector<World> offsprings;
    vector<World> elite = selectElite(population);
    offsprings.insert(offsprings.end(), elite.begin(), elite.end());
    vector<World> eliteChildren = crossover(elite);

    // float similarity = population[POPULATION_SIZE / 2].fitness / population[0].fitness;
    // float similarityPunishment = std::max(1.0f, similarity * SIM_PUNISH_FACTOR);
    // cout << "Similarity punishment: " << similarityPunishment << "\n";
    // float mutationProb = std::min(1.0f, (MUTATION_PROPORTION * similarityPunishment));
    cout << "Mutation probability: " << MUTATION_PROB << "\n";
    mutate(eliteChildren, MUTATION_PROB);
    offsprings.insert(offsprings.end(), eliteChildren.begin(), eliteChildren.end());
    int leftoverSize = POPULATION_SIZE * (1 - ELITE_PROPORTION*2);

    vector<World> leftover(population.end() - leftoverSize, population.end());

    mutate(leftover, MUTATION_PROB);
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

        float averageFitness = 0.0f;

        cout << "Generation " << gen+1 << "/" << NUM_GENERATIONS << "\n";
        cout << "-----------------------------------\n";
        
        // ==== 1. Evaluate population (simulate + fitness) ====
        for (int ind = 0; ind < POPULATION_SIZE; ind++) {
            vector<float> fitnesses = {};
            bool failedEarly = false;
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
                float fitness = calculateFitness(worldCopy);
                // early stopping if no positive fitness achieved
                if (t >= 5 && gen > 0 && *std::max_element(fitnesses.begin(), fitnesses.begin() + t) <= averageFitness * 0.1f) {
                    population[ind].fitness = -1;
                    failedEarly = true;
                    break;
                } else {
                    fitnesses.push_back(fitness);
                }
            }
            if (failedEarly) continue;
            // assign summed fitness
            population[ind].fitness = accumulate(fitnesses.begin(), fitnesses.end(), 0.0f) / fitnesses.size();
        }

        // ==== 2. Sort and log ====
        sortByFitness(population);

        cout << "Population sorted by fitness.\n";
        for (size_t i = 0; i < population.size(); i++) {
            cout << " " << population[i].fitness;
            if (i < population.size() - 1) cout << ", ";
        }
        cout << "\nBest fitness: " << population[0].fitness << endl;
        averageFitness = accumulate(population.begin(), population.end(), 0.0f,
                        [](float sum, const World& w) { return sum + w.fitness; })
                    / population.size();
        cout << "Average fitness: " << averageFitness << endl;

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