#pragma once

#include <vector>
#include <memory>

#include "lkjbFilter.h"


namespace FilterStage
{

enum MZTiFilterType
{
    filterTypePeak,
    filterTypeLoShelf,
    filterTypeHiShelf,
    filterTypeLoPass6,
    filterTypeLoPass,
    filterTypeHiPass6,
    filterTypeHiPass,
    
    filterNumTypes
};

class FilterInstance
{
public:

    FilterInstance();
    void clear();
    void setFilter(double freq, double Q, double gain, MZTiFilterType filterType, double sampleRate);

    inline void processBlock(double* processData, int numSamples)
    {
        processBlock(processData, nullptr, numSamples);
    }

    inline void processBlock(double* dataL, double* dataR, int numSamples)
    {
        if (dataL != nullptr)
            for (int i = 0; i < numSamples; ++i)
                dataL[i] = params.process(stateL, dataL[i]);

        if (dataR != nullptr)
            for (int i = 0; i < numSamples; ++i)
                dataR[i] = params.process(stateR, dataR[i]);
    }

    void getResponse(float* mags, const float* normalizedFreqs, int numPoints);

private:

    MZTiFilterType curType = filterTypePeak;
    lkjb::MZTIFilterParams<double> params;
    lkjb::MZTIFilterParams<double>::State stateL;
    lkjb::MZTIFilterParams<double>::State stateR;
};


}
