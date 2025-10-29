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

const double MIN_GROWTH_ENERGY = 2.0;

const double DEFAULT_MUTATION_RATE = 1.0;
const double MUTATION_STRENGTH = 0.2;

const double DEFAULT_FLOW_RATE = 1.0;
const double TUBE_LENGTH = 20.0;


const vector<pair<int, int>> GROW_NET_DIMS = {
    {6, 8}, // 6 x 8
    {8, 8}, // 8 x 8
    {8, 3}  // 8 x 3
    // + 8 + 8 + 3 
    // total: 155
};

const vector<pair<int, int>> FLOW_NET_DIMS = {
    {3, 8},
    {8, 8},
    {8, 2}
    // total: 122
};
// total genome size: 277


struct Junction;
struct Tube;
struct FoodSource;


struct Genome {
    vector<vector<double>> growNetWeights;
    vector<vector<double>> flowNetWeights;

    Genome() {

        // Initialize weights with random values
        for (const auto& layer_dim : GROW_NET_DIMS) {
            int in = layer_dim.first;
            int out = layer_dim.second;
            // weight matrix (in x out)
            growNetWeights.push_back(Random::randvec(in * out, 0.0, 1.0));
            // bias vector (1 x out)
            growNetWeights.push_back(Random::randvec(out, 0.0, 1.0));
        }

        for (const auto& layer_dim : FLOW_NET_DIMS) {
            int in = layer_dim.first;
            int out = layer_dim.second;
            // weight matrix (in x out)
            flowNetWeights.push_back(Random::randvec(in * out, 0.0, 1.0));
            // bias vector (1 x out)
            flowNetWeights.push_back(Random::randvec(out, 0.0, 1.0));
        }
    }

    vector<vector<double>> getGrowNetValues() const {
        vector<vector<double>> vals;
        for (size_t i = 0; i < GROW_NET_DIMS.size(); ++i) {
            vector<double> layer_params;
            // append weights
            layer_params.insert(layer_params.end(), growNetWeights[i*2].begin(), growNetWeights[i*2].end());
            // append biases
            layer_params.insert(layer_params.end(), growNetWeights[i*2+1].begin(), growNetWeights[i*2+1].end());
            vals.push_back(layer_params);
        }
        return vals;
    }

    vector<vector<double>> getFlowNetValues() const {
        vector<vector<double>> vals;
        for (size_t i = 0; i < FLOW_NET_DIMS.size(); ++i) {
            vector<double> layer_params;
            // append weights
            layer_params.insert(layer_params.end(), flowNetWeights[i*2].begin(), flowNetWeights[i*2].end());
            // append biases
            layer_params.insert(layer_params.end(), flowNetWeights[i*2+1].begin(), flowNetWeights[i*2+1].end());
            vals.push_back(layer_params);
        }
        return vals;
    }

    void setGenomeValues(const vector<double>& growVals,
                         const vector<double>& flowVals) {
        growNetWeights.clear();
        flowNetWeights.clear();
        
        size_t growIdx = 0;
        for (const auto& layer_dim : GROW_NET_DIMS) {
            int in = layer_dim.first;
            int out = layer_dim.second;
            size_t weight_size = in * out;
            size_t bias_size = out;

            vector<double> weights(growVals.begin() + growIdx,
                                   growVals.begin() + growIdx + weight_size);
            growNetWeights.push_back(weights);
            growIdx += weight_size;

            vector<double> biases(growVals.begin() + growIdx,
                                  growVals.begin() + growIdx + bias_size);
            growNetWeights.push_back(biases);
            growIdx += bias_size;
        }
    }

