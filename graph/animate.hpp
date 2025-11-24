#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

#include "gen_alg.hpp"

const int WIN_WIDTH = 2000;
const int WIN_HEIGHT = 1500;

const double JUNCTION_RADIUS_FACTOR = 3.0;

const float MIN_TUBE_THICKNESS = 0.5f;
const int TUBE_THICKNESS_FACTOR = 1;

const float FPS = 60.0f;
const float DEFAULT_ZOOM = 0.4f;


const int OBJECT_TRANSPARENCY = 80;

const sf::Color BACKGROUND_COLOR(28, 43, 52);
const sf::Color JUNCTION_COLOR(255, 200, 40);
const sf::Color DEPLETED_JUNCTION_COLOR(180, 140, 20);
const sf::Color TUBE_COLOR(255, 200, 40);
// const sf::Color FOOD_SOURCE_COLOR(153, 191, 63, OBJECT_TRANSPARENCY);
const sf::Color FOOD_SOURCE_COLOR(203, 71, 68, OBJECT_TRANSPARENCY);
const sf::Color FOOD_SOURCE_CAPACITY_COLOR(255, 255, 255, OBJECT_TRANSPARENCY);


struct JunctionVisual {
    double x;
    double y;
    double energy;
    string touchingFoodSource;
    int signal;
    vector<int> signalHistory;
};

struct TubeVisual {
    double x1;
    double y1;
    double x2;
    double y2;
    double flowRate;
};

struct FoodSourceVisual {
    double x;
    double y;
    double radius;
    double energy;
};

struct Frame {
    vector<JunctionVisual> junctions;
    vector<TubeVisual> tubes;
    vector<FoodSourceVisual> foodSources;

    double fitness = 0.0;

    void addObject(const string& line) {
        stringstream ss(line);
        string token;
        vector<string> fields;

        while (getline(ss, token, ',')) {
            fields.push_back(token);
        }

        fitness = stod(fields[1]);

        if (!fields[2].empty()) {
            vector<int> signalHistory;
            for (size_t i = 0; i < MAX_SIGNAL_HISTORY_LENGTH; ++i) {
                if (!fields[7 + i].empty()) {
                    signalHistory.push_back(stoi(fields[7 + i]));
                } else {
                    signalHistory.push_back(0); // or some default value
                }
            }
            JunctionVisual j{stod(fields[2]), stod(fields[3]), stod(fields[4]), (fields[5] == "1" ? "yes" : "no"), stoi(fields[6]), signalHistory};
            junctions.push_back(j);
        } else if (!fields[7+MAX_SIGNAL_HISTORY_LENGTH].empty()) {
            TubeVisual t{
                stod(fields[7+MAX_SIGNAL_HISTORY_LENGTH]),
                stod(fields[8+MAX_SIGNAL_HISTORY_LENGTH]),
                stod(fields[9+MAX_SIGNAL_HISTORY_LENGTH]),
                stod(fields[10+MAX_SIGNAL_HISTORY_LENGTH]),
                stod(fields[11+MAX_SIGNAL_HISTORY_LENGTH])};
            tubes.push_back(t);
        } else if (!fields[12+MAX_SIGNAL_HISTORY_LENGTH].empty()) {
            FoodSourceVisual f{
                stod(fields[12+MAX_SIGNAL_HISTORY_LENGTH]),
                stod(fields[13+MAX_SIGNAL_HISTORY_LENGTH]),
                stod(fields[14+MAX_SIGNAL_HISTORY_LENGTH]),
                stod(fields[15+MAX_SIGNAL_HISTORY_LENGTH])};
            foodSources.push_back(f);
        }
    }
};