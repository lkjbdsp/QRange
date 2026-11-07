#include "BandAnalyserView.h"

BandAnalyserView::BandAnalyserView(BandAnalyser& analyser)
    : bandAnalyser(analyser)
{
}

void BandAnalyserView::setFreqMapping(std::function<float(float)> f2x, std::function<float(float)> x2f)
{
    freqToX = f2x;
    xToFreq = x2f;
    updateNumCurvePoints();
}

void BandAnalyserView::setMagnitudePositions(float yTop, float yBot)
{
    magPosRange = juce::Range<float> (yTop, yBot);
}

void BandAnalyserView::setMagRange(const juce::Range<float>& newRange)
{
    magRange = newRange;
}

juce::Range<float> BandAnalyserView::getMagRange() const
{
    return magRange;
}

bool BandAnalyserView::IsDirty()
{
    const double refreshFreq = 20;
    std::function<void(std::vector<float>&)> magToY = [this] (std::vector<float>& vec) {
        for (auto& e: vec)
            e = getYFromMag(20*std::log10(juce::jmax(1e-10f, e)));
    };

    if (! bandAnalyser.getEnabled())
        return false;

    if (juce::RelativeTime(juce::Time::getCurrentTime() - lastDrawTime).inSeconds() > 1/refreshFreq)
    {
        lastDrawTime = juce::Time::getCurrentTime();

        bandAnalyser.getData(peakCurve, holdCurve);
        magToY(peakCurve);
        magToY(holdCurve);
        bandDataExists = true;
        return true;
    }

    return false;
}

void BandAnalyserView::Draw(IGraphics* pGraphics)
{
    if (bandFreqs.size() == 0 || ! bandAnalyser.getEnabled())
        return;

    if (showPeak && showFilledPeak)
        fillCurve(pGraphics, peakCurve, IColor(0xFFAAAA, 0x40));
    
    if (showPeak)
        drawCurve(pGraphics, peakCurve, IColor(0xFFAAAA));

    if (showHold)
        drawCurve(pGraphics, holdCurve, IColor(0xDDFFDD));
}

float BandAnalyserView::getYFromMag(float mag) const
{
    mag = magRange.clipValue(mag);
    return magPosRange.getStart() + magPosRange.getLength() * (1.f - (mag - magRange.getStart()) / magRange.getLength());
}

float BandAnalyserView::getMagFromY(float y) const
{
    return (1.f - (y - magPosRange.getStart()) / magPosRange.getLength()) * magRange.getLength() + magRange.getStart();
}

void BandAnalyserView::updateNumCurvePoints()
{
    const size_t numBands = bandAnalyser.getNumBands();

    if (bandFreqs.size() != numBands)
    {
        bandFreqs.resize(numBands);
        peakCurve.resize(numBands);
        holdCurve.resize(numBands);
        bandDataExists = false;
    }

    bandAnalyser.getFreqs(bandFreqs);
}

void BandAnalyserView::setShowFilledPeak(bool show)
{
    showFilledPeak = show;
}

bool BandAnalyserView::getShowFilledPeak() const
{
    return showFilledPeak;
}

void BandAnalyserView::setShowPeak(bool show)
{
    showPeak = show;
}

bool BandAnalyserView::getShowPeak() const
{
    return showPeak;
}

void BandAnalyserView::setShowHold(bool show)
{
    showHold = show;
}

bool BandAnalyserView::getShowHold() const
{
    return showHold;
}

void BandAnalyserView::drawCurve(IGraphics* pGraphics, const std::vector<float>& data, IColor col)
{
    jassert(data.size() == bandFreqs.size() && freqToX != nullptr && data.size() > 0);

    if (freqToX == nullptr || data.size() != bandFreqs.size() || data.size() == 0 || ! bandDataExists)
        return;

    const size_t numBands = data.size();
    float lastX = freqToX(bandFreqs[0]);
    float lastY = data[0];

    for (size_t i=1; i<numBands; ++i)
    {
        const float x = freqToX(bandFreqs[i]);
        const float y = data[i];
        pGraphics->DrawLine(&col, lastX, lastY, x, y);
        lastX = x;
        lastY = y;
    }
}

void BandAnalyserView::fillCurve(IGraphics* pGraphics, const std::vector<float>& data, IColor col)
{
    jassert(data.size() == bandFreqs.size() && freqToX != nullptr && data.size() > 0);

    if (freqToX == nullptr || data.size() != bandFreqs.size() || data.size() == 0 || ! bandDataExists)
        return;

    const size_t numBands = data.size();
    float lastX = std::floor(freqToX(bandFreqs[0]));
    float lastY = data[0];

    for (size_t i = 1; i < numBands; ++i)
    {
        const float nextX = std::floor(freqToX(bandFreqs[i]));
        const float nextY = data[i];
        if (nextX <= lastX)
            continue;

        const float dydx = (nextY - lastY) / (nextX - lastX);
        const float yTop = magPosRange.getStart();
        const float yBot = magPosRange.getEnd();
        IChannelBlend blend(IChannelBlend::kBlendNone, col.A / 255.f);

        for (float x = lastX; x < nextX; ++x)
        {
            const float y0 = magPosRange.clipValue(lastY + (x - lastX) * dydx);
            
            pGraphics->DrawLine(&col, x, y0, x, magPosRange.getEnd(), &blend);
        }

        lastX = nextX;
        lastY = nextY;
    }
}