    void mutate(double mutation_rate = DEFAULT_MUTATION_RATE) {
        for (auto& layer_weights : growNetWeights) {
            for (auto& weight : layer_weights) {
                if (Random::uniform() < mutation_rate) {
                    double change = Random::uniform(-MUTATION_STRENGTH, MUTATION_STRENGTH);
                    weight += clamp(change, 0.0, 1.0);
                }
            }
        }
        for (auto& layer_weights : flowNetWeights) {
            for (auto& weight : layer_weights) {
                if (Random::uniform() < mutation_rate) {
                    double change = Random::uniform(-MUTATION_STRENGTH, MUTATION_STRENGTH);
                    weight += clamp(change, 0.0, 1.0);
                }
            }
        }
    }
};


struct Tube {

    const double x1;
    const double y1;
    const double x2;
    const double y2;

    double flowRate;

    Junction* fromJunction;
    Junction* toJunction;
};    

struct Junction {

    const double x;
    const double y;
    double energy;

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

    double getSummedFlowRate() {
        double totalFlow = 0.0;
        for (const auto& t : inTubes) {
            totalFlow += t.tube->flowRate;
        }
        for (const auto& t : outTubes) {
            totalFlow -= t.tube->flowRate;
        }
        return totalFlow;
    }

    // moves tube from inTubes to outTubes or vice versa
    void switchTubeDirection(Tube& tube) {
        auto it_in = std::find_if(inTubes.begin(), inTubes.end(),
            [&tube](const TubeInfo& ti) { return ti.tube == &tube; });
        if (it_in != inTubes.end()) {
            outTubes.push_back(*it_in);
            inTubes.erase(it_in);
            return;
        }
        auto it_out = std::find_if(outTubes.begin(), outTubes.end(),
            [&tube](const TubeInfo& ti) { return ti.tube == &tube; });
        if (it_out != outTubes.end()) {
            inTubes.push_back(*it_out);
            outTubes.erase(it_out);
            return;
        }
    }
};  


struct FoodSource {
    const double x;
    const double y;
    const double radius;
    double energy;
    // enum class FoodType { A, B, C } type;
};


struct GrowthDecisionNet {
    Genome genome;


    int numberOfInTubes = 0;
    int numberOfOutTubes = 0;
    double averageInTubeAngle = 0.0;
    double averageOutTubeAngle = 0.0;
    double energy = 0.0;
    bool touchingFoodSource = false;
    
    double growthProbability = 0.0;
    double growthAngle = 0.0;
    double angleVariance = 0.0;

    Network net;
    Matrix input;

    GrowthDecisionNet(const Genome& genome,
                    int numberOfInTubes = 0,
                    int numberOfOutTubes = 0,
                    double averageInTubeAngle = 0.0,
                    double averageOutTubeAngle = 0.0,
                    double energy = 0.0,
                    bool touchingFoodSource = false) {

        Layer* layer1 = new FullyConnected<Sigmoid>(GROW_NET_DIMS[0].first, GROW_NET_DIMS[0].second);
        Layer* layer2 = new FullyConnected<Sigmoid>(GROW_NET_DIMS[1].first, GROW_NET_DIMS[1].second);
        Layer* layer3 = new FullyConnected<Sigmoid>(GROW_NET_DIMS[2].first, GROW_NET_DIMS[2].second);

        net.add_layer(layer1);
        net.add_layer(layer2);
        net.add_layer(layer3);
        net.set_output(new RegressionMSE());

        net.init();
        
        net.set_parameters(genome.getGrowNetValues());
        
        
        input = Matrix(6, 1);
        input << static_cast<double>(numberOfInTubes),
        static_cast<double>(numberOfOutTubes),
        static_cast<double>(averageInTubeAngle),
        static_cast<double>(averageOutTubeAngle),
        static_cast<double>(energy),
        static_cast<double>(touchingFoodSource);
    }
    
    void decideAction() {


        vector<vector<double>> params = net.get_parameters();

        Vector pred = net.predict(input);
        growthProbability = static_cast<double>(pred(0));
        growthAngle = clamp(static_cast<double>(pred(1)), 0.0, 2 * M_PI); // 0 to 360°
        angleVariance = clamp(static_cast<double>(pred(2)), 0.0, M_PI); // max 180°
    }
};

