#pragma once

#include "../QRange.h"
#include "../JuceIncludes.h"

namespace BandSorter
{
struct Band
{
    static int compareElements(Band first, Band second)
    {
        return first.freq < second.freq ? -1 : first.freq > second.freq ? 1 : 0;
    }

    double params[kNumParametersPerBand];
    double freq;
};

Band getBand(QRange* qRange, int bandIdx)
{
    Band band;
    const int offset = bandIdx * kNumParametersPerBand;

    for (int i = 0; i < kNumParametersPerBand; ++i)
        band.params[i] = qRange->GetParam(offset + i)->Value();

    band.freq = band.params[kFreq];

    return band;
}

void setBand(QRange* qRange, int bandIdx, Band& band)
{
    const int offset = bandIdx * kNumParametersPerBand;

    for (int i = 0; i < kNumParametersPerBand; ++i)
        qRange->GetParam(offset + i)->Set(band.params[i]);
}

void sortBands(QRange* qRange)
{
    juce::Array<Band> bands;
    for (int i = 0; i < kNumBands; ++i)
        bands.add(getBand(qRange, i));

    Band compare;
    bands.sort(compare, true);

    for (int i = 0; i < kNumBands; ++i)
        setBand(qRange, i, bands.getReference(i));
}
}
