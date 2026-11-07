#include "filterbandgroup.h"
#include "../resource.h"
#include "../QRangeTypes.h"
#include "../QRange.h"

FilterBandGroup::FilterBandGroup(QRange* pPlug, int posX, int posY, int bandIdx)
	: IControl(pPlug, IRECT(posX, posY, posX + groupWidth, posY + groupHeight)), qRange(pPlug),
    bandIndex(bandIdx)
{
    const int paramIdxStart = bandIndex * kNumParametersPerBand;
	const int xL = 2;
	const int xR = 59;
    const int yEnabledT = 2;
	const int yFreqT = 37;
	const int yGainT = 50;
	const int yQT = 104;
	const int yTypeT = 63;
    const int yRoutingT = 124;
    const int hEnabled = 29;
    const int hSlider = 13;
	const int hType = 40;
    bandEnable = new BandEnable(pPlug, IRECT(posX + xL, posY + yEnabledT, posX + xR, posY + yEnabledT + hEnabled), paramIdxStart + kEnabled, bandIndex);
    freqSlider = new TextSlider(pPlug, IRECT(posX + xL, posY + yFreqT, posX + xR, posY + yFreqT + hSlider), paramIdxStart + kFreq);
	qSlider = new TextSlider(pPlug, IRECT(posX + xL, posY + yQT, posX + xR, posY + yQT + hSlider), paramIdxStart + kQ);
	gainSlider = new TextSlider(pPlug, IRECT(posX + xL, posY + yGainT, posX + xR, posY + yGainT + hSlider), paramIdxStart + kGain);
	filterType = new FilterType(pPlug, IRECT(posX + xL, posY + yTypeT, posX + xR, posY + yTypeT + hType), paramIdxStart + kType);
    routingType = new RoutingType(pPlug, IRECT(posX + xL, posY + yRoutingT, posX + xR, posY + yRoutingT + hSlider), paramIdxStart + kRouting);
    cutOrder = new CutOrder(pPlug, IRECT(posX + xL, posY + yGainT, posX + xR, posY + yGainT + hSlider), paramIdxStart + kCutOrder);

	freqSlider->isFreq = true;
	qSlider->isQ = true;
	gainSlider->isGain = true;
    cutOrder->Hide(true);

    childControls.push_back(bandEnable);
    childControls.push_back(filterType);
    childControls.push_back(routingType);
    childControls.push_back(freqSlider);
    childControls.push_back(gainSlider);
    childControls.push_back(qSlider);
    childControls.push_back(cutOrder);
}

FilterBandGroup::~FilterBandGroup()
{
}

void FilterBandGroup::attachControls(IGraphics* pGraphics, BandParameters& bandParams)
{
    bandParams.enabled = pGraphics->AttachControl(bandEnable);
    bandParams.type = pGraphics->AttachControl(filterType);
    bandParams.routing = pGraphics->AttachControl(routingType);
    bandParams.freq = pGraphics->AttachControl(freqSlider);
    bandParams.gain = pGraphics->AttachControl(gainSlider);
    bandParams.Q = pGraphics->AttachControl(qSlider);
    bandParams.cutOrder = pGraphics->AttachControl(cutOrder);
}

void FilterBandGroup::AfterGUIResize(double guiScaleRatio)
{
	for (auto c : childControls)
		if (c != nullptr)
			c->AfterGUIResize(guiScaleRatio);
}

bool FilterBandGroup::Draw(IGraphics* pGraphics)
{
    const ERoutings routing = static_cast<ERoutings> (routingType->GetParam()->Int());

	IColor bgColor(qRange->getColourForRouting(routing));
    IColor bgDisabled(0x333333);
	IColor frameColour1(0xAAAAAA, 0xFF);
	IColor frameColour2(0x666666, 0xFF);
    bgColor.A = 0x55;

    const bool enabled = bandEnable != nullptr && bandEnable->GetParam() != nullptr && bandEnable->GetParam()->Bool();

    pGraphics->FillIRect(enabled ? &bgColor : &bgDisabled, &mDrawRECT);

    if (enabled && qRange->getColorHints())
    {
        IColor c(qRange->getBandColor(bandIndex));
        IRECT rect(mDrawRECT.GetPadded(-1));
        pGraphics->DrawRect(&c, &rect);

        rect = rect.GetPadded(-1);

        if (qRange->getThickCurves())
            pGraphics->DrawRect(&c, &rect);
    }


	pGraphics->DrawLine(&frameColour1, static_cast<float> (mDrawRECT.L), static_cast<float> (mDrawRECT.T), static_cast<float> (mDrawRECT.R), static_cast<float> (mDrawRECT.T));
	pGraphics->DrawLine(&frameColour1, static_cast<float> (mDrawRECT.L), static_cast<float> (mDrawRECT.T), static_cast<float> (mDrawRECT.L), static_cast<float> (mDrawRECT.B));
	pGraphics->DrawLine(&frameColour2, static_cast<float> (mDrawRECT.L), static_cast<float> (mDrawRECT.B), static_cast<float> (mDrawRECT.R), static_cast<float> (mDrawRECT.B));
	pGraphics->DrawLine(&frameColour2, static_cast<float> (mDrawRECT.R), static_cast<float> (mDrawRECT.T), static_cast<float> (mDrawRECT.R), static_cast<float> (mDrawRECT.B));

	return true;
}

