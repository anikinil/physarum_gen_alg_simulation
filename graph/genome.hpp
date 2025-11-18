#include <vector>
#include "utils.hpp"

const int MAX_SIGNAL_HISTORY_LENGTH = 4;
const vector<int> SIGNAL_TYPES = {0, 1, 2, 3};


const vector<pair<int, int>> GROW_NET_DIMS = {
    {MAX_SIGNAL_HISTORY_LENGTH + 8, 16},
    {16, 12},
    {12, 8},
    {8, 8},
    {8, 4}
};

const vector<pair<int, int>> FLOW_NET_DIMS = {
    {4, 5}, // 3 x 5
    {5, 6}, // 5 x 6
    {6, 4}, // 6 x 4
    {4, 2}  // 4 x 2
    // + 5 + 6 + 4 + 2
    // total: 94
};
// total genome size: 309

double INITIAL_WEIGHT_RANGE = 3;

using namespace std;

struct Genome {
    vector<vector<vector<double>>> growNetWeights;
    vector<vector<vector<double>>> flowNetWeights;

    // TODO change structure to:
    // std::vector<std::vector<std::vector<double>>> growNetWeights, flowNetWeights;
    // std::vector<std::vector<double>> growNetBiases, flowNetBiases;

    Genome() {
        auto addLayer = [&](const std::pair<int,int>& d, 
                            std::vector<std::vector<std::vector<double>>>& net) {
            int in = d.first, out = d.second;
            double s = std::sqrt(2.0 / in);
            std::vector<std::vector<double>> W(in, std::vector<double>(out));
            for (int i = 0; i < in; ++i)
                for (int j = 0; j < out; ++j)
                    W[i][j] = Random::gaussian(0.0, s);
            std::vector<std::vector<double>> bias_row(1, std::vector<double>(out, 0.0));
            net.push_back(std::move(W));
            net.push_back(std::move(bias_row));
        };

        for (const auto& d : GROW_NET_DIMS) addLayer(d, growNetWeights);
        for (const auto& d : FLOW_NET_DIMS) addLayer(d, flowNetWeights);
    }


    vector<vector<vector<double>>> getGrowNetWeights() const {
        return growNetWeights;
    }

    vector<vector<vector<double>>> getFlowNetWeights() const {
        return flowNetWeights;
    }

    void setGrowNetWights(vector<double> weights) {
        int index = 0;
        for (const auto& layer_dim : GROW_NET_DIMS) {
            int in = layer_dim.first;
            int out = layer_dim.second;
            for (int i = 0; i < in; ++i) {
                for (int j = 0; j < out; ++j) {
                    growNetWeights[index][i][j] = weights.front();
                    weights.erase(weights.begin());
                }
            }
            index++;
            for (int j = 0; j < out; ++j) {
                growNetWeights[index][0][j] = weights.front();
                weights.erase(weights.begin());
            }
            index++;
        }
    }

    void setFlowNetWights(vector<double> weights) {
        int index = 0;
        for (const auto& layer_dim : FLOW_NET_DIMS) {
            int in = layer_dim.first;
            int out = layer_dim.second;
            for (int i = 0; i < in; ++i) {
                for (int j = 0; j < out; ++j) {
                    flowNetWeights[index][i][j] = weights.front();
                    weights.erase(weights.begin());
                }
            }
            index++;
            for (int j = 0; j < out; ++j) {
                flowNetWeights[index][0][j] = weights.front();
                weights.erase(weights.begin());
            }
            index++;
        }
    }

    void mutate(double mutation_rate, double mutation_strength) {
        for (auto& weight_matrix : growNetWeights) {
            for (auto& weight_vector : weight_matrix) {
                for (auto& weight : weight_vector) {
                    if (Random::uniform(0.0, 1.0) < mutation_rate) {
                        weight += Random::uniform(-mutation_strength, mutation_strength);
                    }
                }
            }
        }
        for (auto& weight_matrix : flowNetWeights) {
            for (auto& weight_vector : weight_matrix) {
                for (auto& weight : weight_vector) {
                    if (Random::uniform(0.0, 1.0) < mutation_rate) {
                        weight += Random::uniform(-mutation_strength, mutation_strength);
                    }
                }
            }
        }
    }

    vector<double> serialize() const {
        vector<double> genome_vector;
        for (const auto& layer_weights : growNetWeights) {
            for (const auto& weight_vector : layer_weights) {
                for (const auto& weight : weight_vector) {
                    genome_vector.push_back(weight);
                }
            }
        }
        for (const auto& layer_weights : flowNetWeights) {
            for (const auto& weight_vector : layer_weights) {
                for (const auto& weight : weight_vector) {
                    genome_vector.push_back(weight);
                }
            }
        }
        return genome_vector;
    }
};

