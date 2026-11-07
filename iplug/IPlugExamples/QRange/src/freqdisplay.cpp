#include "freqdisplay.h"
#include "../QRange.h"
#include "FilterType.h"
#include "CutOrder.h"
#include "RoutingType.h"

FreqDisplay::FreqDisplay(QRange* pPlug, IRECT pR)
	: IControl(pPlug, pR), qRange(pPlug)
{
    bandHandles.reset(new BandHandles(this));

    for (int i = 0; i < kNumBands; ++i)
        bandHandles->addHandle(i);

    gridText = mText;
    gridText.mColor = IColor(0x999999, 0xFF);
}

void FreqDisplay::setBandAnalyserView(BandAnalyserView* sub)
{
    analyserView = sub;
}

bool FreqDisplay::IsDirty()
{
    if (analyserView != nullptr && analyserView->IsDirty())
        return true;
    
    if (qRange != nullptr)
        qRange->updateAnalyserButtons();

    return IControl::IsDirty();
}

bool FreqDisplay::Draw(IGraphics* pGraphics)
{
    const juce::Time startTime(juce::Time::getCurrentTime());

    if (updateCurves && qRange != nullptr)
    {
        updateCurves = false;
        qRange->updateParameters();
        qRange->getFreqResponse(responseData, displayRect.W());
    }

	const IColor bgColor(0x111111, 0xFF);
    const IColor gridColor(0x777777, 0xFF);

	pGraphics->FillIRect(&bgColor, &mDrawRECT);

    if (analyserView != nullptr)
        analyserView->Draw(pGraphics);

    drawGrid(pGraphics, gridColor);
    drawCurves(pGraphics, qRange != nullptr && qRange->getThickCurves());
    bandHandles->draw(pGraphics);
    pGraphics->DrawRect(&gridColor, &displayRect);

    const juce::Time endTime(juce::Time::getCurrentTime());
    const juce::RelativeTime timeSpent(endTime - startTime);
    return true;
}

void FreqDisplay::AfterGUIResize(double guiScaleRatio)
{
    const int w = mDrawRECT.W();
    const int h = mDrawRECT.H();
    displayRect.L = static_cast<int> (mDrawRECT.L + w * 0.03);
    displayRect.R = static_cast<int> (mDrawRECT.R - w * 0.03);
    displayRect.T = static_cast<int> (mDrawRECT.T + h * 0.05);
    displayRect.B = static_cast<int> (mDrawRECT.B - h * 0.05);

    const int gridTextSize = static_cast<int> (h * 0.04);
    gridText = IText(gridTextSize, &gridText.mColor, gridText.mFont, gridText.mStyle, gridText.mAlign,
        gridText.mOrientation, gridText.mQuality, &gridText.mTextEntryBGColor, &gridText.mTextEntryFGColor);

    bandHandles->AfterGuiResize(guiScaleRatio);
    bandHandles->setTextSize(gridTextSize);
    
    if (analyserView != nullptr)
        analyserView->setMagnitudePositions(getPosFromMag(magRange), getPosFromMag(-magRange));

    triggerUpdateCurves();
}

void FreqDisplay::triggerUpdateCurves()
{
    updateCurves = true;
    SetDirty(false);
}

