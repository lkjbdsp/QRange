#include "QRangeIControl.h"

#include "../QRange.h"
#include "freqdisplay.h"

QRangeIControl::~QRangeIControl()
{
}

QRange* QRangeIControl::getQRange()
{
    return dynamic_cast<QRange*> (GetPlug());
}

void QRangeIControl::setFreqDisplayDirty()
{
    if (QRange* qRange = getQRange())
        qRange->getFreqDisplay()->triggerUpdateCurves();
}