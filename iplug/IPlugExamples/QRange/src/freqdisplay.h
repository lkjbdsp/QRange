#pragma once

#include "IPlugGUIResize.h"
#include "../JuceIncludes.h"
#include "FilterBandDsp.h"
#include "bandhandle.h"

class QRange;
class BandAnalyserView;

class FreqDisplay : public IControl
{
public:

	FreqDisplay(QRange* pPlug, IRECT pR);
    void setBandAnalyserView(BandAnalyserView* sub);

    bool IsDirty() override;
	bool Draw(IGraphics* pGraphics) override;
	void AfterGUIResize(double guiScaleRatio) override;
    void triggerUpdateCurves();    

    void OnMouseDown(int x, int y, IMouseMod* pMod) override;
    void OnMouseUp(int x, int y, IMouseMod* pMod) override;
    void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod) override;
    void OnMouseWheel(int x, int y, IMouseMod* pMod, int d) override;
    void OnMouseDblClick(int x, int y, IMouseMod* pMod) override;

    void setMagRange(float newRange);
    float getMagRange() const;
    float getPosFromFreq(float freq) const;
    float getFreqFromPos(float pos) const;
    float getPosFromMag(float mag) const;
    float getMagFromPos(float pos) const;
    juce::Range<float> getMagPositionRange() const;
    juce::String getStringForFreq(float freq) const;
    IRECT getFreqDisplayRect() const;
    QRange* getQRange() const;

private:

    struct AnalyserDrag
    {
        bool dragging = false;
        float startMag;
        float startRangeTop;
    };

    void drawGrid(IGraphics* pGraphics, IColor gridColor);
    void drawCurve(IGraphics* pGraphics, float* curveData, IColor color, bool thick);
    void drawCurves(IGraphics* pGraphics, bool thick);    

    IText gridText;
    IRECT displayRect;
    juce::Range<float> freqRange = juce::Range<float>(20.f, 20e3f);
    float magRange = 24;
    bool updateCurves = true;
    FilterBandDsp::ResponseData responseData;
    std::unique_ptr<BandHandles> bandHandles;
    BandAnalyserView* analyserView = nullptr;
    BandHandle* curDragHandle = nullptr;
    QRange* qRange;
    AnalyserDrag analyserDrag;
};