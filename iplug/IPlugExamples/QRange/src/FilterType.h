#pragma once

#include "IPlugGUIResize.h"
#include "../resource.h"
#include "../QRangeTypes.h"
#include "../JuceIncludes.h"
#include "QRangeIControl.h"

class QRange;

class FilterType : public QRangeIControl
{
public:
    FilterType(IPlugBase* pPlug, IRECT pR, int paramIdx);

    bool Draw(IGraphics* pGraphics) override;

    void AfterGUIResize(double guiScaleRatio) override;

    IPopupMenu* createSelectionMenu();

    void OnMouseDown(int x, int y, IMouseMod* pMod) override;

    void OnMouseWheel(int x, int y, IMouseMod* pMod, int d) override;

private:

    void menuCallback(int value);

    QRange* getQRange();
    void setFreqDisplayDirty();

};