struct FlowDecisionNet {

    double currentFlowRate = 0.0;
    double inJunctionAverageFlowRate = 0.0;
    double outJunctionAverageFlowRate = 0.0;

    double increaseFlowProb = 0.0;
    double decreaseFlowProb = 0.0;

    Network net;
    Matrix input;

    FlowDecisionNet(const Genome& genome,
                    double currentFlowRate = 0.0,
                    double inJunctionAverageFlowRate = 0.0,
                    double outJunctionAverageFlowRate = 0.0) {

        Layer* layer1 = new FullyConnected<Sigmoid>(FLOW_NET_DIMS[0].first, FLOW_NET_DIMS[0].second);
        Layer* layer2 = new FullyConnected<Sigmoid>(FLOW_NET_DIMS[1].first, FLOW_NET_DIMS[1].second);
        Layer* layer3 = new FullyConnected<Sigmoid>(FLOW_NET_DIMS[2].first, FLOW_NET_DIMS[2].second);

        net.add_layer(layer1);
        net.add_layer(layer2);
        net.add_layer(layer3);
        net.set_output(new RegressionMSE());

        net.init();
        net.set_parameters(genome.getFlowNetValues());

        input = Matrix(3, 1);
        input << currentFlowRate,
                inJunctionAverageFlowRate,
                outJunctionAverageFlowRate;

    }

    void decideAction() {
        Vector pred = net.predict(input);
        increaseFlowProb = clamp(static_cast<double>(pred(0)), 0.0, 1.0);
        decreaseFlowProb = clamp(static_cast<double>(pred(1)), 0.0, 1.0);
    }
};


struct World {
    Genome genome;

    vector<unique_ptr<Junction>> junctions;
    vector<unique_ptr<Tube>> tubes;
    vector<unique_ptr<FoodSource>> foodSources;

    GrowthDecisionNet growthDecisionNet;
    FlowDecisionNet flowDecisionNet;
    double fitness = 0.0;
    
    World(const Genome& g,
        vector<unique_ptr<Junction>>&& js,
        vector<unique_ptr<FoodSource>>&& fs)
        : genome(g),
        growthDecisionNet(g),
        flowDecisionNet(g),
        junctions(std::move(js)),
        foodSources(std::move(fs)) {}

    Genome getGenome() const {
        return genome;
    }

    void mutateGenome(double mutation_rate = DEFAULT_MUTATION_RATE) {
        genome.mutate(mutation_rate);
    }

    void growTubeFrom(Junction& from, double angle) {

        double newX = from.x + TUBE_LENGTH * cos(angle);
        double newY = from.y + TUBE_LENGTH * sin(angle);

        // TODO detect collision here

        auto newJunction = std::make_unique<Junction>(Junction{newX, newY, 1});
        Junction* newJuncPtr = newJunction.get();

        from.energy -= 1;

        auto newTube = std::make_unique<Tube>(Tube{
            from.x, from.y, newX, newY, 1, &from, newJuncPtr
        });

        from.outTubes.push_back({newTube.get(), angle});
        newJuncPtr->inTubes.push_back({newTube.get(), angle + M_PI});

        tubes.push_back(std::move(newTube));
        junctions.push_back(std::move(newJunction));
    }

    void run(int steps = 100, bool save = false) {

        if (save) {
            namespace fs = std::filesystem;
            fs::path p("data/animation_frames.csv");
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
            std::ofstream file("data/animation_frames.csv", std::ios::app);
            file << "step,"
                 << "j_x,j_y,j_energy,"
                 << "t_x1,t_y1,t_x2,t_y2,t_flow_rate,"
                 << "f_x,f_y,f_radius,f_energy\n";
        }

        for (int step = 0; step < steps; ++step) {
            if (save) {
                this->saveFrame(step);
            }
            this->step();
        }
    }

