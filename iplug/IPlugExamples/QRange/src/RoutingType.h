#include "IPlugGUIResize.h"
#include "../resource.h"
#include "../QRangeTypes.h"
#include "../JuceIncludes.h"
#include "QRangeIControl.h"

class QRange;

class RoutingType : public QRangeIControl
{
public:

    RoutingType(IPlugBase* pPlug, IRECT pR, int paramIdx);

    bool Draw(IGraphics* pGraphics) override;
    void AfterGUIResize(double guiScaleRatio) override;

    IPopupMenu* createSelectionMenu();
    void OnMouseDown(int x, int y, IMouseMod* pMod) override;
    void OnMouseWheel(int /*x*/, int /*y*/, IMouseMod* pMod, int d) override;

protected:

    void menuCallback(int value);
};

class AnalyserRouting : public QRangeIControl
{
public:
    
    AnalyserRouting(QRange* pPlug, IRECT pR, int textSize, IColor color, IColor bgCol);

    bool Draw(IGraphics* pGraphics) override;
    void AfterGUIResize(double guiScaleRatio) override;
    IPopupMenu* createSelectionMenu();
    void setTextColor(IColor color);

    void OnMouseDown(int x, int y, IMouseMod* pMod) override;
    void OnMouseWheel(int /*x*/, int /*y*/, IMouseMod* pMod, int d) override;

private:

    QRange* qRange;
    int size;
    IColor bgColor;
};