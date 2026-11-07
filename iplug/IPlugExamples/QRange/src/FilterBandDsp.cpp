#include "FilterBandDsp.h"


FilterBandDsp::FilterBandDsp()    
{
    responseData.resize(1000);
}

void FilterBandDsp::clear()
{
    filter.clear();
    filterCutAux.clear();
}

void FilterBandDsp::setFilter(bool newEnabled, double newFreq, double newQ, double newGain, EFilterTypes newType, ERoutings newRouting, ECutOrders newOrder, double newSampleRate)
{
    if (enabled == newEnabled && freq == newFreq && qFactor == newQ && gain == newGain && type == newType && routing == newRouting && order == newOrder && sampleRate == newSampleRate)
        return;

    const bool cutType = newType == kTypeHiCut || newType == kTypeLoCut;

    if (enabled != newEnabled || routing != newRouting || type != newType || (cutType && order != newOrder))
        clear();

    enabled = newEnabled;
    freq = newFreq;
    qFactor = newQ;
    gain = newGain;
    type = newType;
    routing = newRouting;
    order = newOrder;
    sampleRate = newSampleRate;

    FilterStage::MZTiFilterType filterType = getMztiType(type, order);
    const bool fourthOrder = cutType && order == kCutOrder48dB;

    if (fourthOrder)
    {
        const double filterOrder = 4;
        const double pi = 3.14159265358979323846;
        const double Q0 = -0.5 / cos(pi * (2 * 0 + filterOrder + 1) / (2 * filterOrder));
        const double Q1 = -0.5 / cos(pi * (2 * 1 + filterOrder + 1) / (2 * filterOrder));
        filter.setFilter(freq, Q0, gain, filterType, sampleRate);
        filterCutAux.setFilter(freq, Q1, gain, filterType, sampleRate);
    }
    else
    {
        filter.setFilter(freq, qFactor, gain, filterType, sampleRate);
    }
}

void FilterBandDsp::processBlock(double* chL, int numSamples)
{
    processBlock(chL, nullptr, numSamples);
}

void FilterBandDsp::processBlock(double* chL, double* chR, int numSamples)
{
    if (! enabled)
        return;

    const bool cutType = type == kTypeHiCut || type == kTypeLoCut;
    const bool fourthOrder = cutType && order == kCutOrder48dB;

    if (chR == nullptr)
    {
        filter.processBlock(chL, numSamples);
        if (fourthOrder)
            filterCutAux.processBlock(chL, numSamples);
        return;
    }

    switch (routing)
    {
    case kRoutingStereo:
        filter.processBlock(chL, chR, numSamples);
        if (fourthOrder)
            filterCutAux.processBlock(chL, chR, numSamples);
        break;
    case kRoutingLeft:
        filter.processBlock(chL, numSamples);
        if (fourthOrder)
            filterCutAux.processBlock(chL, numSamples);
        break;
    case kRoutingRight:
        filter.processBlock(chR, numSamples);
        if (fourthOrder)
            filterCutAux.processBlock(chR, numSamples);
        break;
    case kRoutingMid:
        processBlockMidOrSide(chL, chR, numSamples, true);
        break;
    case kRoutingSide:
        processBlockMidOrSide(chL, chR, numSamples, false);
        break;
    default:
        jassertfalse;
        break;
    }
}

void FilterBandDsp::getResponse(ResponseData& data, const float* normalizedFreqs, int numPoints)
{
    if (! enabled)
        return;

    const bool cutType = type == kTypeHiCut || type == kTypeLoCut;
    const bool fourthOrder = cutType && order == kCutOrder48dB;

    if (numPoints > responseData.size())
        responseData.resize(numPoints);

    float* tmpData = responseData.data();

    {
        juce::FloatVectorOperations::fill(tmpData, 1.f, numPoints);
        filter.getResponse(tmpData, normalizedFreqs, numPoints);

        if (fourthOrder)
            filterCutAux.getResponse(tmpData, normalizedFreqs, numPoints);
    }

    if (data.mono) // Mono
    {
        juce::FloatVectorOperations::multiply(data.mid, tmpData, numPoints);
        return;
    }

    switch (routing)
    {
    case kRoutingStereo:
        juce::FloatVectorOperations::multiply(data.left, tmpData, numPoints);
        juce::FloatVectorOperations::multiply(data.right, tmpData, numPoints);

        break;
    case kRoutingLeft:
        juce::FloatVectorOperations::multiply(data.left, tmpData, numPoints);
        break;
    case kRoutingRight:
        juce::FloatVectorOperations::multiply(data.right, tmpData, numPoints);
        break;
    case kRoutingMid:
        juce::FloatVectorOperations::multiply(data.mid, tmpData, numPoints);
        break;
    case kRoutingSide:
        juce::FloatVectorOperations::multiply(data.side, tmpData, numPoints);
        break;
    default:
        jassertfalse;
        break;
    }
}

FilterStage::MZTiFilterType FilterBandDsp::getMztiType(EFilterTypes t, ECutOrders o) const
{
    switch (t)
    {
    default:
        jassertfalse;
    case kTypePeak:
        return FilterStage::filterTypePeak;
    case kTypeLoShelf:
        return FilterStage::filterTypeLoShelf;
    case kTypeHiShelf:
        return FilterStage::filterTypeHiShelf;
    case kTypeLoCut:
        return o == kCutOrder12dB ? FilterStage::filterTypeHiPass6 : FilterStage::filterTypeHiPass;
    case kTypeHiCut:
        return o == kCutOrder12dB ? FilterStage::filterTypeLoPass6 : FilterStage::filterTypeLoPass;
    }
}

void FilterBandDsp::processBlockMidOrSide(double* chL, double* chR, int numSamples, bool processMid)
{
    const int blockSize = 64;
    double auxMid[blockSize];
    double auxSide[blockSize];

    const bool cutType = type == kTypeHiCut || type == kTypeLoCut;
    const bool fourthOrder = cutType && order == kCutOrder48dB;

    while (numSamples > 0)
    {
        const int curNumSamples = juce::jmin(numSamples, blockSize);

        juce::FloatVectorOperations::copy(auxMid, chL, curNumSamples);
        juce::FloatVectorOperations::copy(auxSide, chL, curNumSamples);
        juce::FloatVectorOperations::add(auxMid, chR, curNumSamples);
        juce::FloatVectorOperations::subtract(auxSide, chR, curNumSamples);
        juce::FloatVectorOperations::multiply(auxMid, 0.5, curNumSamples);
        juce::FloatVectorOperations::multiply(auxSide, 0.5, curNumSamples);

        filter.processBlock(processMid ? auxMid : auxSide, curNumSamples);
        if (fourthOrder)
            filterCutAux.processBlock(processMid ? auxMid : auxSide, curNumSamples);

        juce::FloatVectorOperations::copy(chL, auxMid, curNumSamples);
        juce::FloatVectorOperations::copy(chR, auxMid,curNumSamples);
        juce::FloatVectorOperations::add(chL, auxSide, curNumSamples);
        juce::FloatVectorOperations::subtract(chR, auxSide, curNumSamples);

        numSamples -= curNumSamples;
        chL += curNumSamples;
        chR += curNumSamples;
    }
}

