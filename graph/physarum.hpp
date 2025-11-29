#include <vector>
#include <deque>
#include <memory>
#include <numeric>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <optional>
#include <utility>
#include <cmath>

using namespace std;

#include "decision.hpp"


const double GROWTH_COST = 1.5;
const double DEFAULT_JUNCTION_ENERGY = 1.0;
const double MIN_GROWTH_ENERGY = (DEFAULT_JUNCTION_ENERGY + GROWTH_COST);
const double PASSIVE_ENERGY_LOSS = 0.01;

const int MAX_TUBES_PER_JUNCTION = 4;
const double DEFAULT_FLOW_RATE = 0.4;
const double FLOW_RATE_CHANGE_STEP = 0.05;
const double TUBE_LENGTH = 20.0;

const double MAX_JUNCTION_ENERGY = 20.0;
const double MAX_TUBE_FLOW_RATE = 2.0;

const double FOOD_ENERGY_ABSORB_RATE = 0.1;

const double MIN_GROWTH_ANGLE_VARIANCE = 0.3;

struct Junction;
struct Tube;
struct FoodSource;


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

    int signal = 0;
    deque<int> signalHistory;

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

    double averageInFlowRate() {
        if (inTubes.empty()) return 0.0;
        double sum = accumulate(inTubes.begin(), inTubes.end(), 0.0,
            [](double acc, const TubeInfo& ti) { return acc + ti.tube->flowRate; });
        return sum / inTubes.size();
    }

    double averageOutFlowRate() {
        if (outTubes.empty()) return 0.0;
        double sum = accumulate(outTubes.begin(), outTubes.end(), 0.0,
            [](double acc, const TubeInfo& ti) { return acc + ti.tube->flowRate; });
        return sum / outTubes.size();
    }

    double averageAngleInTubes() {
        if (inTubes.empty()) return Random::uniform(0.0, 2.0 * M_PI);
        double sum = accumulate(inTubes.begin(), inTubes.end(), 0.0,
            [](double acc, const TubeInfo& ti) { return acc + ti.angle; });
        return sum / inTubes.size();
    }

    double averageAngleOutTubes() {
        if (outTubes.empty()) return Random::uniform(0.0, 2.0 * M_PI);
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

    int getTotalTubes() {
        return inTubes.size() + outTubes.size();
    }

    void saveSignal(int signal) {
        signalHistory.push_back(signal);
        if (signalHistory.size() > MAX_SIGNAL_HISTORY_LENGTH) {
            signalHistory.pop_front();  // use deque instead of vector
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

struct World {
    Genome genome;

    vector<unique_ptr<Junction>> junctions;
    vector<unique_ptr<Tube>> tubes;
    vector<unique_ptr<FoodSource>> foodSources;

    GrowthDecisionNet growthDecisionNet;
    FlowDecisionNet flowDecisionNet;

    double food_consumed = 0.0;
    double fitness = 0.0;
    
    World(const Genome& g)
        : genome(g),
        growthDecisionNet(g),
        flowDecisionNet(g) {}

    Genome getGenome() const {
        return genome;
    }

    void mutateGenome(double mutation_rate, double mutation_strength) {
        genome.mutate(mutation_rate, mutation_strength);
    }

    FoodSource* touchingFoodSource(const Junction& junc) {
        for (const auto& fs : foodSources) {
            double dist = sqrt((junc.x - fs->x) * (junc.x - fs->x) + (junc.y - fs->y) * (junc.y - fs->y));
            if (dist <= fs->radius) {
                return fs.get();
            }
        }
        return nullptr;
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

    void placeNewFoodSources(vector<unique_ptr<FoodSource>>&& newFoodSources) {
        for (auto& fs : newFoodSources) {
            foodSources.push_back(std::move(fs));
        }
    }

    void placeNewJunctions(vector<unique_ptr<Junction>>&& newJunctions) {
        for (auto& junc : newJunctions) {
            junctions.push_back(std::move(junc));
            junctions.back()->foodSource = getFoodSourceAt(*junctions.back());
        }
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

            newJuncPtr->foodSource = touchingFoodSource(*newJunction);
            // add to world
            tubes.push_back(std::move(newTube));
            junctions.push_back(std::move(newJunction));

        // if collision, create intersection junction and split existing tube
        } else {
            
            newX = *collisionInfo.x;
            newY = *collisionInfo.y;

            auto newJunction = std::make_unique<Junction>(Junction{newX, newY, DEFAULT_JUNCTION_ENERGY});
            Junction* newJuncPtr = newJunction.get();

            newJuncPtr->foodSource = touchingFoodSource(*newJunction);

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
        from.energy -= GROWTH_COST; // cost of growing

        // prevent noise
        if (from.energy < 1e-6) from.energy = 0.0;    
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
                 << "fitness,"
                 << "j_x,j_y,j_energy,j_touching_food,j_signal,";
                for (size_t i = 0; i < MAX_SIGNAL_HISTORY_LENGTH; ++i) {
                    file << "j_signal_hist_" << i << ",";
                }
                file
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
            if (junc->energy < 1e-6) junc->energy = 0.0;
            file << step << ','
                 << fitness << ','
                 << junc->x << ','
                 << junc->y << ','
                 << junc->energy << ','
                 << (junc->isTouchingFoodSource() ? 1 : 0) << ','
                 << junc->signal << ',';
                 for (size_t j = 0; j < MAX_SIGNAL_HISTORY_LENGTH; ++j)
                     if (j < junc->signalHistory.size())
                         file << junc->signalHistory[j] << ',';
                 file << ",,,,,"
                 << ",,,\n";
        }
        for (const auto& tube : tubes) {
            // if (tube->flowRate < 1e-6) tube->flowRate = 0.0;
            file << step << ','
                 << fitness << ",,,,,,";
                 for (size_t j = 0; j < MAX_SIGNAL_HISTORY_LENGTH; ++j)
                     file << ",";
                 file << tube->x1 << ',' << tube->y1 << ','
                 << tube->x2 << ',' << tube->y2 << ','
                 << tube->flowRate << ','
                 << ",,,\n";
        }
        for (const auto& fs : foodSources) {
            file << step << ','
                 << fitness << ",,,,,,";
                 for (size_t j = 0; j < MAX_SIGNAL_HISTORY_LENGTH; ++j)
                     file << ",";
                 file << ",,,,,"
                 << fs->x << ',' << fs->y << ',' << fs->radius << ','
                 << fs->energy << "\n";
        }
    }

    void step() {
        updateJunctions();
        updateTubes();
        updateFood();
        updateFitness();
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

        // remove any dangling TubeInfo references from junctions
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
        // cout << "Existing count" << existingCount << endl;

        for (size_t i = 0; i < existingCount; ++i) {

            if (i >= junctions.size()) break;
            Junction* junc = junctions[i].get();
            if (!junc) continue;

            // handle energy movement

            // incoming tubes
            // for (auto& inTubeInfo : junc->inTubes) {
            //     Tube* tube = inTubeInfo.tube;
            //     if (!tube || !tube->fromJunction) continue; // guard against dangling pointers
            //     double energyAmount = tube->fromJunction->energy * tube->flowRate;
            //     // if (energyAmount > tube->fromJunction->energy) {
            //     //     energyAmount = tube->fromJunction->energy; // limit flow by available energy
            //     // }
            //     tube->fromJunction->energy -= energyAmount;
            //     junc->energy += energyAmount;
            //     junc->saveSignal(tube->fromJunction->signal);
            //     cout << "Junction " << junc->x << ", " << junc->y << " saved signal " << tube->fromJunction->signal << " from incoming tube." << endl;

            //     if (junc->energy > MAX_JUNCTION_ENERGY) {
            //         junc->energy = MAX_JUNCTION_ENERGY; // cap junction energy -> junction looses excess energy 
            //     }
            // }
            // outgoing tubes
            for (auto& outTubeInfo : junc->outTubes) {
                Tube* tube = outTubeInfo.tube;
                if (!tube || !tube->toJunction) continue; // guard against dangling pointers
                double energyAmount = junc->energy * tube->flowRate;
                // if (energyAmount > junc->energy) {
                //     energyAmount = junc->energy; // limit flow by available energy
                // }
                junc->energy -= energyAmount;
                tube->toJunction->energy += energyAmount;
                tube->toJunction->saveSignal(junc->signal);
                // cout << "Junction " << tube->toJunction->x << ", " << tube->toJunction->y << " saved signal " << junc->signal << " from outgoing tube." << endl;

                if (tube->toJunction->energy > MAX_JUNCTION_ENERGY) {
                    tube->toJunction->energy = MAX_JUNCTION_ENERGY; // cap junction energy -> junction looses excess energy 
                }
            }

            if (junc->energy < 1e-6) continue; // depleted junctions can't grow

            // handle growth decision

            int numInTubes = junc->numInTubes();
            int numOutTubes = junc->numOutTubes();
            double averageAngleIn = junc->averageAngleInTubes();
            double averageAngleOut = junc->averageAngleOutTubes();
            double energy = junc->energy  / MAX_JUNCTION_ENERGY; // normalize energy input
            // cout << "Junction energy: " << energy << endl;
            bool touchingFoodSource = junc->isTouchingFoodSource();
            deque<int> signalHistory = junc->signalHistory;
            
            growthDecisionNet.decideAction(numInTubes,
                                            numOutTubes,
                                            junc->averageInFlowRate(),
                                            junc->averageOutFlowRate(),
                                            averageAngleIn,
                                            averageAngleOut,
                                            energy,
                                            touchingFoodSource,
                                            signalHistory);
               
            if (junc->getTotalTubes() < MAX_TUBES_PER_JUNCTION && Random::uniform() < growthDecisionNet.growthProbability && junc->energy > MIN_GROWTH_ENERGY) {
                double variance = max(growthDecisionNet.angleVariance, MIN_GROWTH_ANGLE_VARIANCE);
                double angle = 
                    averageAngleIn + 
                    growthDecisionNet.growthAngle +
                    Random::uniform(-variance, variance);
                
                growTubeFrom(*junc, angle);
            }

            junc->signal = growthDecisionNet.signal;

            // passive energy loss
            junc->energy *= 1 - PASSIVE_ENERGY_LOSS;
            // junc->energy -= PASSIVE_ENERGY_LOSS/junc->energy;
            if (junc->energy < 1e-6) junc->energy = 0.0;
        }        
    }
    
    void updateTubes() {

        for (auto& tube : tubes) {

            // let flow rate decision net decide on flow rate changes
            double currFlowRate = tube->flowRate;
            double inJunctionAverageFlowRate = static_cast<double>(tube->fromJunction->getSummedFlowRate());
            double outJunctionAverageFlowRate = static_cast<double>(tube->toJunction->getSummedFlowRate());
            double signal = static_cast<double>(tube->fromJunction->signal) / SIGNAL_TYPES.size();

            flowDecisionNet.decideAction(currFlowRate,
                                        inJunctionAverageFlowRate,
                                        outJunctionAverageFlowRate,
                                        signal);
            // adjust flow rate based on decision net
            if (Random::uniform() < flowDecisionNet.increaseFlowProb) {
                tube->flowRate += FLOW_RATE_CHANGE_STEP;
                if (tube->flowRate > MAX_TUBE_FLOW_RATE) {
                    tube->flowRate = MAX_TUBE_FLOW_RATE; // cap flow rate
                }
            }
            if (Random::uniform() < flowDecisionNet.decreaseFlowProb && tube->flowRate > 0) {
                tube->flowRate -= FLOW_RATE_CHANGE_STEP;
            }

            // rearrange tube direction if flow rate changes to negative
            if (tube->flowRate < 0) {
                std::swap(tube->fromJunction, tube->toJunction);
                tube->flowRate = -tube->flowRate;
                tube->fromJunction->switchTubeDirection(*tube);
                tube->toJunction->switchTubeDirection(*tube);
            }
            
            if (tube->flowRate < 1e-6) tube->flowRate = 0.0;
        }
    }

    // void deleteFoodSource(FoodSource* fs) {
    //     foodSources.erase(
    //         std::remove_if(foodSources.begin(), foodSources.end(),
    //             [&](const std::unique_ptr<FoodSource>& f) {
    //                 return f.get() == fs;
    //             }),
    //             foodSources.end());
    // }

    void deleteDepleetedFoodSources() {
        // remove food sources with energy <= 0
        foodSources.erase(
            std::remove_if(foodSources.begin(), foodSources.end(),
                [](const std::unique_ptr<FoodSource>& fs) {
                    return fs->energy <= 1e-6;
                }),
                foodSources.end());
    }

    void updateFood() {

        for (auto& foodSource : foodSources) {
            
            for (auto& junc : junctions) {
                

                if (junc->foodSource == foodSource.get()) {
                    if (junc->energy == MAX_JUNCTION_ENERGY)
                        continue;

                    FoodSource* fs = junc->foodSource; // keep the pointer

                    fs->energy -= FOOD_ENERGY_ABSORB_RATE;
                    food_consumed += FOOD_ENERGY_ABSORB_RATE;

                    
                    junc->energy += FOOD_ENERGY_ABSORB_RATE;

                    
                    if (junc-> energy > MAX_JUNCTION_ENERGY) {
                        junc->energy = MAX_JUNCTION_ENERGY;
                    }

                    // only one junction can feed on foodsource -> go to next food source
                    break;
                }
            }
        }

        deleteDepleetedFoodSources();

        // for (auto& junc : junctions) {

        //     if (junc->isTouchingFoodSource()) {
        //         if (junc->energy == MAX_JUNCTION_ENERGY)
        //             continue;

        //         FoodSource* fs = junc->foodSource; // keep the pointer

        //         fs->energy -= FOOD_ENERGY_ABSORB_RATE;
        //         food_consumed += FOOD_ENERGY_ABSORB_RATE;
                
        //         if (fs->energy <= 0) {
        //             // clear all references BEFORE deleting from vector
        //             for (auto& j : junctions) {
        //                 if (j->foodSource == fs)
        //                 j->foodSource = nullptr;
        //             }
        //             deleteFoodSource(fs);
        //         }
                
        //         junc->energy += FOOD_ENERGY_ABSORB_RATE;
                
        //         if (junc-> energy > MAX_JUNCTION_ENERGY) {
        //             junc->energy = MAX_JUNCTION_ENERGY;
        //         }
        //     }
        // }
    }

    void updateFitness() {
        calculateFitness();
    }

    void calculateFitness() {
        
        // total food energy consumed;
        // fitness = food_consumed;


        // total cell energy fitness

        double totalEnergy = 0.0;
        for (const auto& junc : junctions) {
            totalEnergy += junc->energy;
        }
        fitness = totalEnergy;

        // energy centrality-based fitness

        // double centralityScore = 0.0;
        // for (const auto& junc : junctions) {
        //     double distToCenter = sqrt(junc->x * junc->x + junc->y * junc->y);
        //     double centrality = 1.0 / (1.0 + distToCenter);
        //     centralityScore += junc->energy * centrality;
        // }
        // fitness = centralityScore;
    }
};