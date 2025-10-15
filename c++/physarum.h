#include <optional>
#include <cstdint>
#include <vector>
#include <cmath>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <iostream>
using namespace std;

namespace std {
    template <>
    struct hash<std::pair<int, int>> {
        std::size_t operator()(const std::pair<int, int>& p) const noexcept {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
        }
    };
}


vector<char> dirs = {'n', 'l', 'r', 'u', 'd'};

float MIN_GROWTH_ENERGY = 3;
float ENERGY_PORTION = 0.2;
float MIN_ENERGY_TO_PASS = 0.1;

struct Cell {
    int x;
    int y;
    float energy;
    char energyDir;
};

struct Food {
    int x;
    int y;
    float energy;
};


struct World {
    vector<Cell> cells;
    vector<Food> foods;

    vector<uint8_t> rules;

    float fitness = 0;

    World step() {
        
        World newWorld = updateGrowth();
        newWorld = newWorld.updateEnergy();
        newWorld = newWorld.updateFood();
        return newWorld;
    }

    World updateGrowth() {

        // cout << "UPDATE_GROWTH" << "\n";

        World newWorld;
        newWorld.rules = rules;
        newWorld.foods = foods;

        // cout << "RULES SIZE:  " << rules.size() << '\n';
        for (Cell & cell : cells) {

            // cout << "debug: cell at (" << cell.x << ", " << cell.y << ") with energy " << cell.energy << "\n";

            uint8_t state = getCellState(cell);
            uint8_t action = getNextAction(state);

            char growthDir = decodeGrowthDir(action);

            int targetX = cell.x;
            int targetY = cell.y;

            if (growthDir == 'l') targetX -= 1;
            else if (growthDir == 'r') targetX += 1;
            else if (growthDir == 'u') targetY += 1;
            else if (growthDir == 'd') targetY -= 1;

            if (growthDir == 'n' || cell.energy < MIN_GROWTH_ENERGY || anyObstaclesAt(targetX, targetY)) {
                Cell updatedOrigCell = cell;
                newWorld.cells.push_back(updatedOrigCell);
                continue;
            }
            
            
            Cell updatedOrigCell = cell;
            updatedOrigCell.energy /= 2;
            newWorld.cells.push_back(updatedOrigCell);
            newWorld.cells.push_back(Cell{targetX, targetY, cell.energy/2, 'n'});
        }

        newWorld.cells = resolveGrowthConflicts(newWorld.cells);

        return newWorld;
    }

    bool anyObstaclesAt(int x, int y) {
        for (Food & food : foods) {
            if (food.x == x && food.y == y) return true;
        }
        for (Cell & cell : cells) {
            if (cell.x == x && cell.y == y) return true;
        }
        return false;
    }

    bool anyCellsAt(int x, int y) {
        for (Cell & cell : cells) {
            if (cell.x == x && cell.y == y) return true;
        }
        return false;
    }

    vector<Cell> resolveGrowthConflicts(vector<Cell> cells) {

        unordered_map<pair<int,int>, vector<Cell>> map;

        for (Cell & cell : cells) {
            map[{cell.x, cell.y}].push_back(cell);
        }

        vector<Cell> newCells;

        for (auto el : map) {
            Cell strongest = *max_element(
                el.second.begin(),
                el.second.end(),
                [](const Cell& a, const Cell& b){
                    return a.energy < b.energy;
            });

            newCells.push_back(strongest);
        }

        return newCells;
    }

