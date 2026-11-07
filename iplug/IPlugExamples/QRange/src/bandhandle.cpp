#include "bandhandle.h"

#include "../QRange.h"
#include "freqdisplay.h"

BandHandle::BandHandle(FreqDisplay* display, int idx)
    : freqDisplay(display), qRange(display->getQRange()), index(idx), paramStartIndex(idx * kNumParametersPerBand)
{    
}

int BandHandle::getEnabledIndex() const
{
    return paramStartIndex + kEnabled;
}

bool BandHandle::isEnabled()
{
    return  getParam(getEnabledIndex())->Bool();
}

void BandHandle::setTextSize(int newTextSize)
{
    IColor col(0xCCCCCC);
    text = IText(newTextSize, &col, "Verdana", IText::kStyleBold);
}

void BandHandle::draw(IGraphics* pGraphics)
{
    if (! isEnabled())
        return;

    const QRange* qRange = freqDisplay != nullptr ? freqDisplay->getQRange() : nullptr;

    const DispPosition pos(getPosition());
    const float x = pos.x - 1.f;
    const float y = pos.y - 1.f;
    const float rad = getHandleRad();
    IRECT rect(x - rad, y - rad, x + rad, y + rad);

    const ERoutings routing = static_cast<ERoutings> (getParam(paramStartIndex + kRouting)->Int());

    IColor bgCol(0, 255);
    IColor cirCol(qRange != nullptr && qRange->getColorHints() ? qRange->getBandColor(index) : IColor(0xCCCCCC));

    pGraphics->FillCircle(&bgCol, x, y, rad);

    if (routing != kRoutingStereo)
    {
        IColor ovCol(qRange->getColourForRouting(routing));
        IChannelBlend blend(IChannelBlend::kBlendNone, 0.4f);
        pGraphics->FillCircle(&ovCol, x, y, rad, &blend);
    }

    pGraphics->DrawIText(&text, juce::String(index + 1).toRawUTF8(), &rect);

    if (qRange->getThickCurves())
    {
        pGraphics->DrawCircle(&cirCol, x, y, rad - 0.5f);
        pGraphics->DrawCircle(&cirCol, x, y, rad + 0.5f);
    }
    else
    {
        pGraphics->DrawCircle(&cirCol, x, y, rad);
    }
}

void BandHandle::dragStart(int y, IMouseMod* pMod)
{
    dragQFactor = pMod->C;

    if (dragQFactor)
    {
        qRange->BeginInformHostOfParamChange(paramStartIndex + kQ);
        qDragStartVal = getParam(paramStartIndex + kQ)->GetNormalized();
        qDragStartY = y;
    }
    else
    {
        qRange->BeginInformHostOfParamChange(paramStartIndex + kFreq);

        if (! bandIsCut())
            qRange->BeginInformHostOfParamChange(paramStartIndex + kGain);
    }
}

void BandHandle::drag(int x, int y)
{
    if (dragQFactor)
    {
        const double mod = 0.002;
        const double newQ = juce::jlimit(0., 1., qDragStartVal + mod * (qDragStartY - y));
        qRange->SetParameterFromGUI(paramStartIndex + kQ, newQ);
    }
    else
    {
        const float newFreq = freqDisplay->getFreqFromPos(x + 0.5f);
        const double freqNorm = getParam(paramStartIndex + kFreq)->GetNormalized(newFreq);

        qRange->SetParameterFromGUI(paramStartIndex + kFreq, freqNorm);
        qRange->setControlValue(paramStartIndex + kFreq, freqNorm);

        if (! bandIsCut())
        {
            double shelfMul = bandIsShelf() ? 2. : 1.;
            const double newMag = shelfMul * freqDisplay->getMagFromPos(y + 0.5f);
            const double magNorm = getParam(paramStartIndex + kGain)->GetNormalized(newMag);
            qRange->SetParameterFromGUI(paramStartIndex + kGain, magNorm);
            qRange->setControlValue(paramStartIndex + kGain, magNorm);
        }
    }
}

