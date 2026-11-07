#pragma once

#include "SampleDelay.h"

class PluginBypass
{
public:

    PluginBypass()
    {
        setSampleRate(44100);
    }

    void setBlockSize(size_t blockSize)
    {
        if (bufferL.size() != blockSize)
        {
            bufferL.resize(blockSize);
            bufferR.resize(blockSize);
            clear();
        }
    }

    void setSampleRate(double sampleRate)
    {
        fadeLenSamples = static_cast<int> (sampleRate * fadeLength);
    }

    void setLatency(size_t latency)
    {
        if (delayL.getSize() != latency)
        {
            delayL.setSize(latency);
            delayR.setSize(latency);
            clear();
        }
    }

    void setBypass(bool newBypass)
    {
        if (bypass != newBypass)
        {
            bypass = newBypass;
            fadeIdx = fadeLenSamples;
        }
    }

    void clear()
    {
        delayL.clear();
        delayR.clear();
        bufferL.clear();
        bufferR.clear();
        fadeIdx = 0;
    }

    void processBlockPre(const double* inL, const double* inR, int numSamples)
    {
        delayL.processBlock(bufferL.data(), inL, numSamples);

        if (inR != nullptr)
            delayR.processBlock(bufferR.data(), inR, numSamples);
    }

    void processBlockPost(double* outL, double* outR, const double* inL, const double* inR, int numSamples)
    {
        int startIdx = 0;

        if (fadeIdx > 0)
        {
            const int fadeNumSamples = numSamples < fadeIdx ? numSamples : fadeIdx;
            const int fadeStart = fadeLenSamples - fadeIdx;

            processFade(outL, bypass ? bufferL.data() : inL, bypass ? inL : bufferL.data(), fadeStart, fadeNumSamples);

            if (outR != nullptr && inR != nullptr)
                processFade(outR, bypass ? bufferR.data() : inR, bypass ? inR : bufferR.data(), fadeStart, fadeNumSamples);

            startIdx += fadeNumSamples;
            fadeIdx -= fadeNumSamples;
            numSamples -= fadeNumSamples;
        }

        if (numSamples > 0)
        {
            if (bypass)
            {
                memcpy(outL + startIdx, bufferL.data() + startIdx, sizeof(double) * numSamples);
                
                if (outR != nullptr)
                    memcpy(outR + startIdx, bufferR.data() + startIdx, sizeof(double) * numSamples);
            }
            else
            {
                memcpy(outL + startIdx, inL + startIdx, sizeof(double) * numSamples);

                if (outR != nullptr && inR != nullptr)
                    memcpy(outR + startIdx, inR+ startIdx, sizeof(double) * numSamples);
            }
        }
    }



private:

    void processFade(double* out, const double* inFrom, const double* inTo, int fadeStart, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            double gain = (fadeStart + i) / static_cast<double> (fadeLenSamples);
            out[i] = inFrom[i] * gain + inTo[i] * (1 - gain);
        }
    }

    bool bypass = false;
    float fadeLength = 0.005f;
    int fadeLenSamples = 0;
    int fadeIdx = 0;
    SampleDelay<double> delayL;
    SampleDelay<double> delayR;
    std::vector<double> bufferL;
    std::vector<double> bufferR;
};