    World updateEnergy() {

        World newWorld;
        newWorld.rules = rules;
        newWorld.foods = foods;
        
        // cout << "UPDATE_ENERGY" << "\n";
        
        for (Cell & cell : cells) {

            // cout << "debug: cell at (" << cell.x << ", " << cell.y << ") with energy " << cell.energy << "\n";

            uint8_t state = getCellState(cell);
            uint8_t action = getNextAction(state);
            
            char energyDir = decodeEnergyDir(action);
            
            if (energyDir == 'n' || cell.energy * ENERGY_PORTION < MIN_ENERGY_TO_PASS) {
                Cell newCell = cell;
                newWorld.cells.push_back(newCell);
                continue;
            }   

            int targetX = cell.x;
            int targetY = cell.y;

            if (energyDir == 'l') targetX -= 1;

            else if (energyDir == 'r') targetX += 1;
            else if (energyDir == 'u') targetY += 1;
            else if (energyDir == 'd') targetY -= 1;

            if (!anyCellsAt(targetX, targetY)) {
                Cell newCell = cell;
                newWorld.cells.push_back(newCell);
                continue;
            }

            Cell updatedOrigCell = cell;
            updatedOrigCell.energy *= (1 - ENERGY_PORTION);
            updatedOrigCell.energyDir = energyDir;
            newWorld.cells.push_back(updatedOrigCell);
            newWorld.cells.push_back(Cell{targetX, targetY, cell.energy * ENERGY_PORTION, 'n'});
        }

        // cout << "Num rules: " << newWorld.rules.size() << '\n';
        newWorld.cells = resolveEnergyConflicts(newWorld.cells);
        // cout << "After energy conflict resolution:\n";
        // cout << "Num rules: " << newWorld.rules.size() << '\n';


        return newWorld;
    }

    vector<Cell> resolveEnergyConflicts(vector<Cell> cells) {

        unordered_map<pair<int,int>, vector<Cell>> map;

        for (Cell & cell : cells) {
            map[{cell.x, cell.y}].push_back(cell);
        }

        vector<Cell> newCells;

        for (auto el : map) {
            
            if (el.second.size() == 1) {
                newCells.push_back(el.second[0]);
                continue;
            }

            float newEnergy = 0;

            for (Cell cell : el.second) {
                newEnergy += cell.energy;
            }

            newCells.push_back(Cell{el.first.first, el.first.second, newEnergy, 'n'});
        }

        return newCells;
    }

    World updateFood() {

        World newWorld;
        newWorld.rules = rules;
        newWorld.cells = cells;
        newWorld.foods = foods;

        for (Food & food : newWorld.foods) {
            if (food.energy > 0) {
                for (Cell & cell : newWorld.cells) {
    
                    if ((food.x == cell.x && abs(food.y - cell.y) == 1) 
                    || (abs(food.x - cell.x) == 1 && food.y == cell.y)) {
    
                        food.energy -= 1;
                        cell.energy += 1;
                    }
                }
            }
        }
        
        return newWorld;
    }

    uint8_t getNextAction(uint8_t stateCode) {

        // cout << "GET NEXT ACTION - statecode: " << static_cast<int>(stateCode) << '\n';
        // cout << "RULES SIZE: " << rules.size() << '\n';
        // cout << "GET NEXT ACTION - ACTION: " << static_cast<int>(rules[stateCode]) << '\n';

        return rules[stateCode];
    }

    char decodeGrowthDir(uint8_t actionCode) {
        return dirs[actionCode / 5];
    }

    char decodeEnergyDir(uint8_t actionCode) {
        return dirs[actionCode % 5];
    }

    uint8_t getCellState(Cell cell) {
        
        uint8_t state = 0;
        
        for(Cell & c : cells) {
            if (c.x == cell.x - 1 && c.y == cell.y) { // neighbor left
                state += 1;
                if (c.energyDir == 'r') { // energy from left
                    state += 1 << 4;
                }
                continue;
            }
            if (c.x == cell.x + 1 && c.y == cell.y) { // neighbor right
                state += 1 << 1;
                if (c.energyDir == 'l') { // energy from right
                    state += 1 << 5;
                }
                continue;
            }
            if (c.x == cell.x && c.y == cell.y - 1) { // neighbor up
                state += 1 << 2;
                if (c.energyDir == 'd') { // energy from up
                    state += 1 << 6;
                }
                continue;
            }
            if (c.x == cell.x - 1 && c.y == cell.y + 1) { // neighbor down
                state += 1 << 3;
                if (c.energyDir == 'u') { // energy from down
                    state += 1 << 7;
                }
                continue;
            } // no neighbors, no energy received -> 0
        }

        return state;
    }
};
