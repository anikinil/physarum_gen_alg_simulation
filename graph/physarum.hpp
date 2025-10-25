#include <vector>
#include <memory>
#include <numeric>

#include "utils.hpp"

using namespace std;

// struct Junction;
// struct Tube;
// struct FoodSource;

const float DEFAULT_MUTATION_RATE = 0.05f;
const int DEFAULT_FLOW_RATE = 1; 

struct Tube {

    float x1, y1;
    float x2, y2;

    int flow_rate;

    Junction* from;
    Junction* to;
};

struct Junction {

    float x;
    float y;
    int energy;

    FoodSource* food_source = nullptr;

    struct TubeInfo { Tube* tube; float angle; };
    vector<TubeInfo> in_tubes;
    vector<TubeInfo> out_tubes;
};

struct FoodSource {
    float x;
    float y;
    int energy;
    // enum class FoodType { A, B, C } type;
};

struct ActionParameter {
    float value;
    const float  min_value;
    const float max_value;
    float mutation_rate = DEFAULT_MUTATION_RATE;
};

struct JunctionAction {
    // probability to grow a new tube at each junction per time step
    ActionParameter growth_probability{Random::uniform(), 0.0f, 1.0f};
    // how much to turn from average angle when growing new tube
    ActionParameter angle_shift{Random::uniform(-180.0f, 180.0f), -180.0f, 180.0f};
    // noise in angle when growing new tube
    ActionParameter angle_noise{Random::uniform(-180.0f, 180.0f), -180.0f, 180.0f};
};

struct TubeAction {
    // how much to increase flow rate when energy is high
    ActionParameter flow_increase_factor{Random::uniform(1.0f, 3.0f), 1.0f, 5.0f};
    // how much to decrease flow rate when energy is low
    ActionParameter flow_decrease_factor{Random::uniform(0.2f, 1.0f), 0.0f, 1.0f};
    // energy thresholds
    ActionParameter high_energy_threshold{Random::uniform(5.0f, 20.0f), 0.0f, 100.0f};
    ActionParameter low_energy_threshold{Random::uniform(0.0f, 5.0f), 0.0f, 100.0f};
};

struct Action {
    JunctionAction junction_action;
    TubeAction tube_action;
};

struct World {

    vector<unique_ptr<Junction>> junctions;
    vector<unique_ptr<Tube>> tubes;
    vector<unique_ptr<FoodSource>> food_sources;

    Action action;

    float fitness = 0.0;
    
    void step() {

        updateJunctions();
        updateTubes();
        updateFood();

        // render();
    }

    void updateJunctions() {
        
        for (auto &junction : junctions) {
            
            // Placeholder for junction update logic
        }

    }

    void updateTubes() {
        // Placeholder for tube update logic
    }

    void updateFood() {
        // Placeholder for food update logic
    }

};
