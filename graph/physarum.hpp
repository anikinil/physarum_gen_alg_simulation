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

const float TUBE_LENGTH = 10.0f;



struct Genome {
    vector<vector<float>> weights;

    const vector<pair<int, int>> GROW_NET_DIMS = {
        {6, 8}, // 5+1 x 8
        {9, 8}, // 8+1 x 8
        {9, 2}  // 8+1 x 2
    };

    const vector<pair<int, int>> FLOW_NET_DIMS = {
        {5, 8}, // input to hidden
        {8, 8}, // hidden to hidden
        {8, 2}  // hidden to output
    };

    Genome() {
        // Initialize weights with random values
        for (const auto& layer_dim : GROW_NET_DIMS) {
            int rows = layer_dim.first;
            int cols = layer_dim.second;
            weights.push_back(Random::randvec(rows * cols, -1.0f, 1.0f));
        }
    }

    vector<vector<double>> getGrowNetValues() const {
        vector<vector<double>> vals;
        for (const auto& layer_dim : GROW_NET_DIMS) {
            int rows = layer_dim.first;
            int cols = layer_dim.second;
            int size = rows * cols;
            const auto& layer_weights = weights[&layer_dim - &GROW_NET_DIMS[0]];
            vector<double> layer_vals;
            for (int i = 0; i < size; ++i) {
                layer_vals.push_back(static_cast<double>(layer_weights[i]));
            }
            vals.push_back(layer_vals);
        }
        return vals;
    }

    vector<vector<double>> getFlowNetValues() const {
        vector<vector<double>> vals;
        for (const auto& layer_dim : FLOW_NET_DIMS) {
            int rows = layer_dim.first;
            int cols = layer_dim.second;
            int size = rows * cols;
            const auto& layer_weights = weights[&layer_dim - &FLOW_NET_DIMS[0]];
            vector<double> layer_vals;
            for (int i = 0; i < size; ++i) {
                layer_vals.push_back(static_cast<double>(layer_weights[i]));
            }
            vals.push_back(layer_vals);
        }
        return vals;
    }

};

struct Tube {

    const float x1, y1;
    const float x2, y2;

    int flow_rate;

    Junction* fromJunction;
    Junction* toJunction;
};

struct Junction {

    const float x;
    const float y;
    int energy;

    FoodSource* foodSource = nullptr;

    struct TubeInfo { Tube* tube; float angle; };
    vector<TubeInfo> inTubes;
    vector<TubeInfo> outTubes;

    int numInTubes() {
        return inTubes.size();
    }

    int numOutTubes() {
        return outTubes.size();
    }

    float averageAngleInTubes() {
        if (inTubes.empty()) return -1.0f;
        float sum = accumulate(inTubes.begin(), inTubes.end(), 0.0f,
            [](float acc, const TubeInfo& ti) { return acc + ti.angle; });
        return sum / inTubes.size();
    }

    float averageAngleOutTubes() {
        if (outTubes.empty()) return -1.0f;
        float sum = accumulate(outTubes.begin(), outTubes.end(), 0.0f,
            [](float acc, const TubeInfo& ti) { return acc + ti.angle; });
        return sum / outTubes.size();
    }

    bool isTouchingFoodSource() {
        return foodSource != nullptr;
    }
};

struct FoodSource {
    const float x;
    const float y;
    const float radius;
    int energy;
    // enum class FoodType { A, B, C } type;
};

struct GrowthDecisionNet {
    Genome genome;

    int numberOfInTubes = 0;
    int numberOfOutTubes = 0;
    float averageAngleInTubes = 0.0f;
    float averageAngleOutTubes = 0.0f;
    bool touchingFoodSource = false;

    float growthProbability = 0.0f;
    float growthAngle = 0.0f;

    Network net;
    Matrix input;

    GrowthDecisionNet(const Genome& genome,
                    int numberOfInTubes = 0,
                    int numberOfOutTubes = 0,
                    float averageAngleInTubes = 0.0f,
                    float averageAngleOutTubes = 0.0f,
                    bool touchingFoodSource = false) {

        Layer* layer1 = new FullyConnected<Tanh>(genome.GROW_NET_DIMS[0].first, genome.GROW_NET_DIMS[0].second);
        Layer* layer2 = new FullyConnected<Tanh>(genome.GROW_NET_DIMS[1].first, genome.GROW_NET_DIMS[1].second);
        Layer* layer3 = new FullyConnected<Identity>(genome.GROW_NET_DIMS[2].first, genome.GROW_NET_DIMS[2].second);

        net.add_layer(layer1);
        net.add_layer(layer2);
        net.add_layer(layer3);
        // net.set_output(new RegressionMSE());

        net.set_parameters(genome.getGrowNetValues());

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
        growthProbability = static_cast<float>(pred(0));
        growthAngle = static_cast<float>(pred(1));
    }
};

struct FlowDecisionNet {
    // To be implemented

    // TODO vote on prob of increasing/decreasing flow rate based on inputs
};


struct World {
    vector<unique_ptr<Junction>> junctions;
    vector<unique_ptr<Tube>> tubes;
    vector<unique_ptr<FoodSource>> foodSources;

    GrowthDecisionNet growthDecisionNet;
    float fitness = 0.0f;
        
    World(const Genome& genome,
        vector<unique_ptr<Junction>>&& js,
        vector<unique_ptr<FoodSource>>&& fs)
        : growthDecisionNet(genome),
        junctions(std::move(js)),
        foodSources(std::move(fs)) {}

    void growTubeFrom(Junction& from, float angle) {
        float newX = from.x + TUBE_LENGTH * cos(angle);
        float newY = from.y + TUBE_LENGTH * sin(angle);

        auto newJunction = std::make_unique<Junction>(Junction{newX, newY, 0});
        Junction* newJuncPtr = newJunction.get();

        auto newTube = std::make_unique<Tube>(Tube{
            from.x, from.y, newX, newY, DEFAULT_FLOW_RATE, &from, newJuncPtr
        });

        from.outTubes.push_back({newTube.get(), angle});
        newJuncPtr->inTubes.push_back({newTube.get(), angle + M_PIf});

        tubes.push_back(std::move(newTube));
        junctions.push_back(std::move(newJunction));
    }


    void step() {
        updateJunctions();
        Printer::print(junctions[0]->numOutTubes());
        updateTubes();
        updateFood();
    }

    void updateJunctions() {

        for (const auto& junc: junctions) {
            growthDecisionNet.numberOfInTubes = junc->numInTubes();
            growthDecisionNet.numberOfOutTubes = junc->numOutTubes();
            growthDecisionNet.averageAngleInTubes = junc->averageAngleInTubes();
            growthDecisionNet.averageAngleOutTubes = junc->averageAngleOutTubes();
            growthDecisionNet.touchingFoodSource = junc->isTouchingFoodSource();

            growthDecisionNet.decideAction();

            if (Random::uniform() < growthDecisionNet.growthProbability) {
                growTubeFrom(*junc, growthDecisionNet.growthAngle);
            }
        }
    }
    
    void updateTubes() {}
    void updateFood() {}
};