void FreqDisplay::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    BandHandle* handle = bandHandles->getNearestHandle(x + 0.5f, y + 0.5f, bandHandles->getGrabRad(), false);

    if (pMod->L)
    {
        curDragHandle = handle;

        if (curDragHandle != nullptr)
        {
            curDragHandle->dragStart(y, pMod);
        }
        else if (analyserView != nullptr && displayRect.Contains(x, y))
        {
            qRange->getBandAnalyser().clearHold();
        }
        else if (analyserView != nullptr && x > displayRect.R)
        {
            analyserDrag.startRangeTop = analyserView->getMagRange().getEnd();
            analyserDrag.startMag = analyserView->getMagFromY(static_cast<float> (y));
            analyserDrag.dragging = true;
        }
    }
    else if (pMod->R && handle != nullptr)
    {
        FilterType* type = dynamic_cast<FilterType*> (handle->getFilterType());
        CutOrder* cutOrder = dynamic_cast<CutOrder*> (handle->getCutOrder());
        RoutingType* routing = dynamic_cast<RoutingType*> (handle->getRouting());

        if (type != nullptr || cutOrder != nullptr || routing != nullptr)
        {
            const int enabledIdx = handle->getEnabledIndex();
            const bool isBandSolo = qRange->getSoloIndex() == enabledIdx;
            std::function<void(void)> soloBand = [this, enabledIdx, isBandSolo]() {
                qRange->setSoloIndex(isBandSolo ? -1 : enabledIdx);
                qRange->updateEnabledControls();
            };

            std::unique_ptr<IPopupMenu> menu(new IPopupMenu());

            menu->AddItem(new IPopupMenuItem("Solo Band", isBandSolo ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, soloBand));
            menu->AddSeparator();

            if (type != nullptr)
                menu->AddItem("Filter Type", type->createSelectionMenu());

            if (cutOrder != nullptr && handle->bandIsCut())
                menu->AddItem("Cut Order", cutOrder->createSelectionMenu());

            if (routing != nullptr)
                menu->AddItem("Routing", routing->createSelectionMenu());

            IRECT rect(x, y, x, y);
            IPlugBase::ShowMenu(menu.get(), GetGUI(), rect);
        }
    }
    else if (pMod->R && analyserView != nullptr)
    {
        if (x > displayRect.R)
        {
            std::unique_ptr<IPopupMenu> menu(getQRange()->getAnalyserRangeMenu(true));

            IRECT menuRect(x, y, x, y);
            IPlugBase::ShowMenu(menu.get(), GetPlug()->GetGUI(), menuRect);
        }
        else if (x < displayRect.L)
        {            
            std::unique_ptr<IPopupMenu> menu(getQRange()->getRangeMenu(true));

            IRECT menuRect(x, y, x, y);
            IPlugBase::ShowMenu(menu.get(), GetPlug()->GetGUI(), menuRect);
        }
    }
}

void FreqDisplay::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    if (curDragHandle != nullptr)
        curDragHandle->dragEnd();

    curDragHandle = nullptr;
    analyserDrag.dragging = false;
}

void FreqDisplay::OnMouseDrag(int x, int y, int /*dX*/, int /*dY*/, IMouseMod* pMod)
{
    if (curDragHandle != nullptr)
    {
        if (analyserView != nullptr && analyserView->IsDirty())
            SetDirty(false);

        curDragHandle->drag(x, y);
        SetDirty(false);
    }
    else if (analyserDrag.dragging)
    {
        const juce::Range<float> topRange(-30.f, 30.f);
        const float deltaMag = (analyserView->getMagFromY(static_cast<float>(y)) - analyserDrag.startMag) / 5;
        juce::Range<float> rng(analyserView->getMagRange());
        const float granularity = 6.f;
        const float newTop = topRange.clipValue(granularity * floor((analyserDrag.startRangeTop + deltaMag) / granularity + 0.5f));

        rng = juce::Range<float>(newTop - rng.getLength(), newTop);
        analyserView->setMagRange(rng);
    }
}

void FreqDisplay::OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
{
    if (BandHandle* handle = bandHandles->getNearestHandle(x + 0.5f, y + 0.5f, bandHandles->getGrabRad(), false))
    {
        handle->mouseWheel(d, pMod->S);
        SetDirty(false);
    }
}

void FreqDisplay::OnMouseDblClick(int x, int y, IMouseMod* /*pMod*/)
{
    if (BandHandle* handle = bandHandles->getNearestHandle(x + 0.5f, y + 0.5f, bandHandles->getGrabRad(), false))
    {
        handle->setEnabled(false);
    }
    else if (BandHandle* handle = bandHandles->getNearestHandle(x + 0.5f, y + 0.5f, static_cast<float> (mDrawRECT.W()), true))
    {
        handle->setEnabled(true);
        handle->drag(x, y);
        SetDirty(false);
    }
}

void FreqDisplay::setMagRange(float newRange)
{
    if (newRange < 4.f)
        return;

    newRange = 4.f * std::floor(newRange / 4.f + 0.5f);

    if (newRange != magRange)
    {
        magRange = newRange;
        SetDirty(false);
    }
}

float FreqDisplay::getMagRange() const
{
    return magRange;
}

float FreqDisplay::getPosFromFreq(float freq) const
{
    const float x = 0.5f + displayRect.L + displayRect.W() * std::log(freq / freqRange.getStart()) / std::log(freqRange.getEnd() / freqRange.getStart());
    return juce::jlimit(static_cast<float> (displayRect.L), static_cast<float> (displayRect.R), x);
}

