#pragma once

#include <vector>
#include "JuceIncludes.h"

template <class DataType>
class SampleDelay
{
public:

    SampleDelay(size_t size = 0)
    {
        setSize(size);
    }

    void setSize(size_t size)
    {
        if (data.size() != size)
        {
            data.resize(size);
            clear();
        }
    }

    size_t getSize() const
    {
        return data.size();
    }

    void clear()
    {
        juce::FloatVectorOperations::clear(data.data(), static_cast<int> (data.size()));
        index = 0;
    }

    void processBlock(DataType* out, const DataType* in, int numSamples)
    {
        if (data.size() == 0)
        {
            memcpy(out, in, numSamples * sizeof(DataType));
            return;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            const DataType y = data[index];
            data[index] = in[i];

            if (++index >= data.size())
                index = 0;

            out[i] = y;
        }
    }

protected:

    std::vector<DataType> data;
    size_t index = 0;
};