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

template <int k=0 >
class Records {
public:
    std::vector<Record> recs;

    float operator[](int i) const {
        return recs[i].Sensors[k];
    }
};

class TimeSwipeResampler {
    //std::vector<float> buffers[4];
    Records<0> buffer;
    int upFactor;
    int downFactor;
    unsigned pad;
    size_t sliceSize;
    size_t sliceSizePad;
    // Coefficients for each input size
    //std::map<unsigned,std::unique_ptr<ResamplerState>> states;
    std::unique_ptr<ResamplerState> state;
public:
    TimeSwipeResampler(int up, int down);
    std::vector<Record> Resample(std::vector<Record>&& records);
};