float FreqDisplay::getFreqFromPos(float pos) const
{
    return freqRange.getStart() * std::exp((pos - displayRect.L - 0.5f) / displayRect.W() * std::log(freqRange.getEnd() / freqRange.getStart()));
}

float FreqDisplay::getPosFromMag(float mag) const
{
    mag = juce::jlimit(-magRange, magRange, mag);
    const float y = 0.5f + displayRect.MH() - 0.5f * displayRect.H() * (mag / magRange);
    return y;
}

float FreqDisplay::getMagFromPos(float pos) const
{
    return - 2.f * (pos - displayRect.MH() - 0.5f) / displayRect.H() * magRange;
}

juce::Range<float> FreqDisplay::getMagPositionRange() const
{
    const float minPos = getPosFromMag(-magRange);
    const float maxPos = getPosFromMag(magRange);
    return juce::Range<float>(juce::jmin(minPos, maxPos), juce::jmax(minPos, maxPos));
}

juce::String FreqDisplay::getStringForFreq(float freq) const
{
    if (freq < 1000)
        return juce::String(static_cast<int> (freq + 0.5f));

    return juce::String(static_cast<int> (freq / 1000 + 0.5f)) + "k";
}

IRECT FreqDisplay::getFreqDisplayRect() const
{
    return displayRect;
}

QRange* FreqDisplay::getQRange() const
{
    return qRange;
}

void FreqDisplay::drawGrid(IGraphics* pGraphics, IColor gridColor)
{
    const int numGridMags = 4;
    float gridMags[numGridMags] = { 0.f, 6.f, 12.f, 18.f };

    for (int i = 0; i < numGridMags; ++i)
        gridMags[i] = std::floor(gridMags[i] * magRange / 24.f + 0.5f);

    const int labelDist = static_cast<int> (5 * qRange->GetGUIResize()->GetGUIScaleRatio());

    {
        const float xL = static_cast<float> (displayRect.L);
        const float xR = static_cast<float> (displayRect.R);

        IText leftLabel(gridText);
        IText rightLabel(gridText);
        leftLabel.mAlign = IText::kAlignNear;
        rightLabel.mAlign = IText::kAlignFar;

        IRECT textRectMags(displayRect);
        textRectMags.L += labelDist;
        textRectMags.R -= labelDist;

        for (int i = 0; i < numGridMags; ++i)
        {
            const float mag = gridMags[i];
            const float yt = getPosFromMag(mag);
            const float yb = getPosFromMag(-mag);

            textRectMags.T = static_cast<int> (yt - gridText.mSize);
            textRectMags.B = static_cast<int> (yt);
            pGraphics->DrawLine(&gridColor, xL, yt, xR, yt);
            pGraphics->DrawIText(&leftLabel, juce::String(static_cast<int> (mag + 0.5f)).toRawUTF8(), &textRectMags);
            pGraphics->DrawIText(&rightLabel, juce::String(static_cast<int> (mag + 0.5f)).toRawUTF8(), &textRectMags);

            if (yt != yb)
            {
                textRectMags.T = static_cast<int> (yb);
                textRectMags.B = static_cast<int> (yb + gridText.mSize);
                pGraphics->DrawLine(&gridColor, xL, yb, xR, yb);
                pGraphics->DrawIText(&leftLabel, juce::String(static_cast<int> (-mag - 0.5f)).toRawUTF8(), &textRectMags);
                pGraphics->DrawIText(&rightLabel, juce::String(static_cast<int> (-mag - 0.5f)).toRawUTF8(), &textRectMags);
            }
        }
    }


    if (qRange->getAnalyserEnabled())
    {
        const int numMags = 4;
        float gridMags[numMags + 1] = { 0.f, 6.f, 12.f, 18.f, 0.f };
        gridMags[numMags] = magRange;

        IText rightLabel(gridText);
        rightLabel.mAlign = IText::kAlignNear;

        rightLabel.mColor = IColor(0xFFFFFF);

        IRECT textRectMags(mDrawRECT);
        textRectMags.L = displayRect.R + labelDist;

        for (int i = 0; i <= numMags; ++i)
        {
            const float mag = gridMags[i];
            const float yt = getPosFromMag(mag);
            const float yb = getPosFromMag(-mag);
            const float anMagTop = analyserView->getMagFromY(yt);
            const float anMagBot = analyserView->getMagFromY(yb);

            textRectMags.T = static_cast<int> (yt - gridText.mSize / 2);
            textRectMags.B = textRectMags.T + gridText.mSize;
            pGraphics->DrawIText(&rightLabel, juce::String(static_cast<int> (anMagTop)).toRawUTF8(), &textRectMags);

            if (mag != 0)
            {
                textRectMags.T = static_cast<int> (yb - gridText.mSize/2);
                textRectMags.B = textRectMags.T + gridText.mSize;
                pGraphics->DrawIText(&rightLabel, juce::String(static_cast<int> (anMagBot)).toRawUTF8(), &textRectMags);
            }
        }
    }

    {
        const int numFreqs = 8;
        const float gridFreqs[numFreqs] = { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f };
        IRECT textRectMagsT(displayRect);
        IRECT textRectMagsB(displayRect);
        IText freqLabelText(gridText);
        freqLabelText.mAlign = IText::kAlignNear;
        textRectMagsT.B = textRectMagsT.T + gridText.mSize;
        textRectMagsB.T = textRectMagsB.B - gridText.mSize;
        const float yt = static_cast<float> (displayRect.T);
        const float yb = static_cast<float> (displayRect.B);

        for (int i = 0; i < numFreqs; ++i)
        {
            const float freq = gridFreqs[i];
            const float x = getPosFromFreq(freq);
            const int textL = static_cast<int> (x + 0.003f * displayRect.W());
            const int textR = textL + displayRect.W();
            const juce::String freqStr(getStringForFreq(freq));
            textRectMagsT.L = textL;
            textRectMagsT.R = textR;
            textRectMagsB.L = textL;
            textRectMagsB.R = textR;

            pGraphics->DrawLine(&gridColor, x, yt, x, yb);
            pGraphics->DrawIText(&freqLabelText, freqStr.toRawUTF8(), &textRectMagsT);
            pGraphics->DrawIText(&freqLabelText, freqStr.toRawUTF8(), &textRectMagsB);
        }
    }
}

