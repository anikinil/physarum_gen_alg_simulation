#include <vector>
#include <memory>
#include <numeric>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <utility>
#include <cmath>


#include "utils.hpp"

#include "MiniDNN.h"


using namespace std;
using namespace MiniDNN;


typedef Eigen::MatrixXd Matrix;
typedef Eigen::VectorXd Vector;

const double MIN_GROWTH_ENERGY = 2.0;
const double DEFAULT_JUNCTION_ENERGY = 1.0;
const double GROWING_COST = 0.0; // 0.05;

const double DEFAULT_FLOW_RATE = 0.1;
const double FLOW_RATE_CHANGE_STEP = 0.1;
const double TUBE_LENGTH = 30.0;


const vector<pair<int, int>> GROW_NET_DIMS = {
    {6, 8}, // 6 x 8
    {8, 8}, // 8 x 8
    {8, 3}  // 8 x 3
    // + 8 + 8 + 3 
    // total: 155
};

const vector<pair<int, int>> FLOW_NET_DIMS = {
    {3, 8}, // 3 x 8
    {8, 8}, // 8 x 8
    {8, 2}  // 8 x 2
    // + 8 + 8 + 2
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
        // Lambda to generate small random values
        auto smallRand = [](int n) {
            return Random::randvec(n, -0.1, 0.1);
        };

        // Initialize weights with random values
        for (const auto& layer_dim : GROW_NET_DIMS) {
            int in = layer_dim.first;
            int out = layer_dim.second;
            // weight matrix (in x out)
            growNetWeights.push_back(smallRand(in * out));
            // bias vector (1 x out)
            growNetWeights.push_back(smallRand(out));
        }

        for (const auto& layer_dim : FLOW_NET_DIMS) {
            int in = layer_dim.first;
            int out = layer_dim.second;
            // weight matrix (in x out)
            flowNetWeights.push_back(smallRand(in * out));
            // bias vector (1 x out)
            flowNetWeights.push_back(smallRand(out));
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

    void mutate(double mutation_rate, double mutation_strength) {
        for (auto& layer_weights : growNetWeights) {
            for (auto& weight : layer_weights) {
                if (Random::uniform() < mutation_rate) {
                    double change = Random::uniform(-mutation_strength, mutation_strength);
                    weight += change;
                }
            }
        }
        for (auto& layer_weights : flowNetWeights) {
            for (auto& weight : layer_weights) {
                if (Random::uniform() < mutation_rate) {
                    double change = Random::uniform(-mutation_strength, mutation_strength);
                    weight += change;
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

    GrowthDecisionNet(const Genome& genome) {

        Layer* layer1 = new FullyConnected<Sigmoid>(GROW_NET_DIMS[0].first, GROW_NET_DIMS[0].second);
        Layer* layer2 = new FullyConnected<Sigmoid>(GROW_NET_DIMS[1].first, GROW_NET_DIMS[1].second);
        Layer* layer3 = new FullyConnected<Sigmoid>(GROW_NET_DIMS[2].first, GROW_NET_DIMS[2].second);

        net.add_layer(layer1);
        net.add_layer(layer2);
        net.add_layer(layer3);

        net.init();
        
        net.set_parameters(genome.getGrowNetValues());
    }
    
    void decideAction(int numberOfInTubes,
                    int numberOfOutTubes,
                    double averageInTubeAngle,
                    double averageOutTubeAngle,
                    double energy,
                    bool touchingFoodSource) {

        input = Matrix(6, 1);
        input << static_cast<double>(numberOfInTubes),
        static_cast<double>(numberOfOutTubes),
        static_cast<double>(averageInTubeAngle),
        static_cast<double>(averageOutTubeAngle),
        static_cast<double>(energy),
        static_cast<double>(touchingFoodSource);

        
        Vector pred = net.predict(input);
        growthProbability = static_cast<double>(pred(0));
        growthAngle = static_cast<double>(pred(1) * 2.0 * M_PI);
        angleVariance = static_cast<double>(pred(2)) * M_PI;
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

    FlowDecisionNet(const Genome& genome) {

        Layer* layer1 = new FullyConnected<Sigmoid>(FLOW_NET_DIMS[0].first, FLOW_NET_DIMS[0].second);
        Layer* layer2 = new FullyConnected<Sigmoid>(FLOW_NET_DIMS[1].first, FLOW_NET_DIMS[1].second);
        Layer* layer3 = new FullyConnected<Sigmoid>(FLOW_NET_DIMS[2].first, FLOW_NET_DIMS[2].second);

        net.add_layer(layer1);
        net.add_layer(layer2);
        net.add_layer(layer3);

        net.init();
        net.set_parameters(genome.getFlowNetValues());   
    }

    void decideAction(double currentFlowRate,
                    double inJunctionAverageFlowRate,
                    double outJunctionAverageFlowRate) {

        input = Matrix(3, 1);
        input << currentFlowRate,
                inJunctionAverageFlowRate,
                outJunctionAverageFlowRate;
        Vector pred = net.predict(input);
        increaseFlowProb = static_cast<double>(pred(0));
        decreaseFlowProb = static_cast<double>(pred(1));
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

    void mutateGenome(double mutation_rate, double mutation_strength) {
        genome.mutate(mutation_rate, mutation_strength);
    }

    bool touchingFoodSource(const Junction& junc) {
        for (const auto& fs : foodSources) {
            double dist = sqrt((junc.x - fs->x) * (junc.x - fs->x) + (junc.y - fs->y) * (junc.y - fs->y));
            if (dist <= fs->radius) {
                return true;
            }
        }
        return false;
    }

    FoodSource* getFoodSourceAt(const Junction& junc) {
        for (const auto& fs : foodSources) {
            double dist = sqrt((junc.x - fs->x) * (junc.x - fs->x) + (junc.y - fs->y) * (junc.y - fs->y));
            if (dist <= fs->radius) {
                return fs.get();
            }
        }
        return nullptr;
    }

    void growTubeFrom(Junction& from, double angle) {

        double newX = from.x + TUBE_LENGTH * cos(angle);
        double newY = from.y + TUBE_LENGTH * sin(angle);

        auto collisionInfo = getCollisionInfo(from, newX, newY);

        // if no collision, create new junction and tube
        if (collisionInfo.tube == nullptr) {

            auto newJunction = std::make_unique<Junction>(Junction{newX, newY, DEFAULT_JUNCTION_ENERGY});
            Junction* newJuncPtr = newJunction.get();

            auto newTube = std::make_unique<Tube>(Tube{
                from.x, from.y, newX, newY, DEFAULT_FLOW_RATE, &from, newJuncPtr
            });

            // connect tubes to junctions
            from.outTubes.push_back({ newTube.get(), angle });
            newJuncPtr->inTubes.push_back({ newTube.get(), angle });

            if (touchingFoodSource(*newJunction)) {
                newJuncPtr->foodSource = getFoodSourceAt(*newJunction);
            }

            // add to world
            tubes.push_back(std::move(newTube));
            junctions.push_back(std::move(newJunction));

        // if collision, create intersection junction and split existing tube
        } else {
            
            newX = *collisionInfo.x;
            newY = *collisionInfo.y;

            auto newJunction = std::make_unique<Junction>(Junction{newX, newY, DEFAULT_JUNCTION_ENERGY});
            Junction* newJuncPtr = newJunction.get();

            auto newTube = std::make_unique<Tube>(Tube{
                from.x, from.y, newX, newY, DEFAULT_FLOW_RATE, &from, newJuncPtr
            });
            
            // split the existing tube at the intersection point and connect both pieces to the new junction
            Tube* existing = collisionInfo.tube;
            Junction* origFrom = existing->fromJunction;
            Junction* origTo = existing->toJunction;
            double existingFlow = existing->flowRate;

            // create two new tube segments: segment A = origFrom -> newJunc, segment B = newJunc -> origTo
            auto segA = std::make_unique<Tube>(Tube{
                existing->x1, existing->y1,   // keep original start coords
                newX, newY,
                existingFlow,
                origFrom,
                newJuncPtr
            });

            auto segB = std::make_unique<Tube>(Tube{
                newX, newY,
                existing->x2, existing->y2,   // keep original end coords
                existingFlow,
                newJuncPtr,
                origTo
            });

            
            // Update junction lists: replace references to existing tube with the new segments where appropriate.
            
            auto replaceAll = [&](Junction* j, Tube* oldT, Tube* newT) {
                for (auto& ti : j->inTubes)
                    if (ti.tube == oldT) ti.tube = newT;
                for (auto& ti : j->outTubes)
                    if (ti.tube == oldT) ti.tube = newT;
            };

            // replace in origFrom and origTo junctions
            replaceAll(origFrom, existing, segA.get());
            replaceAll(origTo, existing, segB.get());


            // remove the original existing tube from the world's tubes vector
            tubes.erase(std::remove_if(tubes.begin(), tubes.end(), [&](const std::unique_ptr<Tube>& t) { return t.get() == existing; }),
            tubes.end());
            
            // add updated objects to world
            tubes.push_back(std::move(newTube));
            tubes.push_back(std::move(segA));
            tubes.push_back(std::move(segB));
            junctions.push_back(std::move(newJunction));
        }



        from.energy -= DEFAULT_JUNCTION_ENERGY; // energy passed to new junction
        from.energy -= GROWING_COST; // cost of growing
    }

    // Axis-aligned bounding box overlap check
    bool bboxOverlap(double x1a, double y1a, double x2a, double y2a,
                     double x1b, double y1b, double x2b, double y2b) {
        double minAx = std::min(x1a, x2a), maxAx = std::max(x1a, x2a);
        double minAy = std::min(y1a, y2a), maxAy = std::max(y1a, y2a);
        double minBx = std::min(x1b, x2b), maxBx = std::max(x1b, x2b);
        double minBy = std::min(y1b, y2b), maxBy = std::max(y1b, y2b);

        return !(maxAx < minBx || maxBx < minAx || maxAy < minBy || maxBy < minAy);
    }

    optional<pair<double, double>> getSegmentIntersection(
        double x1, double y1, double x2, double y2,
        double x3, double y3, double x4, double y4) {
        double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        if (std::fabs(denom) < 1e-9) return std::nullopt; // parallel or coincident

        double px = ((x1 * y2 - y1 * x2) * (x3 - x4) - 
                    (x1 - x2) * (x3 * y4 - y3 * x4)) / denom;
        double py = ((x1 * y2 - y1 * x2) * (y3 - y4) - 
                    (y1 - y2) * (x3 * y4 - y3 * x4)) / denom;

        auto within = [](double a, double b, double c) {
            return c >= std::min(a, b) - 1e-9 && c <= std::max(a, b) + 1e-9;
        };

        if (within(x1, x2, px) && within(y1, y2, py) &&
            within(x3, x4, px) && within(y3, y4, py)) {
            return std::make_pair(px, py);
        }

        return std::nullopt;
    }


    struct CollisionInfo {
        std::optional<double> x;
        std::optional<double> y;
        Tube* tube = nullptr;
    };

    CollisionInfo getCollisionInfo(Junction& fromJunc, double& newX, double& newY) {

        for (const auto& tube : tubes) {
            if ((tube->fromJunction == &fromJunc) || (tube->toJunction == &fromJunc)) continue;
            if (!bboxOverlap(fromJunc.x, fromJunc.y, newX, newY, tube->x1, tube->y1, tube->x2, tube->y2)) continue;

            // calculate intersection
            if (auto intersection = getSegmentIntersection(fromJunc.x, fromJunc.y, newX, newY, tube->x1, tube->y1, tube->x2, tube->y2)) {
                return CollisionInfo{intersection->first, intersection->second, tube.get()};
            }
        }
        return {std::nullopt, std::nullopt, nullptr};
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

        if (save) {
            this->saveFrame(0);
        }
        for (int step = 1; step <= steps; ++step) {
            this->step();
            if (save) {
                this->saveFrame(step);
            }
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
        // calculateFitness();
    }

    void removeDeadJunctionsAndTubes() {
        // 1. Collect pointers to dead junctions
        std::vector<Junction*> dead;
        for (auto& j : junctions)
        if (j->energy <= 0) {
            dead.push_back(j.get());
        }
        
        // 2. Remove tubes connected to dead junctions
        tubes.erase(
            std::remove_if(tubes.begin(), tubes.end(),
                [&](const std::unique_ptr<Tube>& t) {
                    return std::find(dead.begin(), dead.end(), t->fromJunction) != dead.end() ||
                        std::find(dead.begin(), dead.end(), t->toJunction) != dead.end();
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

    void updateJunctions() {

        removeDeadJunctionsAndTubes();

        // Remove any dangling TubeInfo references from junctions (tubes that are no longer present)
        auto tubeExists = [&](Tube* t) {
            if (!t) return false;
            for (const auto& ut : tubes) {
                if (ut.get() == t) return true;
            }
            return false;
        };

        for (auto& j : junctions) {
            auto& in = j->inTubes;
            in.erase(std::remove_if(in.begin(), in.end(),
                        [&](const Junction::TubeInfo& ti) { return !tubeExists(ti.tube); }),
                     in.end());
            auto& out = j->outTubes;
            out.erase(std::remove_if(out.begin(), out.end(),
                        [&](const Junction::TubeInfo& ti) { return !tubeExists(ti.tube); }),
                      out.end());
        }

        size_t existingCount = junctions.size();

        for (size_t i = 0; i < existingCount; ++i) {

            // be defensive: the junction pointer might have been removed by previous operations
            if (i >= junctions.size()) break;
            Junction* junc = junctions[i].get();
            if (!junc) continue;

            // handle energy movement

            // incoming tubes
            for (auto& inTubeInfo : junc->inTubes) {
                Tube* tube = inTubeInfo.tube;
                if (!tube || !tube->fromJunction) continue; // guard against dangling pointers
                double flow = tube->flowRate;
                if (flow > tube->fromJunction->energy) {
                    flow = tube->fromJunction->energy; // limit flow by available energy
                }
                junc->energy += flow;
                tube->fromJunction->energy -= flow;
            }
            // outgoing tubes
            for (auto& outTubeInfo : junc->outTubes) {
                Tube* tube = outTubeInfo.tube;
                if (!tube || !tube->toJunction) continue; // guard against dangling pointers
                double flow = tube->flowRate;
                if (flow > junc->energy) {
                    flow = junc->energy; // limit flow by available energy
                }
                junc->energy -= flow;
                tube->toJunction->energy += flow;
            }

            if (junc->energy < 1e-12) continue; // depleted junctions can't grow

            // handle growth decision

            int numInTubes = junc->numInTubes();
            int numOutTubes = junc->numOutTubes();
            double averageAngleIn = junc->averageAngleInTubes();
            double averageAngleOut = junc->averageAngleOutTubes();
            double energy = junc->energy;
            bool touchingFoodSource = junc->isTouchingFoodSource();

            growthDecisionNet.decideAction(numInTubes,
                                            numOutTubes,
                                            averageAngleIn,
                                            averageAngleOut,
                                            energy,
                                            touchingFoodSource);

            if (Random::uniform() < growthDecisionNet.growthProbability && energy > MIN_GROWTH_ENERGY) {
                growTubeFrom(
                    *junc,
                    growthDecisionNet.growthAngle + Random::uniform(-growthDecisionNet.angleVariance, growthDecisionNet.angleVariance));
            }
        }
    }
    
    void updateTubes() {

        for (auto& tube : tubes) {

            // let flow rate decision net decide on flow rate changes
            double currFlowRate = tube->flowRate;
            double inJunctionAverageFlowRate = static_cast<double>(tube->fromJunction->getSummedFlowRate());
            double outJunctionAverageFlowRate = static_cast<double>(tube->toJunction->getSummedFlowRate());

            flowDecisionNet.decideAction(currFlowRate,
                                        inJunctionAverageFlowRate,
                                        outJunctionAverageFlowRate);

            // adjust flow rate based on decision net
            if (Random::uniform() < flowDecisionNet.increaseFlowProb) {
                tube->flowRate += FLOW_RATE_CHANGE_STEP;
            }
            if (Random::uniform() < flowDecisionNet.decreaseFlowProb && tube->flowRate > 0) {
                tube->flowRate -= FLOW_RATE_CHANGE_STEP;
            }

            // clamp flow rate to zero to prevent noise
            if (tube->flowRate < 1e-12) tube->flowRate = 0.0;

            // rearrange tube direction if flow rate changes to negative
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

    void calculateFitness() {
        
        double totalEnergy = 0.0;
        for (const auto& junc : junctions) {
            totalEnergy += junc->energy;
        }
        fitness = totalEnergy;
    }
};