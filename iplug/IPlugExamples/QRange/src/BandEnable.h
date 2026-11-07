#pragma once

#include "../JuceIncludes.h"
#include "IPlugGUIResize.h"
#include "../resource.h"
#include "../QRangeTypes.h"

class QRange;

class BandEnable : public IControl
{
public:

    BandEnable(QRange* pPlug, IRECT pR, int paramIdx, int bandIdx);

    bool Draw(IGraphics* pGraphics) override;

    void OnMouseDown(int x, int y, IMouseMod* pMod) override;

    void AfterGUIResize(double guiScaleRatio) override;

private:

    QRange* qRange;
    int bandIndex;
};
