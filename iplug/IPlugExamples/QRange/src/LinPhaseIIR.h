#pragma once

#include "../JuceIncludes.h"
#include "../SampleDelay.h"


class CircularBuffer
{
public:

	CircularBuffer()
	{
	}

    void clear()
    {
        bufferL.clear(size);
        bufferR.clear(size);
        index = 0;
    }

    void setSize(int newSize)
    {
        if (newSize != size)
        {
            size = newSize;
            bufferL.realloc(size);
            bufferR.realloc(size);
			clear();
        }
    }

	int getSize() const
	{
		return size;
	}

    void push(double sample)
    {
        bufferL[index++] = sample;
        if (index >= size)
            index = 0;
    }

    void push(double left, double right)
    {
        bufferR[index] = right;
        bufferL[index++] = left;

        if (index >= size)
            index = 0;
    }

    void push(const double* left, const double* right, int numSamples)
    {
        jassert(numSamples < size);

        if (numSamples < size - index)
        {
            juce::FloatVectorOperations::copy(bufferL.get() + index, left, numSamples);

            if (right != nullptr)
                juce::FloatVectorOperations::copy(bufferR.get() + index, right, numSamples);

            index += numSamples;
        }
        else
        {
            const int numWrite0 = size - index;
            const int numWrite1 = numSamples - numWrite0;
            juce::FloatVectorOperations::copy(bufferL.get() + index, left, numWrite0);
            juce::FloatVectorOperations::copy(bufferL.get(), left + numWrite0, numWrite1);

            if (right != nullptr)
            {
                juce::FloatVectorOperations::copy(bufferR.get() + index, right, numWrite0);
                juce::FloatVectorOperations::copy(bufferR.get(), right + numWrite0, numWrite1);
            }
            index = numWrite1;
        }
    }

    void getReversed(double* left, double* right) const
    {
        int idx = index - 1;
        if (idx < 0)
            idx = size - 1;

        if (right == nullptr)
        {
            for (int i = 0; i < size; ++i)
            {
                left[i] = bufferL[idx--];

                if (idx < 0)
                    idx = size - 1;
            }
        }
        else
        {
            for (int i = 0; i < size; ++i)
            {
                right[i] = bufferR[idx];
                left[i] = bufferL[idx--];

                if (idx < 0)
                    idx = size - 1;
            }
        }
    }

    void getBuffer(double* left, double* right) const
    {
        int idx = index;
        if (right == nullptr)
        {
            for (int i = 0; i < size; ++i)
            {
                left[i] = bufferL[idx++];

                if (idx >= size)
                    idx = 0;
            }
        }
        else
        {
            for (int i = 0; i < size; ++i)
            {
                right[i] = bufferR[idx];
                left[i] = bufferL[idx++];

                if (idx >= size)
                    idx = 0;
            }
        }
    }

private:

    juce::HeapBlock<double> bufferL;
    juce::HeapBlock<double> bufferR;
    int size = 0;
    int index = 0;
};

class LinPhaseIIR
{
public:

    class Filter
    {
    public:
        virtual ~Filter() {}
        virtual void clearFilter() = 0;
        virtual void processFilter(double* samples, int numSamples) = 0;
        virtual void processFilter(double* samplesL, double* samplesR, int numSamples) = 0;
    };

    LinPhaseIIR(Filter& f);
    void clear();

    void setBlockSize(int newBlockSize);

    void processBlockReversed(double* output, const double* input, int numSamples);
    void processBlockForward(double* output, const double* input, int numSamples);

    void processBlockReversed(double* outL, double* outR, const double* inL, const double* inR, int numSamples);
    void processBlockForward(double* outL, double* outR, const double* inL, const double* inR, int numSamples);

    int getLatencySamples() const;

private:

    struct Stage
    {
        CircularBuffer input;
        std::vector<double> accL, accR; 
        std::vector<double> fifoL, fifoR;
        int counter = 0;
        int fifoPos = 0;
    };

    void allocate();
    void processStage(Stage& s, bool reversed, double* outL, double* outR,
                      const double* inL, const double* inR, int numSamples);
    void runBlock(Stage& s, bool reversed, bool stereo);
    static void reverse(double* data, int n);

    Filter& filter;
    int blockSize = 0;
    int tailLength = 0;

    Stage reversedStage;
    Stage forwardStage;

    std::vector<double> procBufferL;
    std::vector<double> procBufferR;
    std::vector<double> window;
    std::vector<double> window2;
};