void BandHandle::dragEnd()
{
    if (dragQFactor)
    {
        qRange->BeginInformHostOfParamChange(paramStartIndex + kQ);
    }
    else
    {
        qRange->EndInformHostOfParamChange(paramStartIndex + kFreq);

        if (! bandIsCut())
            qRange->EndInformHostOfParamChange(paramStartIndex + kGain);
    }

}

void BandHandle::mouseWheel(int d, bool fine)
{
    const double curQ = getParam(paramStartIndex + kQ)->GetNormalized();
    const double delta = d * (fine ? 0.01 : 0.05);
    const double newQ = juce::jlimit(0., 1., curQ + delta);
    qRange->SetParameterFromGUI(paramStartIndex + kQ, newQ);
    qRange->setControlValue(paramStartIndex + kQ, newQ);
}

void BandHandle::setEnabled(bool en)
{
    const double newEn = en ? 1. : 0.;
    qRange->SetParameterFromGUI(paramStartIndex + kEnabled, newEn);
    qRange->setControlValue(paramStartIndex + kQ, newEn);
}

BandHandle::DispPosition BandHandle::getPosition()
{
    const bool isCut = bandIsCut();
    double shelfMul = bandIsShelf() ? 0.5 : 1.;
    const float x = freqDisplay->getPosFromFreq(static_cast<float> (getParam(paramStartIndex + kFreq)->Value())) - 0.5f;
    const float y = freqDisplay->getPosFromMag(isCut ? 0.f : static_cast<float> (shelfMul * getParam(paramStartIndex + kGain)->Value())) - 0.5f;
    return DispPosition(x, y);
}

bool BandHandle::bandIsCut()
{
    const EFilterTypes filterType = static_cast<EFilterTypes> (getParam(paramStartIndex + kType)->Int());
    return filterType == kTypeHiCut || filterType == kTypeLoCut;
}

bool BandHandle::bandIsShelf()
{
    const EFilterTypes filterType = static_cast<EFilterTypes> (getParam(paramStartIndex + kType)->Int());
    return filterType == kTypeHiShelf || filterType == kTypeLoShelf;
}

IParam* BandHandle::getParam(int idx)
{
    return qRange->GetParam(idx); 
}

float BandHandle::getHandleRad() const
{
    return 0.7f * text.mSize;
}


IControl* BandHandle::getFilterType()
{
    return qRange->getControlForIndex(paramStartIndex + kType);
}

IControl* BandHandle::getRouting()
{
    return qRange->getControlForIndex(paramStartIndex + kRouting);
}

IControl* BandHandle::getCutOrder()
{
    return qRange->getControlForIndex(paramStartIndex + kCutOrder);
}

BandHandles::BandHandles(FreqDisplay* display)
    : freqDisplay(display)
{

}

void BandHandles::addHandle(int index)
{
    handles.add(new BandHandle(freqDisplay, index));
}

void BandHandles::AfterGuiResize(double newScale)
{
    scale = static_cast<float> (newScale);
}

void BandHandles::setTextSize(int newTextSize)
{
    grabRad = 1.2f * newTextSize;

    for (auto h : handles)
        h->setTextSize(newTextSize);
}

void BandHandles::draw(IGraphics* pGraphics)
{
    for (auto h : handles)
        h->draw(pGraphics);
}

float BandHandles::getGrabRad() const
{
    return grabRad;
}

BandHandle* BandHandles::getNearestHandle(float x, float y, float maxDist, bool onlyUnused)
{
    BandHandle* ret = nullptr;
    float lastDist = maxDist;
    const BandHandle::DispPosition p0(x, y);

    if (! freqDisplay->getFreqDisplayRect().Contains(static_cast<int> (x), static_cast<int> (y)))
        return nullptr;

    for (auto h : handles)
    {
        if (h->isEnabled() == onlyUnused) 
            continue;

        const BandHandle::DispPosition p(h->getPosition());
        const float dist = p.getDistance(p0);

        if (dist < lastDist)
        {
            lastDist = dist;
            ret = h;
        }
    }

    return ret;
}