void FreqDisplay::drawCurve(IGraphics* pGraphics, float* curveData, IColor color, bool thick)
{
    float lastY = getPosFromMag(curveData[0]);
    int index = 1;

    if (thick)
    {
        for (int x = displayRect.L + 1; x < displayRect.R; ++x)
        {
            const float y = getPosFromMag(curveData[index]);
            const float xf = static_cast<float> (x);
            pGraphics->DrawLine(&color, xf, lastY + 0.5f, xf, y + 0.5f);
            pGraphics->DrawLine(&color, xf, lastY - 0.5f, xf, y - 0.5f);
            lastY = y;
            ++index;
        }
    }
    else
    {
        for (int x = displayRect.L + 1; x < displayRect.R; ++x)
        {
            const float y = getPosFromMag(curveData[index]);
            const float xf = static_cast<float> (x);
            pGraphics->DrawLine(&color, xf, lastY, xf, y);
            lastY = y;
            ++index;
        }
    }
}

void FreqDisplay::drawCurves(IGraphics* pGraphics, bool thick)
{
    float* mainCurve = responseData.left;
    float* auxCurve = responseData.right;
    float* midCurve = responseData.mid;
    float* sideCurve = responseData.side;

    if (auxCurve == nullptr || mainCurve == nullptr)
    {
        jassertfalse;
        return;
    }

    IColor mainColor(qRange->getColourForRouting(kRoutingLeft));
    IColor auxColor(qRange->getColourForRouting(kRoutingRight));
    IColor midColor(qRange->getColourForRouting(kRoutingMid));
    IColor sideColor(qRange->getColourForRouting(kRoutingSide));

    drawCurve(pGraphics, mainCurve, mainColor, thick);

    if (qRange->usesRouting(kRoutingRight) || qRange->usesRouting(kRoutingLeft))
        drawCurve(pGraphics, auxCurve, auxColor, thick);

    if (qRange->usesRouting(kRoutingMid))
        drawCurve(pGraphics, midCurve, midColor, thick);

    if (qRange->usesRouting(kRoutingSide))
        drawCurve(pGraphics, sideCurve, sideColor, thick);
}

