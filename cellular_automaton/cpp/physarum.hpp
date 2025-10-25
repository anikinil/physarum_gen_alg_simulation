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

float MIN_GROWTH_ENERGY = 10;
float ENERGY_PORTION = 0.2;
float MIN_ENERGY_TO_PASS_ENERGY = 0.5;
float MIN_ENERGY_TO_SIGNAL = 0.2;
float SIGNAL_COST = 0.01;

const int MEMORY_SIZE = 2;

constexpr int STATE_SPACE = 262144; // 2^8 (neighbors) * 2^4 (energy dir) * 2^(MEMORY_SIZE*2) (memory state)

const int ACTION_SPACE = 125;

const int SIGNALS_PER_STEP = 2;


struct Cell {
    int x;
    int y;
    float energy;
    char energyDir;
    unsigned char memory;
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
        for (int i = 0; i < SIGNALS_PER_STEP; i++) {
            newWorld = newWorld.updateSignals();
        }
        return newWorld;
    }

    World updateGrowth() {

        World newWorld;
        newWorld.rules = rules;
        newWorld.foods = foods;

        for (Cell & cell : cells) {

            uint16_t state = getCellState(cell);
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

    Cell *getCellAt(int x, int y) {
        for (Cell & cell : cells) {
            if (cell.x == x && cell.y == y) return &cell;
        }
        return nullptr;
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
        
        for (Cell & cell : cells) {

            uint16_t state = getCellState(cell);
            uint8_t action = getNextAction(state);
            
            char energyDir = decodeEnergyDir(action);

            // no energy passed
            if (energyDir == 'n' || cell.energy * ENERGY_PORTION < MIN_ENERGY_TO_PASS_ENERGY) {
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

            Cell *targetCell = getCellAt(targetX, targetY);

            // if target cell does not exist, do not pass energy
            if (targetCell == nullptr) {
                Cell newCell = cell;
                newWorld.cells.push_back(newCell);
                continue;
            }

            Cell updatedOrigCell = cell;
            updatedOrigCell.energy *= (1 - ENERGY_PORTION);
            updatedOrigCell.energyDir = energyDir;
            newWorld.cells.push_back(updatedOrigCell);
            newWorld.cells.push_back(Cell{targetX, targetY, targetCell->energy * ENERGY_PORTION, 'n'});
        }

        newWorld.cells = resolveEnergyConflicts(newWorld.cells);

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
                        if (food.energy <= 0) break;
                    }
                }
            }
        }

        // remove depleted foods
        vector<Food> remainingFoods;
        for (const Food & f : newWorld.foods) {
            if (f.energy > 0) remainingFoods.push_back(f);
        }
        newWorld.foods = std::move(remainingFoods);

        return newWorld;
    }

    World updateSignals() {

        // helper hash function
        struct pair_hash { 
            size_t operator()(
                const std::pair<int,int> &p)
                const noexcept { 
                    return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1); 
                } };

        World newWorld;
        newWorld.rules = rules;
        newWorld.foods = foods;

        unordered_map<pair<int,int>, Cell, pair_hash> newCellsMap;

        // Copy current cells into map
        for (Cell &cell : cells) {
            newCellsMap[{cell.x, cell.y}] = cell;
        }

        for (Cell &cell : cells) {
            uint16_t state = getCellState(cell);
            uint8_t action = getNextAction(state);
            char signalDir = decodeSignalDir(action);

            if (signalDir == 'n' || cell.energy < MIN_ENERGY_TO_SIGNAL) continue;

            int targetX = cell.x, targetY = cell.y;
            int val = -1;
            if (signalDir == 'l') { targetX--; val = 0; }
            else if (signalDir == 'r') { targetX++; val = 1; }
            else if (signalDir == 'u') { targetY++; val = 2; }
            else if (signalDir == 'd') { targetY--; val = 3; }

            auto it = newCellsMap.find({targetX, targetY});
            if (it == newCellsMap.end()) continue; // no target

            // Update sender
            newCellsMap[{cell.x, cell.y}].energy -= SIGNAL_COST;

            std::cout << "----" << endl;
            // Update receiver memory safely
            uint8_t &mem = it->second.memory;
            mem = ((mem << 2) | val) & ((1 << (MEMORY_SIZE*2)) - 1);
            cout << mem << endl;
        }

        // Move map into vector
        for (auto &[pos, cell] : newCellsMap) {
            newWorld.cells.push_back(cell);
        }

        return newWorld;
    }


    uint8_t getNextAction(uint16_t stateCode) {

        return rules[stateCode];
    }

    char decodeGrowthDir(uint8_t actionCode) {
        return dirs[actionCode / 5];
    }

    char decodeEnergyDir(uint8_t actionCode) {
        return dirs[actionCode % 5];
    }

    char decodeSignalDir(uint8_t actionCode) {
        return dirs[(actionCode / 25) % 5];
    }

    uint16_t getCellState(Cell cell) {
        
        uint16_t state = 0;
        
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
            }
            if (c.x == cell.x - 1 && c.y == cell.y - 1) { // neighbor left up
                state += 1 << 8;
                continue;
            }
            if (c.x == cell.x + 1 && c.y == cell.y - 1) { // neighbor right up
                state += 1 << 9;
                continue;
            }
            if (c.x == cell.x - 1 && c.y == cell.y + 1) { // neighbor left down
                state += 1 << 10;
                continue;
            }
            if (c.x == cell.x + 1 && c.y == cell.y + 1) { // neighbor right down
                state += 1 << 11;
                continue;
            }   
        }
        // cout << static_cast<int>(cell.memory) << endl;
        state |= (static_cast<uint16_t>(cell.memory & ((1 << (MEMORY_SIZE*2)) - 1)) << 12);

        return state;
    }
};
