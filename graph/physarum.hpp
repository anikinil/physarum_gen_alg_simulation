#include <vector>
#include <memory>
#include <numeric>

#include "utils.hpp"

#include "MiniDNN.h"


using namespace std;
using namespace MiniDNN;


typedef Eigen::MatrixXd Matrix;
typedef Eigen::VectorXd Vector;

// struct Junction;
// struct Tube;
// struct FoodSource;

const float DEFAULT_MUTATION_RATE = 0.05f;
const int DEFAULT_FLOW_RATE = 1; 

struct Tube {

    const float x1, y1;
    const float x2, y2;

    int flow_rate;

    Junction* from;
    Junction* to;
};

struct Junction {

    const float x;
    const float y;
    int energy;

    FoodSource* foodSource = nullptr;

    struct TubeInfo { Tube* tube; float angle; };
    vector<TubeInfo> in_tubes;
    vector<TubeInfo> out_tubes;
};

struct FoodSource {
    const float x;
    const float y;
    int energy;
    // enum class FoodType { A, B, C } type;
};

struct GrowthDecisionNet {
    vector<vector<double>> genome;

    int numberOfInTubes = 0;
    int numberOfOutTubes = 0;
    float averageAngleInTubes = 0.0f;
    float averageAngleOutTubes = 0.0f;
    bool touchingFoodSource = false;

    float growth_probability = 0.0f;
    float growth_angle = 0.0f;

    Network net;
    Matrix input;

    GrowthDecisionNet(const vector<vector<double>>& genome,
                    int numberOfInTubes = 0,
                    int numberOfOutTubes = 0,
                    float averageAngleInTubes = 0.0f,
                    float averageAngleOutTubes = 0.0f,
                    bool touchingFoodSource = false) {

        Layer* layer1 = new FullyConnected<Tanh>(5, 8);
        Layer* layer2 = new FullyConnected<Tanh>(8, 8);
        Layer* layer3 = new FullyConnected<Identity>(8, 2);

        net.add_layer(layer1);
        net.add_layer(layer2);
        net.add_layer(layer3);
        net.set_output(new RegressionMSE());

        net.set_parameters(genome);

        input = Matrix({
            static_cast<double>(numberOfInTubes),
            static_cast<double>(numberOfOutTubes),
            static_cast<double>(averageAngleInTubes),
            static_cast<double>(averageAngleOutTubes),
            static_cast<double>(touchingFoodSource)
        });
    }

    void decideAction() {
        Vector pred = net.predict(input);
        growth_probability = static_cast<float>(pred(0));
        growth_angle = static_cast<float>(pred(1));
    }
};


struct World {
    vector<unique_ptr<Junction>> junctions;
    vector<unique_ptr<Tube>> tubes;
    vector<unique_ptr<FoodSource>> foodSources;

    GrowthDecisionNet growthDecisionNet;
    float fitness = 0.0f;
        
    World(const vector<vector<double>>& genome,
        vector<unique_ptr<Junction>>&& js,
        vector<unique_ptr<FoodSource>>&& fs)
        : growthDecisionNet(genome),
        junctions(std::move(js)),
        foodSources(std::move(fs)) {}


    void step() {
        updateJunctions();
        updateTubes();
        updateFood();
    }

    void updateJunctions() {}
    void updateTubes() {}
    void updateFood() {}
};