    void saveFrame(int step) {
        std::ofstream file("data/animation_frames.csv", std::ios::app);

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

        removeDeadJunctionsAndTubes();

        size_t existingCount = junctions.size();


        for (size_t i = 0; i < existingCount; ++i) {
            
            Junction* junc = junctions[i].get();

            growthDecisionNet.numberOfInTubes = junc->numInTubes();
            growthDecisionNet.numberOfOutTubes = junc->numOutTubes();
            growthDecisionNet.averageInTubeAngle = junc->averageAngleInTubes();
            growthDecisionNet.averageOutTubeAngle = junc->averageAngleOutTubes();
            growthDecisionNet.energy = junc->energy;
            growthDecisionNet.touchingFoodSource = junc->isTouchingFoodSource();

            growthDecisionNet.decideAction();

            // TODO detect collision and if collides, put the new junction on nearest tube in the way


            if (Random::uniform() < growthDecisionNet.growthProbability && junc->energy > MIN_GROWTH_ENERGY) {
                growTubeFrom(*junc, growthDecisionNet.growthAngle + 
                    Random::uniform(-growthDecisionNet.angleVariance, growthDecisionNet.angleVariance));                
            }

            junc->energy += 1 * junc->getSummedFlowRate();
        }
    }

    void removeDeadJunctionsAndTubes() {
        // 1. Collect pointers to dead junctions
        std::vector<Junction*> dead;
        for (auto& j : junctions)
            if (j->energy <= 0)
                dead.push_back(j.get());

        // 2. Remove tubes connected to dead junctions
        tubes.erase(
            std::remove_if(tubes.begin(), tubes.end(),
                [&](const std::unique_ptr<Tube>& t) {
                    return std::find(dead.begin(), dead.end(), t->fromJunction) != dead.end() ||
                        std::find(dead.begin(), dead.end(), t->toJunction)   != dead.end();
                }),
            tubes.end());

        // 3. Remove the dead junctions themselves
        junctions.erase(
            std::remove_if(junctions.begin(), junctions.end(),
                [](const std::unique_ptr<Junction>& j) {
                    return j->energy <= 0;
                }),
            junctions.end());
    }

    
    void updateTubes() {

        for (auto& tube : tubes) {
            
            flowDecisionNet.currentFlowRate = tube->flowRate;
            flowDecisionNet.inJunctionAverageFlowRate = static_cast<double>(tube->fromJunction->getSummedFlowRate());
            flowDecisionNet.outJunctionAverageFlowRate = static_cast<double>(-tube->toJunction->getSummedFlowRate());

            flowDecisionNet.decideAction();

            // cout << "increase prob: " << flowDecisionNet.increaseFlowProb << ", decrease_prob: " << flowDecisionNet.decreaseFlowProb << endl;

            if (Random::uniform() < flowDecisionNet.increaseFlowProb) {
                // cout << "INCREASE" << endl;
                tube->flowRate += 1;
            }
            if (Random::uniform() < flowDecisionNet.decreaseFlowProb && tube->flowRate > 0) {
                // cout << "DECREASE" << endl;
                tube->flowRate -= 1;
            }

            // cout << "-------------" << endl;

            if (tube->flowRate < 0) {
                std::swap(tube->fromJunction, tube->toJunction);
                tube->flowRate = -tube->flowRate;
                tube->fromJunction->switchTubeDirection(*tube);
                tube->toJunction->switchTubeDirection(*tube);
            }
        }
    }

    void updateFood() {

        for (auto& junc : junctions) {

            if (junc->isTouchingFoodSource()) {
                junc->energy += 1; // absorb food energy
                junc->foodSource->energy -= 1;
                if (junc->foodSource->energy < 0) {
                    junc->foodSource = nullptr; // food source depleted
                }
            }
        }
    }

    double calculateFitness() {
        
        double fitnessSum = 0.0;
        for (const auto& junc : junctions) {
            double dist = sqrt(junc->x * junc->x + junc->y * junc->y);
            fitnessSum += junc->energy * dist;
        }
        fitness = fitnessSum;
        return fitness;
    }
};

