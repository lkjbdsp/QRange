#include "LinPhaseIIR.h"


LinPhaseIIR::LinPhaseIIR(Filter& f)
	: filter(f)
{
	setBlockSize(8192);
}

void LinPhaseIIR::clear()
{
	inputCounter = 0;
	inputBuffer.clear();
    outputBufferL.clear(blockSize * 3 / 2);
    outputBufferR.clear(blockSize * 3 / 2);
	outputIndex = 0;
}

void LinPhaseIIR::setBlockSize(int newBlockSize)
{
	if (newBlockSize != blockSize)
	{
		jassert((newBlockSize & 1) == 0);
		blockSize = newBlockSize;
		inputBuffer.setSize(blockSize);
        procBufferL.realloc(blockSize);
        procBufferR.realloc(blockSize);
        outputBufferL.realloc(blockSize * 3 / 2);
        outputBufferR.realloc(blockSize * 3 / 2);
        window.realloc(blockSize);
        window2.realloc(blockSize);

        for (int i = 0; i < blockSize; ++i)
        {
            window[i] = 0.5f - 0.5f * std::cos(2. * juce::double_Pi * i / (blockSize));
            window2[i] = std::sqrt(window[i]);
        }

		clear();
	}
}

void LinPhaseIIR::processBlockReversed(double* output, const double* input, int numSamples)
{
	for (int i=0; i<numSamples; ++i)
	{
		inputBuffer.push(input[i]);

		if (outputIndex >= blockSize / 2)
		{
            auto w = procMode == kWindowBoth ? window2.get() : window.get();
            outputIndex = 0;
			inputBuffer.getReversed(procBufferL.get(), nullptr);
            
            if (procMode != kWindowPost)
                juce::FloatVectorOperations::multiply(procBufferL.get(), w, blockSize);

			filter.clearFilter();
			filter.processFilter(procBufferL.get(), blockSize);
			reverseBuffer(procBufferL.get());

            if (procMode != kWindowPre)
                juce::FloatVectorOperations::multiply(procBufferL.get(), w, blockSize);

			moveOutput(outputBufferL.get());

            juce::FloatVectorOperations::add(outputBufferL + blockSize / 2, procBufferL.get(), blockSize);
		}
	
		output[i] = outputBufferL[outputIndex++];
	}
}

void LinPhaseIIR::processBlockForward(double* output, const double* input, int numSamples)
{
	for (int i = 0; i < numSamples; ++i)
	{
		inputBuffer.push(input[i]);

		if (outputIndex >= blockSize / 2)
		{
            auto w = procMode == kWindowBoth ? window2.get() : window.get();
            outputIndex = 0;
			inputBuffer.getBuffer(procBufferL.get(), nullptr);

            if (procMode != kWindowPost)
                juce::FloatVectorOperations::multiply(procBufferL.get(), w, blockSize);

			filter.clearFilter();
			filter.processFilter(procBufferL.get(), blockSize);
            
            if (procMode != kWindowPre)
                juce::FloatVectorOperations::multiply(procBufferL.get(), w, blockSize);

			moveOutput(outputBufferL.get());

            juce::FloatVectorOperations::add(outputBufferL + blockSize / 2, procBufferL.get(), blockSize);
		}

		output[i] = outputBufferL[outputIndex++];
	}
}

void LinPhaseIIR::processBlockReversed(double* outL, double* outR, const double* inL, double* inR, int numSamples)
{
    if (outR == nullptr || inR == nullptr)
    {
        processBlockReversed(outL, inL, numSamples);
        return;
    }
    
    for (int i = 0; i < numSamples; ++i)
    {
        inputBuffer.push(inL[i], inR[i]);

        if (outputIndex >= blockSize / 2)
        {
            auto w = procMode == kWindowBoth ? window2.get() : window.get();
            outputIndex = 0;
            inputBuffer.getReversed(procBufferL.get(), procBufferR.get());

            if (procMode != kWindowPost)
            {
                juce::FloatVectorOperations::multiply(procBufferL.get(), w, blockSize);
                juce::FloatVectorOperations::multiply(procBufferR.get(), w, blockSize);
            }

            filter.clearFilter();
            filter.processFilter(procBufferL.get(), procBufferR.get(), blockSize);
            reverseBuffer(procBufferL.get());
            reverseBuffer(procBufferR.get());

            if (procMode != kWindowPre)
            {
                juce::FloatVectorOperations::multiply(procBufferL.get(), w, blockSize);
                juce::FloatVectorOperations::multiply(procBufferR.get(), w, blockSize);
            }

            moveOutput(outputBufferL.get());
            moveOutput(outputBufferR.get());

            juce::FloatVectorOperations::add(outputBufferL.get() + blockSize / 2, procBufferL.get(), blockSize);
            juce::FloatVectorOperations::add(outputBufferR.get() + blockSize / 2, procBufferR.get(), blockSize);
        }

        outR[i] = outputBufferR[outputIndex];
        outL[i] = outputBufferL[outputIndex++];
    }
}

void LinPhaseIIR::processBlockForward(double* outL, double* outR, const double* inL, double* inR, int numSamples)
{
    if (outR == nullptr || inR == nullptr)
    {
        processBlockForward(outL, inL, numSamples);
        return;
    }
    
    for (int i = 0; i < numSamples; ++i)
    {
        inputBuffer.push(inL[i], inR[i]);

        if (outputIndex >= blockSize / 2)
        {
            auto w = procMode == kWindowBoth ? window2.get() : window.get();
            outputIndex = 0;
            inputBuffer.getBuffer(procBufferL.get(), procBufferR.get());

            if (procMode != kWindowPost)
            {
                juce::FloatVectorOperations::multiply(procBufferL.get(), w, blockSize);
                juce::FloatVectorOperations::multiply(procBufferR.get(), w, blockSize);
            }

            filter.clearFilter();
            filter.processFilter(procBufferL.get(), procBufferR.get(), blockSize);

            if (procMode != kWindowPre)
            {
                juce::FloatVectorOperations::multiply(procBufferL.get(), w, blockSize);
                juce::FloatVectorOperations::multiply(procBufferR.get(), w, blockSize);
            }

            moveOutput(outputBufferL.get());
            moveOutput(outputBufferR.get());

            juce::FloatVectorOperations::add(outputBufferL.get() + blockSize / 2, procBufferL.get(), blockSize);
            juce::FloatVectorOperations::add(outputBufferR.get() + blockSize / 2, procBufferR.get(), blockSize);
        }

        outR[i] = outputBufferR[outputIndex];
        outL[i] = outputBufferL[outputIndex++];
    }
}

void LinPhaseIIR::setProcMode(ProcMode newMode)
{
    procMode = newMode;
}

LinPhaseIIR::ProcMode LinPhaseIIR::getProcMode() const
{
    return procMode;
}

int LinPhaseIIR::getLatencySamples() const
{
    return 2 * blockSize * 3 / 2 - 4;
}

void LinPhaseIIR::reverseBuffer(double* data) const
{	
	for (int i = 0; i < blockSize / 2; ++i)
	{
		const double tmp = data[blockSize - i - 1];
		data[blockSize - i - 1] = data[i];
		data[i] = tmp;
	}
}

void LinPhaseIIR::moveOutput(double* data) const
{
	for (int i = 0; i < blockSize; ++i)
        data[i] = data[i + blockSize / 2];

    juce::FloatVectorOperations::clear(data + blockSize, blockSize / 2);
}
