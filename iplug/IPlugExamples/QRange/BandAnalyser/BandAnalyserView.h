#pragma once

#include "IControl.h"
#include "../JuceIncludes.h"
#include "BandAnalyser.h"

class BandAnalyserView
{
public:

    BandAnalyserView(BandAnalyser& analyser);
    void setFreqMapping(std::function<float(float)> f2x, std::function<float(float)> x2f);    
    void setMagnitudePositions(float yTop, float yBot);
    void setMagRange(const juce::Range<float>& newRange);
    juce::Range<float> getMagRange() const;

    bool IsDirty();
    void Draw(IGraphics* pGraphics);

    float getYFromMag(float mag) const;
    float getMagFromY(float y) const;
    
    void updateNumCurvePoints();

    void setShowFilledPeak(bool show);
    bool getShowFilledPeak() const;
    void setShowPeak(bool show);
    bool getShowPeak() const;
    void setShowHold(bool show);
    bool getShowHold() const;

private:

    void drawCurve(IGraphics* pGraphics, const std::vector<float>& data, IColor col);
    void fillCurve(IGraphics* pGraphics, const std::vector<float>& data, IColor col);

    BandAnalyser& bandAnalyser;    
    std::function<float(float)> freqToX;
    std::function<float(float)> xToFreq;
    juce::Range<float> magPosRange;
    juce::Range<float> magRange = juce::Range<float> (-60.f, 6.f);

    std::vector<float> peakCurve;
    std::vector<float> holdCurve;
    std::vector<float> bandFreqs;
    bool bandDataExists = false;
    bool showFilledPeak = true;
    bool showPeak = true;
    bool showHold = true;

    juce::Time lastDrawTime;
};


