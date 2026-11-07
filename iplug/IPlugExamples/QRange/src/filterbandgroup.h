#pragma once

#include "IPlugGUIResize.h"
#include "TextSlider.h"
#include "FilterType.h"
#include "RoutingType.h"
#include "BandEnable.h"
#include "CutOrder.h"

class QRange;

class FilterBandGroup : public IControl
{
public:
	FilterBandGroup(QRange* pPlug, int posX, int posY, int bandIdx);
	~FilterBandGroup();
	void attachControls(IGraphics* pGraphics, BandParameters& bandParams);
	void AfterGUIResize(double guiScaleRatio) override;
	bool Draw(IGraphics* pGraphics) override;

	static const int groupWidth = 61;
	static const int groupHeight = 144;

private:

    int bandIndex;
    QRange* qRange;
    BandEnable* bandEnable = nullptr;
    FilterType* filterType = nullptr;
    RoutingType* routingType = nullptr;
    TextSlider* freqSlider = nullptr;
    TextSlider* gainSlider = nullptr;
    TextSlider* qSlider = nullptr;
    CutOrder* cutOrder = nullptr;

	std::vector<IControl*> childControls;

};