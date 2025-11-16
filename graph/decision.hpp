#include <vector>
#include "genome.hpp"
#include "FNN.hpp"

using namespace std;


struct GrowthDecisionNet {

    Genome genome;
    FNN net;

    int numberOfInTubes = 0;
    int numberOfOutTubes = 0;
    double averageInTubeAngle = 0.0;
    double averageOutTubeAngle = 0.0;
    double energy = 0.0;
    bool touchingFoodSource = false;
    
    double growthProbability = 0.0;
    double growthAngle = 0.0;
    double angleVariance = 0.0;

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
                    bool touchingFoodSource) {

        vector<double> input = {
            static_cast<double>(numberOfInTubes),
            static_cast<double>(numberOfOutTubes),
            averageInTubeAngle,
            averageOutTubeAngle,
            energy,
            static_cast<double>(touchingFoodSource)
        };

        
        vector<double> pred = net.predict(input);
        // cout << "GrowthDecisionNet prediction: " << pred[0] << ", " << pred[1] << ", " << pred[2] << endl;
        growthProbability = pred[0];
        growthAngle = pred[1] * 2.0 * M_PI;
        angleVariance = pred[2] * M_PI;

    }
};

struct FlowDecisionNet {

    Genome genome;
    FNN net;

    double currentFlowRate = 0.0;
    double inJunctionAverageFlowRate = 0.0;
    double outJunctionAverageFlowRate = 0.0;

    double increaseFlowProb = 0.0;
    double decreaseFlowProb = 0.0;

    FlowDecisionNet(const Genome& genome) {

        auto net = FNN();
        net.initialize(genome.getFlowNetWeights());
        this->net = net;
    }

    void decideAction(double currentFlowRate,
                    double inJunctionAverageFlowRate,
                    double outJunctionAverageFlowRate) {

        vector<double> input = {
            currentFlowRate,
            inJunctionAverageFlowRate,
            outJunctionAverageFlowRate };

        vector<double> pred = net.predict(input);
        // cout << "FlowDecisionNet prediction: " << pred[0] << ", " << pred[1] << endl;
        increaseFlowProb = pred[0];
        decreaseFlowProb = pred[1];
    }
};
