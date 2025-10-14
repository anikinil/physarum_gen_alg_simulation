#include "physarum.h"
#include <iostream>
#include <random>
#include <cstdint>
using namespace std;


void printWorld(const World& world) {
    cout << "World state:\n";
    for (const Cell& cell : world.cells) {
        cout << "Cell at (" << cell.x << ", " << cell.y << ") with energy " << cell.energy << " and direction " << cell.energyDir << "\n";
    }
    for (const Food& food : world.foods) {
        cout << "Food at (" << food.x << ", " << food.y << ") with energy " << food.energy << "\n";
    }
    cout << "-----\n";
}

#include <fstream>

void saveWorldFrame(std::ofstream& file, const World& world, int individual, int frame) {
    // Save cells
    for (const auto& cell : world.cells) {
        file << individual << ","
             << frame << ",cell,"
             << cell.x << ","
             << cell.y << ","
             << cell.energy << ","
             << cell.energyDir << "\n";
    }

    // Save foods
    for (const auto& food : world.foods) {
        file << individual << ","
             << frame << ",food,"
             << food.x << ","
             << food.y << ","
             << food.energy << ","
             << "" << "\n";
    }
}

int main(int argc, char* argv[]) {
    int numGenerations = stoi(argv[1]);
    int numIndividuals = stoi(argv[2]);
    int numSteps = stoi(argv[3]);

    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(0, 24);

    
    
    for (int i = 0; i < numIndividuals; i++) {
        vector<uint8_t> rules;
        
        for (int i = 0; i < 256; i++) {
            rules.push_back(dist(gen));
            // rules.push_back(11);
        }
        
        World world = {
            .cells = { {0, 0, 10.0f, 'n'} },           // and here
            .foods = { {2, 2, 5.0f}, {-2, -2, 5.0f} }, // vary here
            .rules = rules
        };
        
        std::ofstream file("world_all_frames.csv");
        file << "individual,frame,type,x,y,energy,energy_dir\n";
        
        for (int individual = 0; individual < 3; individual++) {
            for (int i = 0; i <= 5; i++) {

                cout << "Step " << i << ":\n";
                printWorld(world);
                world = world.step();
                saveWorldFrame(file, world, individual, i);
            }
        }
        file.close();
    }

    return 0;
}
