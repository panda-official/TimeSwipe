#pragma once
#include <vector>
#include <map>
#include <memory>
#include "timeswipe.hpp"

struct ResamplerState {
    std::vector<float> h;
    int delay;
    int outputSize;
    ResamplerState(int upFactor, int downFactor, size_t inputSize);
};

class TimeSwipeResampler {
    std::vector<float> buffers[4];
    int upFactor;
    int downFactor;
    unsigned pad;
    // Coefficients for each input size
    std::map<unsigned,std::unique_ptr<ResamplerState>> states;
public:
    TimeSwipeResampler(int up, int down);
    std::vector<Record> Resample(std::vector<Record>&& records);
};
