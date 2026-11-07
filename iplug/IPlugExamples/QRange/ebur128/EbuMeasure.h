#pragma once
#include <memory>

#include "../SampleDelay.h"

class EbuMeasure
{
public:

    EbuMeasure();
    ~EbuMeasure();

    void setSampleRate(double sampleRate);
    void clear();
    void setDelay(int newDelay);

    void processBlock(const double* chL, const double* chR, int numSamples);

    double getValue() const;

private:
    
    class EbuR128;

    std::unique_ptr<EbuR128> ebuR128;
    SampleDelay<double> delay[2];
    double sampleRate = 0;
};

