#include "physarum.h"
#include <iostream>
#include <random>
#include <cstdint>
#include <numeric>
using namespace std;

#include <fstream>


#include <filesystem>
#include <regex>

int NUM_GENERATIONS = 800;
int POPULATION_SIZE  = 100;
int NUM_STEPS = 400;

float INITIAL_ENERGY = 9.0f;

float ELITE_PROPORTION = 0.2f;
float MUTATION_PROPORTION = 0.1f;

int MIN_FOOD_DIST = 1;
int MAX_FOOD_DIST = 3;


Food getRandomizedFood() {
    std::random_device rd;
    std::mt19937 generate(rd());
    std::uniform_int_distribution<int8_t> dist(MIN_FOOD_DIST, MAX_FOOD_DIST);
    std::uniform_int_distribution<int> dir(0, 1);
    auto signedOffset = [&](int8_t v) -> int8_t {
        return dir(generate) ? -v : v;
    };
    return Food{ signedOffset(dist(generate)), signedOffset(dist(generate)), 5.0f };
}

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


void saveBestIndividual(vector<World>& population) {
    if (population.empty()) return;

    // find best individual index from population
    int bestIdx = 0;
    float bestFit = population[0].fitness;
    for (size_t i = 1; i < population.size(); ++i) {
        if (population[i].fitness > bestFit) {
            bestFit = population[i].fitness;
            bestIdx = static_cast<int>(i);
        }
    }

    // find the highest generation number present in data/
    namespace fs = std::filesystem;
    std::regex fileRe(R"(^gen_(\d+)(?:_meta)?\.csv$)");
    int maxGen = -1;
    fs::path dataDir("data");
    if (fs::exists(dataDir) && fs::is_directory(dataDir)) {
        for (auto const& entry : fs::directory_iterator(dataDir)) {
            if (!entry.is_regular_file()) continue;
            std::smatch m;
            std::string name = entry.path().filename().string();
            if (std::regex_match(name, m, fileRe)) {
                int g = std::stoi(m[1].str());
                if (g > maxGen) maxGen = g;
            }
        }
    }

    if (maxGen < 0) {
        std::cerr << "saveBestIndividual: no generation files found in data/\n";
        return;
    }

    // open the generation frames file and filter frames belonging to bestIdx
    std::string genFile = "data/gen_" + std::to_string(maxGen) + ".csv";
    std::ifstream in(genFile);
    if (!in) {
        std::cerr << "saveBestIndividual: could not open " << genFile << "\n";
        return;
    }

    std::ofstream out("data/best_ind.csv");
    if (!out) {
        std::cerr << "saveBestIndividual: could not create data/best_ind.csv\n";
        return;
    }

    std::string line;
    bool headerWritten = false;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (!headerWritten) {
            // copy header as-is
            out << line << "\n";
            headerWritten = true;
            continue;
        }
        // parse individual id (first CSV field)
        auto pos = line.find(',');
        if (pos == std::string::npos) continue;
        std::string idstr = line.substr(0, pos);
        try {
            int id = std::stoi(idstr);
            if (id == bestIdx) out << line << "\n";
        } catch (...) {
            // skip malformed line
            continue;
        }
    }

    std::cout << "Saved best individual " << bestIdx << " frames from generation " << maxGen << " to data/best_ind.csv\n";

    // Save rules of the best individual to a separate file
    fs::path rulesPath = fs::path("data") / ("best_ind_rules_gen_" + std::to_string(maxGen) + "_ind_" + std::to_string(bestIdx) + ".csv");
    std::ofstream rulesOut(rulesPath);
    if (!rulesOut) {
        std::cerr << "saveBestIndividual: could not create " << rulesPath << "\n";
    } else {
        rulesOut << "index,action\n";
        const auto &rules = population[bestIdx].rules;
        for (size_t i = 0; i < rules.size(); ++i) {
            rulesOut << i << "," << static_cast<int>(rules[i]) << "\n";
        }
        std::cout << "Saved best individual rules to " << rulesPath << "\n";
    }
}

void sort(vector<World>& worlds) {
    std::sort(worlds.begin(), worlds.end(), [](const World& a, const World& b) {
        return a.fitness > b.fitness;
    });
}

float totalCellsFitness(World world) {
    return world.cells.size();
}

float areaFitness(World world) {

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

    int area = (maxX - minX) * (maxY - minY);
    return static_cast<float>(area);
}

float calculateFitness(World world) {
    // return totalCellsFitness(world);
    return areaFitness(world);
}

vector<World> selectElite(vector<World> population) {
    int numElite = POPULATION_SIZE * ELITE_PROPORTION;
    return vector<World>(population.begin(), population.begin() + numElite);
}

vector<World> crossover(vector<World> parents) {
    
    vector<World> children;

    std::uniform_int_distribution<uint8_t> actionDist(0, 24);

    int numMutations = NUM_STATES * MUTATION_PROPORTION;

    std::vector<int> v(NUM_STATES);
    iota(v.begin(), v.end(), 0);
    std::mt19937 rng(std::random_device{}());
    
    for (int i = 0; i < parents.size(); i++) {

        World parent1 = parents[i];
        World parent2;
        if (i == parents.size() - 1) parent2 = parents[0];
        else parent2 = parents[i+1];

        std::shuffle(v.begin(), v.end(), rng);
        vector<int> indexes(v.begin(), v.begin() + NUM_STATES/2);

        vector<uint8_t> childRules;

        for (int i = 0; i < NUM_STATES/2; i++) {
            childRules.push_back(parent1.rules[i]);
        }

        for (int i = NUM_STATES/2; i < NUM_STATES; i++) {
            childRules.push_back(parent2.rules[i]);
        }

        World child;
        child.rules = childRules;
        children.push_back(child);
    }

    return children;
}

