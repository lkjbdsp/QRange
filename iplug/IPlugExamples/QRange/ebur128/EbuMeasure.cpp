#include "EbuMeasure.h"

#include "ebur128.c"

class EbuMeasure::EbuR128
{
public:

    EbuR128(double SR, mode m)
        : sampleRate(SR), ebuMode(m)
    {
        state = ebur128_init(numChannels, static_cast<unsigned long> (sampleRate + 0.5), ebuMode);
        ebur128_set_channel(state, 0, EBUR128_LEFT);
        ebur128_set_channel(state, 1, EBUR128_RIGHT);
    }

    ~EbuR128()
    {
        if (state != nullptr)
            ebur128_destroy(&state);
    }

    void clear()
    {
        if (state != nullptr)
            ebur128_destroy(&state);

        state = ebur128_init(numChannels, static_cast<unsigned long> (sampleRate + 0.5), ebuMode);
        ebur128_set_channel(state, 0, EBUR128_LEFT);
        ebur128_set_channel(state, 1, EBUR128_RIGHT);
    }

    ebur128_state* getState()
    {
        return state;
    }

    int getNumChannels() const
    {
        return numChannels;
    }

    double getSampleRate() const
    {
        return sampleRate;
    }

    void processBlock(const double* data, int numSamples)
    {
        ebur128_add_frames_double(state, data, static_cast<size_t> (numSamples));
    }

    double getValue() const
    {
        double out = 0;
        switch (ebuMode)
        {
        default:
            jassertfalse;
            break;
        case EBUR128_MODE_M:
            ebur128_loudness_momentary(state, &out);
            break;
        case EBUR128_MODE_S:
            ebur128_loudness_shortterm(state, &out);
            break;
        case EBUR128_MODE_I:
            ebur128_loudness_global(state, &out);
            break;
        }
        return out;
    }

private:
    static const int numChannels = 2;
    double sampleRate;
    mode ebuMode;
    ebur128_state* state = nullptr;
};



EbuMeasure::EbuMeasure()
{
}

EbuMeasure::~EbuMeasure()
{    
}

void EbuMeasure::setSampleRate(double sampleRate)
{
    if (ebuR128.get() == nullptr || ebuR128->getSampleRate() != sampleRate)
        ebuR128.reset(new EbuR128(sampleRate, EBUR128_MODE_S));
}

void EbuMeasure::clear()
{
    if (ebuR128.get() != nullptr)
        ebuR128->clear();

    delay[0].clear();
    delay[1].clear();
}

void EbuMeasure::setDelay(int newDelay)
{
    delay[0].setSize(static_cast<size_t>(newDelay));
    delay[1].setSize(static_cast<size_t>(newDelay));
}

void EbuMeasure::processBlock(const double* chL, const double* chR, int numSamples)
{
    if (ebuR128.get() != nullptr)
    {
        if (chR == nullptr)
            chR = chL;

        const int blockSize = 64;
        const int numChannels = 2;
        double delL[blockSize];
        double delR[blockSize];
        double data[blockSize*numChannels];

        while (numSamples > 0)
        {
            const int curNumSamples = numSamples > blockSize ? blockSize : numSamples;

            if (delay[0].getSize() > 0)
            {
                delay[0].processBlock(delL, chL, curNumSamples);
                delay[1].processBlock(delR, chR, curNumSamples);

                for (int i = 0; i < curNumSamples; ++i)
                {
                    data[numChannels*i + 0] = delL[i];
                    data[numChannels*i + 1] = delR[i];
                }
            }
            else
            {
                for (int i = 0; i < curNumSamples; ++i)
                {
                    data[numChannels*i + 0] = chL[i];
                    data[numChannels*i + 1] = chR[i];
                }
            }

            ebuR128->processBlock(data, curNumSamples);

            chL += curNumSamples;
            chR += curNumSamples;
            numSamples -= curNumSamples;
        }
    }
}

double EbuMeasure::getValue() const
{
    return ebuR128.get() != nullptr ? ebuR128->getValue() : 0;
}
