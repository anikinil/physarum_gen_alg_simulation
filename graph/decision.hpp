#include <vector>
#include "genome.hpp"
#include "FNN.hpp"

using namespace std;

struct GrowthDecisionNet {

    Genome genome;
    FNN net;
    
    double growthProbability = 0.0;
    double growthAngle = 0.0;
    double angleVariance = 0.0;
    int signal = 0;

    GrowthDecisionNet(const Genome& genome) {

        auto net = FNN();
        net.initialize(genome.getGrowNetWeights());
        this->net = net;
    }
    
    void decideAction(int numberOfInTubes,
                    int numberOfOutTubes,
                    double averageInFlowRate,
                    double averageOutFlowRate,
                    double averageInTubeAngle,
                    double averageOutTubeAngle,
                    double energy,
                    bool touchingFoodSource,
                    const deque<int>& signalHistory) {

        vector<double> input = {
            static_cast<double>(numberOfInTubes),
            static_cast<double>(numberOfOutTubes),
            averageInTubeAngle,
            averageOutTubeAngle,
            energy,
            static_cast<double>(touchingFoodSource)
        };

        
        for (int signal : signalHistory) {
            double signal_value = static_cast<double>(signal) / NUM_SIGNAL_TYPES;
            input.push_back(signal_value);
        }
        
        vector<double> pred = net.predict(input);
        // cout << "GrowthDecisionNet prediction: " << pred[0] << ", " << pred[1] << ", " << pred[2] << pred[3] << endl;
        growthProbability = pred[0];
        growthAngle = pred[1] * 2.0 * M_PI;
        angleVariance = pred[2] * M_PI;
        signal = static_cast<int>(pred[3] * NUM_SIGNAL_TYPES);
    }
};

struct FlowDecisionNet {

    Genome genome;
    FNN net;

    double increaseFlowProb = 0.0;
    double decreaseFlowProb = 0.0;

    FlowDecisionNet(const Genome& genome) {

        auto net = FNN();
        net.initialize(genome.getFlowNetWeights());
        this->net = net;
    }

    void decideAction(double currentFlowRate,
                    double inJunctionAverageFlowRate,
                    double outJunctionAverageFlowRate,
                    int signal) {

        vector<double> input = {
            currentFlowRate,
            inJunctionAverageFlowRate,
            outJunctionAverageFlowRate,
            static_cast<double>(signal) / SIGNAL_TYPES.size()
        };

        vector<double> pred = net.predict(input);
        // cout << "FlowDecisionNet prediction: " << pred[0] << ", " << pred[1] << endl;
        increaseFlowProb = pred[0];
        decreaseFlowProb = pred[1];
    }
};
