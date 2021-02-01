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
    SensorsData buffer;
    int upFactor;
    int downFactor;
    unsigned pad;
    size_t sliceSize;
    size_t sliceSizePad;
    std::unique_ptr<ResamplerState> state;
public:
    TimeSwipeResampler(int up, int down);
    SensorsData Resample(SensorsData&& records);
};
