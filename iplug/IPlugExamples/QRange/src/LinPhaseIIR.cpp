#include <cmath>
#include "LinPhaseIIR.h"

namespace MathConst
{
    static constexpr double doublePi = 3.141592653589793;
}

LinPhaseIIR::LinPhaseIIR(Filter& f)
    : filter(f)
{
    setBlockSize(8192);
}

void LinPhaseIIR::allocate()
{
    const int len = blockSize + tailLength;

    procBufferL.assign(len, 0.0);
    procBufferR.assign(len, 0.0);

    for (Stage* s : { &reversedStage, &forwardStage })
    {
        s->input.setSize(blockSize);
        s->accL.assign(len, 0.0);
        s->accR.assign(len, 0.0);
        s->fifoL.assign(blockSize / 2, 0.0);
        s->fifoR.assign(blockSize / 2, 0.0);
        s->counter = 0;
        s->fifoPos = 0;
    }

    window.resize(blockSize);
    window2.resize(blockSize);
    for (int i = 0; i < blockSize; ++i)
    {
        window[i] = 0.5 - 0.5 * std::cos(2.0 * MathConst::doublePi * (i + 0.5) / blockSize);
        window2[i] = std::sqrt(window[i]);
    }
}

void LinPhaseIIR::clear()
{
    for (Stage* s : { &reversedStage, &forwardStage })
    {
        s->input.clear();
        std::fill(s->accL.begin(), s->accL.end(), 0.0);
        std::fill(s->accR.begin(), s->accR.end(), 0.0);
        std::fill(s->fifoL.begin(), s->fifoL.end(), 0.0);
        std::fill(s->fifoR.begin(), s->fifoR.end(), 0.0);
        s->counter = 0;
        s->fifoPos = 0;
    }
}

void LinPhaseIIR::setBlockSize(int newBlockSize)
{
    if (newBlockSize != blockSize)
    {
        blockSize = newBlockSize & ~1;
        tailLength = blockSize;
        allocate();
    }
}

int LinPhaseIIR::getLatencySamples() const
{
    return 2 * blockSize + tailLength - 2;
}

void LinPhaseIIR::reverse(double* data, int n)
{
    for (int i = 0, j = n - 1; i < j; ++i, --j)
        std::swap(data[i], data[j]);
}

void LinPhaseIIR::runBlock(Stage& s, bool reversed, bool stereo)
{
    const int N = blockSize;
    const int H = N / 2;
    const int len = N + tailLength;
    const double* w = window.data();

    double* pL = procBufferL.data();
    double* pR = stereo ? procBufferR.data() : nullptr;

    if (reversed)
        s.input.getReversed(pL, pR);
    else
        s.input.getBuffer(pL, pR);

    {
        for (int i = 0; i < N; ++i)
        {
            pL[i] *= w[i];
            if (stereo) pR[i] *= w[i];
        }
    }

    for (int i = N; i < len; ++i)
    {
        pL[i] = 0.0;
        if (stereo) pR[i] = 0.0;
    }

    filter.clearFilter();
    if (stereo)
        filter.processFilter(pL, pR, len);
    else
        filter.processFilter(pL, len);

    if (reversed)
    {
        reverse(pL, len);
        if (stereo) reverse(pR, len);
    }

    for (int i = 0; i < len; ++i)
    {
        s.accL[i] += pL[i];
        if (stereo) s.accR[i] += pR[i];
    }

    std::copy(s.accL.begin(), s.accL.begin() + H, s.fifoL.begin());
    std::copy(s.accL.begin() + H, s.accL.end(), s.accL.begin());
    std::fill(s.accL.end() - H, s.accL.end(), 0.0);

    if (stereo)
    {
        std::copy(s.accR.begin(), s.accR.begin() + H, s.fifoR.begin());
        std::copy(s.accR.begin() + H, s.accR.end(), s.accR.begin());
        std::fill(s.accR.end() - H, s.accR.end(), 0.0);
    }
}

void LinPhaseIIR::processStage(Stage& s, bool reversed, double* outL, double* outR,
                               const double* inL, const double* inR, int numSamples)
{
    const bool stereo = outR != nullptr && inR != nullptr;
    const int H = blockSize / 2;

    for (int i = 0; i < numSamples; ++i)
    {
        if (stereo)
            s.input.push(inL[i], inR[i]);
        else
            s.input.push(inL[i]);

        if (++s.counter >= H)
        {
            s.counter = 0;
            runBlock(s, reversed, stereo);
            s.fifoPos = 0;
        }

        outL[i] = s.fifoL[s.fifoPos];
        if (stereo)
            outR[i] = s.fifoR[s.fifoPos];
        ++s.fifoPos;
    }
}

void LinPhaseIIR::processBlockReversed(double* output, const double* input, int numSamples)
{
    processStage(reversedStage, true, output, nullptr, input, nullptr, numSamples);
}

void LinPhaseIIR::processBlockForward(double* output, const double* input, int numSamples)
{
    processStage(forwardStage, false, output, nullptr, input, nullptr, numSamples);
}

void LinPhaseIIR::processBlockReversed(double* outL, double* outR, const double* inL, const double* inR, int numSamples)
{
    processStage(reversedStage, true, outL, outR, inL, inR, numSamples);
}

void LinPhaseIIR::processBlockForward(double* outL, double* outR, const double* inL, const double* inR, int numSamples)
{
    processStage(forwardStage, false, outL, outR, inL, inR, numSamples);
}
