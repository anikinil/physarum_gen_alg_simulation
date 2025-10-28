#pragma once
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

const int WIN_WIDTH = 2000;
const int WIN_HEIGHT = 1500;

const int JUNCTION_RADIUS = 5;
const int TUBE_THICKNESS = 3;

const float FPS = 5.0f;

struct JunctionVisual {
    double x;
    double y;
    int energy;
};

struct TubeVisual {
    double x1;
    double y1;
    double x2;
    double y2;
    int flowRate;
};

struct FoodSourceVisual {
    double x;
    double y;
    double radius;
    int energy;
};

struct Frame {
    vector<JunctionVisual> junctions;
    vector<TubeVisual> tubes;
    vector<FoodSourceVisual> foodSources;

    void addObject(const string& line) {
        stringstream ss(line);
        string token;
        vector<string> fields;

        while (getline(ss, token, ',')) fields.push_back(token);

        if (!fields[1].empty()) {
            JunctionVisual j{stod(fields[1]), stod(fields[2]), stoi(fields[3])};
            junctions.push_back(j);
        } else if (!fields[4].empty()) {
            TubeVisual t{stod(fields[4]), stod(fields[5]), stod(fields[6]), stod(fields[7]), stoi(fields[8])};
            tubes.push_back(t);
        } else if (!fields[9].empty()) {
            FoodSourceVisual f{stod(fields[9]), stod(fields[10]), stod(fields[11]), stoi(fields[12])};
            foodSources.push_back(f);
        }
    }
};