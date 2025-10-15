#include "physarum.h"
#include <iostream>
#include <random>
#include <cstdint>
using namespace std;

#include <fstream>

void saveWorldFrame(std::ofstream& file, const World& world, int individual, int step) {
    // Save cells
    for (const auto& cell : world.cells) {
        file << individual << ","
             << step << ",cell,"
             << cell.x << ","
             << cell.y << ","
             << cell.energy << ","
             << cell.energyDir << "\n";
    }

    // Save foods
    for (const auto& food : world.foods) {
        file << individual << ","
             << step << ",food,"
             << food.x << ","
             << food.y << ","
             << food.energy << ","
             << "" << "\n";
    }
}

void savePopulationMeta(const vector<World>& population, int generation) {

    std::ofstream file("data/gen_" + std::to_string(generation) + "_meta.csv");
    file << "individual,fitness,rules\n";

    for (size_t i = 0; i < population.size(); i++) {
        file << i << ","
             << population[i].fitness << ",";
        // Convert rules vector to comma-separated string
        for (size_t j = 0; j < population[i].rules.size(); ++j) {
            file << static_cast<int>(population[i].rules[j]);
            if (j != population[i].rules.size() - 1) file << " ";
        }
        file << "\n";
    }
}

float totalCellsFitness(World world) {
    return world.cells.size();
}

float calculateFitness(World world) {
    return totalCellsFitness(world);
}

void sort(vector<World>& worlds) {
    std::sort(worlds.begin(), worlds.end(), [](const World& a, const World& b) {
        return a.fitness > b.fitness;
    });
}

    vector<World> selectElite(const vector<World>& population, int eliteCount) {
        return vector<World>(population.begin(), population.begin() + eliteCount);
    }

World crossover(World p1, World p2) {
    World child;
}

void mutate(World& world) {
    // std::random_device rd;
    // std::mt19937 generate(rd());
    // std::uniform_int_distribution<size_t> indexDist(0, world.rules.size() - 1);
    // std::uniform_int_distribution<uint8_t> actionDist(0, 24);

    // int mutationIndex = indexDist(generate);
    // world.rules[mutationIndex] = actionDist(generate);
}

vector<World> generateOffsprings(const vector<World>& parents) {
    vector<World> offsprings;

    // Generate offsprings by crossover and mutation
    for (size_t i = 0; i < parents.size(); i += 2) {
        if (i + 1 < parents.size()) {
            World child = crossover(parents[i], parents[i + 1]);
            mutate(child);
            offsprings.push_back(child);
        }
    }

    return offsprings;
}

int main(int argc, char* argv[]) {
    int numGenerations = stoi(argv[1]);
    int numIndividuals = stoi(argv[2]);
    int numSteps = stoi(argv[3]);
    
    // generate initial population
    
    std::random_device rd;
    std::mt19937 generate(rd());
    std::uniform_int_distribution<uint8_t> actions(0, 24);
    std::uniform_int_distribution<uint8_t> dist(-2, 2);

    vector<World> population;
    
    for (int i = 0; i < numIndividuals; i++) {
        vector<uint8_t> rules;
        
        for (int i = 0; i < 256; i++) {
            rules.push_back(actions(generate));
            // rules.push_back(11);
        }
        
        World world = {
            .cells = { {0, 0, 10.0f, 'n'} },
            .foods = { {dist(generate), dist(generate), 5.0f}, {dist(generate), dist(generate), 5.0f} },
            .rules = rules
        };

        population.push_back(world);
    }

    // evolve

    for (int gen = 0; gen < numGenerations; gen++) {
        cout << "Generation " << gen << ":\n";
        
        std::ofstream file("data/gen_" + std::to_string(gen) + ".csv");
        file << "individual,step,type,x,y,energy,energy_dir\n";
        
        for (int ind = 0; ind < numIndividuals; ind++) {
            World worldCopy = population[ind];
            
            for (int step = 0; step <= numSteps; step++) {
                // cout << "Step " << step << ":\n";
                worldCopy = worldCopy.step();
                saveWorldFrame(file, worldCopy, ind, step);
            }

            float fitness = calculateFitness(worldCopy);
            population[ind].fitness = fitness;

        }
        file.close();

        savePopulationMeta(population, gen);
        sort(population);


    }

    return 0;
}
