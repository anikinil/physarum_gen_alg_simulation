#include <vector>
#include <memory>
#include <numeric>
#include <algorithm>
#include <filesystem>

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

const double TUBE_LENGTH = 50.0;

struct Junction;
struct Tube;
struct FoodSource;


struct Genome {
    vector<vector<double>> weights;

    const vector<pair<int, int>> GROW_NET_DIMS = {
        {5, 8}, // 5 x 8
        {8, 8}, // 8 x 8
        {8, 3}  // 8 x 3
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

    const double x1;
    const double y1;
    const double x2;
    const double y2;

    int flowRate;

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
    double angleVariance = 0.0;

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
        growthAngle = clamp(static_cast<double>(pred(1)), 0.0, 2 * M_PI); // 0 to 360°
        angleVariance = clamp(static_cast<double>(pred(2)), 0.0, M_PI); // max 180°

        // Printer::print("Growth Probability: " + to_string(growthProbability));
        // Printer::print("Growth Angle: " + to_string(growthAngle));
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
        newJuncPtr->inTubes.push_back({newTube.get(), angle + M_PI});

        tubes.push_back(std::move(newTube));
        junctions.push_back(std::move(newJunction));
    }

    void run(int steps = 100, bool save = false) {

        if (save) {
            namespace fs = std::filesystem;
            fs::path p("data/frames.csv");
            std::error_code ec;
            if (p.has_parent_path()) {
                fs::create_directories(p.parent_path(), ec);
            }
            if (fs::exists(p, ec)) {
                auto sz = fs::file_size(p, ec);
                if (!ec && sz > 0) {
                    std::ofstream ofs(p, std::ios::trunc);
                }
            } else {
                std::ofstream ofs(p);
            }
        }

        if (save) {
            std::ofstream file("data/frames.csv", std::ios::app);
            file << "step,"
                 << "j_x,j_y,j_energy,"
                 << "t_x1,t_y1,t_x2,t_y2,t_flow_rate,"
                 << "f_x,f_y,f_radius,f_energy\n";
        }

        for (int step = 0; step < steps; ++step) {
            this->step();
            if (save) {
                this->saveFrame(step);
            }
        }
    }

    void saveFrame(int step) {
        std::ofstream file("data/frames.csv", std::ios::app);

        for (const auto& junc : junctions) {
            file << step << ','
                 << junc->x << ','
                 << junc->y << ','
                 << junc->energy << ','
                 << ",,,,,"
                 << ",,,\n";
        }
        for (const auto& tube : tubes) {
            file << step << ','
                 << ",,,"
                 << tube->x1 << ',' << tube->y1 << ','
                 << tube->x2 << ',' << tube->y2 << ','
                 << tube->flowRate << ','
                 << ",,,\n";
        }
        for (const auto& fs : foodSources) {
            file << step << ','
                 << ",,,"
                 << ",,,,,"
                 << fs->x << ',' << fs->y << ',' << fs->radius << ','
                 << fs->energy << "\n";
        }
    }

    void step() {
        updateJunctions();
        updateTubes();
        updateFood();
    }

    void updateJunctions() {

        removeDeadJunctions();

        size_t existingCount = junctions.size();
        // Printer::print("Existing count: " + to_string(existingCount));

        for (size_t i = 0; i < existingCount; ++i) {
            
            Junction* junc = junctions[i].get();

            growthDecisionNet.numberOfInTubes = junc->numInTubes();
            growthDecisionNet.numberOfOutTubes = junc->numOutTubes();
            growthDecisionNet.averageAngleInTubes = junc->averageAngleInTubes();
            growthDecisionNet.averageAngleOutTubes = junc->averageAngleOutTubes();
            growthDecisionNet.touchingFoodSource = junc->isTouchingFoodSource();

            growthDecisionNet.decideAction();

            // TODO detect collision and if collides, put the new junction on nearest tube in the way

            if (Random::uniform() < growthDecisionNet.growthProbability) {
                // Printer::print("Growing tube from junction at (" + to_string(junc->x) + ", " + to_string(junc->y) + ") with angle " + to_string(growthDecisionNet.growthAngle));
                growTubeFrom(*junc, growthDecisionNet.growthAngle + 
                    Random::uniform(-growthDecisionNet.angleVariance, growthDecisionNet.angleVariance));
            }
        }
    }

    void removeDeadJunctions() {
        junctions.erase(
            std::remove_if(junctions.begin(), junctions.end(),
                [](const unique_ptr<Junction>& junc) {
                    return junc->energy <= -100; // TODO set to 0 (energy depletion condition)
                }),
            junctions.end());
    }
    
    void updateTubes() {}
    void updateFood() {}
};

