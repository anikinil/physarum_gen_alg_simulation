#pragma once
#include <random>
#include <vector>
#include <iostream>

using namespace std;

class Random {
public:
    static double uniform(double min = 0.0, double max = 1.0) {
        static thread_local mt19937 gen(random_device{}());
        uniform_real_distribution<double> dist(min, max);
        return dist(gen);
    }

    static int randint(int min, int max) {
        static thread_local mt19937 gen(random_device{}());
        uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }

    static vector<double> randvec(int size, double min = 0.0, double max = 1.0) {
        vector<double> vec(size);
        for (int i = 0; i < size; ++i) {
            vec[i] = uniform(min, max);
        }
        return vec;
    }
};


class Printer {
public:
    template<typename T>
    static void print(const T& msg) {
        cout << msg << std::endl;
    }
};