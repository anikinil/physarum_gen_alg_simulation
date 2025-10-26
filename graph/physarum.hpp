#include <vector>
#include <memory>
#include <numeric>
#include <algorithm>

#include "utils.hpp"

#include "MiniDNN.h"


using namespace std;
using namespace MiniDNN;


typedef Eigen::MatrixXd Matrix;
typedef Eigen::VectorXd Vector;

// struct Junction;
// struct Tube;
// struct FoodSource;

const double DEFAULT_MUTATION_RATE = 0.05;
const int DEFAULT_FLOW_RATE = 1; 

const double TUBE_LENGTH = 10.0;

struct Junction;
struct Tube;
struct FoodSource;


struct Genome {
    vector<vector<double>> weights;

    const vector<pair<int, int>> GROW_NET_DIMS = {
        {5, 8}, // 5 x 8
        {8, 8}, // 8 x 8
        {8, 2}  // 8 x 2
    };

    const vector<pair<int, int>> FLOW_NET_DIMS = {
        {5, 8}, // input to hidden
        {8, 8}, // hidden to hidden
        {8, 2}  // hidden to output
    };

    Genome() {
        // Initialize weights with random values
        for (const auto& layer_dim : GROW_NET_DIMS) {
            int in = layer_dim.first;
            int out = layer_dim.second;
            // weight matrix (in x out)
            weights.push_back(Random::randvec(in * out, 0.0, 1.0));
            // bias vector (1 x out)
            weights.push_back(Random::randvec(out, 0.0, 1.0));
        }
    }


    vector<vector<double>> getGrowNetValues() const {
        vector<vector<double>> vals;
        for (size_t i = 0; i < GROW_NET_DIMS.size(); ++i) {
            vector<double> layer_params;
            // append weights
            layer_params.insert(layer_params.end(), weights[i*2].begin(), weights[i*2].end());
            // append biases
            layer_params.insert(layer_params.end(), weights[i*2+1].begin(), weights[i*2+1].end());
            vals.push_back(layer_params);
        }
    return vals;
}



    // vector<vector<double>> getFlowNetValues() const {
    //     vector<vector<double>> vals;
    //     for (const auto& layer_dim : FLOW_NET_DIMS) {
    //         int rows = layer_dim.first;
    //         int cols = layer_dim.second;
    //         int size = rows * cols;
    //         const auto& layer_weights = weights[&layer_dim - &FLOW_NET_DIMS[0]];
    //         vector<double> layer_vals;
    //         for (int i = 0; i < size; ++i) {
    //             layer_vals.push_back(static_cast<double>(layer_weights[i]));
    //         }
    //         vals.push_back(layer_vals);
    //     }
    //     return vals;
    // }

    void mutate(double mutation_rate = DEFAULT_MUTATION_RATE) {
        for (auto& layer_weights : weights) {
            for (auto& weight : layer_weights) {
                if (Random::uniform() < mutation_rate) {
                    weight += clamp(Random::uniform(-0.1, 0.1), 0.0, 1.0);
                }
            }
        }
    }
};


struct Junction {

    const double x;
    const double y;
    int energy;

    FoodSource* foodSource = nullptr;

    struct TubeInfo { Tube* tube; double angle; };
    vector<TubeInfo> inTubes;
    vector<TubeInfo> outTubes;

    int numInTubes() {
        return inTubes.size();
    }

    int numOutTubes() {
        return outTubes.size();
    }

    double averageAngleInTubes() {
        if (inTubes.empty()) return -1.0;
        double sum = accumulate(inTubes.begin(), inTubes.end(), 0.0,
            [](double acc, const TubeInfo& ti) { return acc + ti.angle; });
        return sum / inTubes.size();
    }

    double averageAngleOutTubes() {
        if (outTubes.empty()) return -1.0;
        double sum = accumulate(outTubes.begin(), outTubes.end(), 0.0,
            [](double acc, const TubeInfo& ti) { return acc + ti.angle; });
        return sum / outTubes.size();
    }

    bool isTouchingFoodSource() {
        return foodSource != nullptr;
    }
};

