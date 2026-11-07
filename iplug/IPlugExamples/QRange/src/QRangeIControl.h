#pragma once

#include "IPlugGUIResize.h"

class QRange;

class QRangeIControl : public IControl
{
public:

    QRangeIControl(IPlugBase* pPlug, IRECT pR, int paramIdx = -1, IChannelBlend blendMethod = IChannelBlend::kBlendNone)
        : IControl(pPlug, pR, paramIdx, blendMethod)
    {
    }
    
    virtual ~QRangeIControl();

protected:

    QRange* getQRange();
    void setFreqDisplayDirty();
};