void mutate(vector<World> inds) {
    std::random_device rd;
    std::mt19937 generate(rd());
    std::uniform_int_distribution<uint8_t> actionDist(0, 24);

    int numMutations = NUM_STATES * MUTATION_PROPORTION;

    std::vector<int> v(NUM_STATES);
    iota(v.begin(), v.end(), 0);   // Fill with [0, 1, ..., size-1]

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
    mutate(eliteChildren);
    
    offsprings.insert(offsprings.end(), eliteChildren.begin(), eliteChildren.end());

    int leftoverSize = POPULATION_SIZE * (1 - ELITE_PROPORTION*2);
    vector<World> leftover(population.end()-leftoverSize-1, population.end());

    mutate(leftover);

    offsprings.insert(offsprings.end(), leftover.begin(), leftover.end());

    for (World & o : offsprings) {
        o.cells = { {0, 0, INITIAL_ENERGY, 'n'} };
        o.foods = { getRandomizedFood(), getRandomizedFood() };
    }

    return offsprings;
}

void saveFitnessHistory(vector<World> population, int gen) {
    namespace fs = std::filesystem;
    fs::create_directories("data");
    fs::path fp("data/generation_fitness.csv");

    float avg = 0.0f;
    float mx = 0.0f;
    if (!population.empty()) {
        avg = std::accumulate(population.begin(), population.end(), 0.0f, [](float sum, const World& w){
            return sum + w.fitness;
        }) / static_cast<float>(population.size());

        auto it = std::max_element(population.begin(), population.end(), [](const World& a, const World& b){
            return a.fitness < b.fitness;
        });
        if (it != population.end()) mx = it->fitness;
    }

    std::ofstream fitFile;
    if (gen == 0) {
        // start a new file for generation 0
        fitFile.open(fp, std::ios::out | std::ios::trunc);
        if (fitFile) {
            fitFile << "generation,average,max\n";
        }
    } else {
        fitFile.open(fp, std::ios::app);
    }

    if (!fitFile) {
        std::cerr << "Could not open " << fp << " for writing/appending fitness data\n";
    } else {
        fitFile << gen << "," << avg << "," << mx << "\n";
    }
}

int main(int argc, char* argv[]) {
    NUM_GENERATIONS = stoi(argv[1]);
    POPULATION_SIZE = stoi(argv[2]);
    NUM_STEPS = stoi(argv[3]);
    
    cout << "Number of generations: " << NUM_GENERATIONS << endl;
    cout << "Populations size: " << POPULATION_SIZE << endl;
    cout << "Number of steps: " << NUM_STEPS << endl;
    cout << "=======================================" << endl;

    // generate initial population
    
    std::random_device rd;
    std::mt19937 generate(rd());
    std::uniform_int_distribution<uint8_t> actions(0, 24);
    vector<World> population;
    
    for (int i = 0; i < POPULATION_SIZE; i++) {
        vector<uint8_t> rules;
        
        for (int i = 0; i < 256; i++) {
            rules.push_back(actions(generate));
            // rules.push_back(11);
        }
        
        World world = {
            .cells = { {0, 0, INITIAL_ENERGY, 'n'} },
            .foods = { getRandomizedFood(), getRandomizedFood() },
            .rules = rules
        };

        population.push_back(world);
    }

    // evolve

    for (int gen = 0; gen < NUM_GENERATIONS; gen++) {
        cout << "Generation " << gen << ":" << endl;

        saveFitnessHistory(population, gen);

        sort(population);
        cout << "Population sorted by fitness.\n";
        for (size_t i = 0; i < population.size(); i++) {
            cout << " " << population[i].fitness;
            if (i < population.size() - 1) cout << ", ";
            else cout << "\n";
        }
        cout << "Best fitness: " << population[0].fitness << endl;
        cout << "Average fitness: " << accumulate(population.begin(), population.end(), 0.0, [](float sum, const World& w) {
            return sum + w.fitness;
        }) / population.size() << endl;
            
        if (gen % 50 == 0) {
            std::ofstream file("data/gen_" + std::to_string(gen) + ".csv");
            file << "individual,step,type,x,y,energy,energy_dir\n";
            
            for (int ind = 0; ind < POPULATION_SIZE; ind++) {
                    World worldCopy = population[ind];
                // saveWorldFrame(file, worldCopy, ind, 0);

                for (int step = 1; step <= NUM_STEPS; step++) {
                    // cout << "Step " << step << ":\n";
                    worldCopy = worldCopy.step();
                    saveWorldFrame(file, worldCopy, ind, step);
                }

                float fitness = calculateFitness(worldCopy);
                population[ind].fitness = fitness;
            }
        
            // close the generation file
            file.close();
        } else {
            // just calculate fitness without saving frames
            for (int ind = 0; ind < POPULATION_SIZE; ind++) {
                World worldCopy = population[ind];
                for (int step = 1; step <= NUM_STEPS; step++) {
                    worldCopy = worldCopy.step();
                }
                float fitness = calculateFitness(worldCopy);
                population[ind].fitness = fitness;
            }
        }
        
        cout << "-----------------------------------" << endl;
        // savePopulationMeta(population, gen);

        if (gen < NUM_GENERATIONS -1) {
            population = generateNextPopulation(population);
        } else {
            saveBestIndividual(population);
        }
    }

    return 0;
}