struct Tube {

    const double x1, y1;
    const double x2, y2;

    int flow_rate;

    Junction* fromJunction;
    Junction* toJunction;
};    

struct FoodSource {
    const double x;
    const double y;
    const double radius;
    int energy;
    // enum class FoodType { A, B, C } type;
};    


struct GrowthDecisionNet {
    Genome genome;

    int numberOfInTubes = 0;
    int numberOfOutTubes = 0;
    double averageAngleInTubes = 0.0;
    double averageAngleOutTubes = 0.0;
    bool touchingFoodSource = false;

    double growthProbability = 0.0;
    double growthAngle = 0.0;

    Network net;
    Matrix input;

    GrowthDecisionNet(const Genome& genome,
                    int numberOfInTubes = 0,
                    int numberOfOutTubes = 0,
                    double averageAngleInTubes = 0.0,
                    double averageAngleOutTubes = 0.0,
                    bool touchingFoodSource = false) {

        Layer* layer1 = new FullyConnected<Sigmoid>(genome.GROW_NET_DIMS[0].first, genome.GROW_NET_DIMS[0].second);
        Layer* layer2 = new FullyConnected<Sigmoid>(genome.GROW_NET_DIMS[1].first, genome.GROW_NET_DIMS[1].second);
        Layer* layer3 = new FullyConnected<Identity>(genome.GROW_NET_DIMS[2].first, genome.GROW_NET_DIMS[2].second);

        net.add_layer(layer1);
        net.add_layer(layer2);
        net.add_layer(layer3);
        net.set_output(new RegressionMSE());

        net.init();
        net.set_parameters(genome.getGrowNetValues());

        input = Matrix(5, 1);
        input << static_cast<double>(numberOfInTubes),
                static_cast<double>(numberOfOutTubes),
                averageAngleInTubes,
                averageAngleOutTubes,
                static_cast<double>(touchingFoodSource);

    }

    void decideAction() {
        Vector pred = net.predict(input);
        growthProbability = static_cast<double>(pred(0));
        growthAngle = static_cast<double>(pred(1));

        Printer::print("Growth Probability: " + to_string(growthProbability));
        Printer::print("Growth Angle: " + to_string(growthAngle));
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
    double fitness = 0.0;
    
    World(const Genome& genome,
        vector<unique_ptr<Junction>>&& js,
        vector<unique_ptr<FoodSource>>&& fs)
        : growthDecisionNet(genome),
        junctions(std::move(js)),
        foodSources(std::move(fs)) {}

    void growTubeFrom(Junction& from, double angle) {
        double newX = from.x + TUBE_LENGTH * cos(angle);
        double newY = from.y + TUBE_LENGTH * sin(angle);

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
        updateTubes();
        updateFood();
    }

    void updateJunctions() {
        // snapshot the current count of junctions before any growth

        removeDeadJunctions();

        size_t existingCount = junctions.size();

        for (size_t i = 0; i < existingCount; ++i) {
            Junction* junc = junctions[i].get();

            growthDecisionNet.numberOfInTubes = junc->numInTubes();
            growthDecisionNet.numberOfOutTubes = junc->numOutTubes();
            growthDecisionNet.averageAngleInTubes = junc->averageAngleInTubes();
            growthDecisionNet.averageAngleOutTubes = junc->averageAngleOutTubes();
            growthDecisionNet.touchingFoodSource = junc->isTouchingFoodSource();

            growthDecisionNet.decideAction();

            if (Random::uniform() < growthDecisionNet.growthProbability) {
                Printer::print("Growing tube from junction at (" + to_string(junc->x) + ", " + to_string(junc->y) + ") with angle " + to_string(growthDecisionNet.growthAngle));
                growTubeFrom(*junc, growthDecisionNet.growthAngle);
            }
        }
    }

    void removeDeadJunctions() {
        junctions.erase(
            std::remove_if(junctions.begin(), junctions.end(),
                [](const unique_ptr<Junction>& junc) {
                    return junc->energy <= -100;
                }),
            junctions.end());
    }
    
    void updateTubes() {}
    void updateFood() {}
};

