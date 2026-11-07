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

	void getReversed(double* left, double* right)
	{
		int idx = index;

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

	void getBuffer(double* left, double* right)
	{
		int idx = index + 1;

        if (right == nullptr)
        {
            for (int i = 0; i < size; ++i)
            {
                if (idx >= size)
                    idx = 0;

                left[i] = bufferL[idx++];
            }
        }
        else
        {
            for (int i = 0; i < size; ++i)
            {
                if (idx >= size)
                    idx = 0;

                right[i] = bufferR[idx];
                left[i] = bufferL[idx++];
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

    enum ProcMode
    {
        kWindowPre,
        kWindowBoth,
        kWindowPost,

        kNumProcModes
    };

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

    void processBlockReversed(double* outL, double* outR, const double* inL, double* inR, int numSamples);
    void processBlockForward(double* outL, double* outR, const double* inL, double* inR, int numSamples);

    void setProcMode(ProcMode newMode);
    ProcMode getProcMode() const;

    int getLatencySamples() const;

private:

	void reverseBuffer(double* data) const;
	void moveOutput(double* data) const;

	Filter& filter;
	int blockSize = 0;
	int outputIndex = 0;

    CircularBuffer inputBuffer;
    juce::HeapBlock<double> procBufferL;
    juce::HeapBlock<double> procBufferR;
	int inputCounter = 0;

    juce::HeapBlock<double> outputBufferL;
    juce::HeapBlock<double> outputBufferR;
    juce::HeapBlock<double> window;
    juce::HeapBlock<double> window2;

    ProcMode procMode = kWindowBoth;
};
