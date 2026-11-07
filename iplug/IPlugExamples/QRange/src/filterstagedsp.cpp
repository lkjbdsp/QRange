#include "filterstagedsp.h"
#include "math.h"

#define PI_FLOAT 3.14159265358979323846
#include <cmath>


namespace FilterStage
{

FilterInstance::FilterInstance()
{
}

void FilterInstance::clear()
{
    stateL.clear();
    stateR.clear();
}

void FilterInstance::setFilter(double freq, double Q, double gain, MZTiFilterType filterType, double sampleRate)
{
    if (curType != filterType)
        clear();

    auto type = filterType == filterTypePeak ? lkjb::filterTypePeak
        : filterType == filterTypeLoShelf ? lkjb::filterTypeLoShelf
        : filterType == filterTypeHiShelf ? lkjb::filterTypeHiShelf
        : filterType == filterTypeLoPass ? lkjb::filterTypeLoPass
        : filterType == filterTypeLoPass6 ? lkjb::filterTypeLoPass6
        : filterType == filterTypeHiPass ? lkjb::filterTypeHiPass
        : filterType == filterTypeHiPass6 ? lkjb::filterTypeHiPass6
        : lkjb::filterNumTypes;

    if (type == lkjb::filterNumTypes)
        return;

    curType = filterType;
    lkjb::MatchedZTransform::setMZTiFilter(params, freq, Q, gain, type, sampleRate);
}

void FilterInstance::getResponse(float* mags, const float* normalizedFreqs, int numPoints)
{
    for (int i = 0; i < numPoints; ++i)
        mags[i] *= static_cast<float> (params.response(normalizedFreqs[i]));
}

}

