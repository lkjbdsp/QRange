#pragma once

#include "filterstagedsp.h"
#include "../resource.h"
#include "../QRangeTypes.h"

#include "../JuceIncludes.h"

class FilterBandDsp
{
public:

    struct ResponseData
    {
        float* left = nullptr;
        float* right = nullptr;
        float* mid = nullptr;
        float* side = nullptr;
        bool mono = false;
    };

    FilterBandDsp();

    void clear();
    void setFilter(bool newEnabled, double newFreq, double newQ, double newGain, EFilterTypes newType, ERoutings newRouting, ECutOrders newOrder, double newSampleRate);

    void processBlock(double* chL, int numSamples);
    void processBlock(double* chL, double* chR, int numSamples);

    void getResponse(ResponseData& data, const float* normalizedFreqs, int numPoints);

private:

    void processBlockMidOrSide(double* chL, double* chR, int numSamples, bool processMid);
    FilterStage::MZTiFilterType getMztiType(EFilterTypes t, ECutOrders o) const;

    bool enabled = true;
    double freq = 0;
    double qFactor = 0;
    double gain = 0;
    EFilterTypes type = kTypePeak;
    ERoutings routing = kRoutingStereo;
    ECutOrders order = kCutOrder12dB;
    double sampleRate = 0;

    FilterStage::FilterInstance filter;
    FilterStage::FilterInstance filterCutAux;

    std::vector<float> responseData;

};