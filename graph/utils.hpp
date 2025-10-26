#pragma once
#include <random>

class Random {
public:
    static double uniform(double min = 0.0, double max = 1.0) {
        static thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<double> dist(min, max);
        return dist(gen);
    }

    static int randint(int min, int max) {
        static thread_local std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<int> dist(min, max);
        return dist(gen);
    }

    static vector<float> randvec(int size, float min = 0.0f, float max = 1.0f) {
        vector<float> vec(size);
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
        std::cout << msg << std::endl;
    }
};