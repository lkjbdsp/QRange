#pragma once

#include "IPlugGUIResize.h"
#include "../JuceIncludes.h"

class FreqDisplay;
class QRange;

class BandHandle
{
public:

    struct DispPosition
    {
        DispPosition(float initX = 0.f, float initY = 0.f) : x(initX), y(initY) {}
        DispPosition(int initX, int initY) : x(static_cast<float> (initX)), y(static_cast<float> (initY)) {}
        float getDistance(const DispPosition& other) const
        {
            const float dx = x - other.x;
            const float dy = y - other.y;
            return std::sqrt(dx * dx + dy * dy);
        }
        float x;
        float y;
    };

    BandHandle(FreqDisplay* display, int index);

    int getEnabledIndex() const;
    bool isEnabled();
    void setTextSize(int newTextSize);
    void draw(IGraphics* pGraphics);
    void dragStart(int y, IMouseMod* pMod);
    void drag(int x, int y);
    void dragEnd();
    void mouseWheel(int d, bool fine);
    void setEnabled(bool en);

    DispPosition getPosition();
    IControl* getFilterType();
    IControl* getRouting();
    IControl* getCutOrder();
    bool bandIsCut();
    bool bandIsShelf();

private:

    IParam* getParam(int idx);
    float getHandleRad() const;

    FreqDisplay* freqDisplay;
    QRange* qRange;
    int index;
    int paramStartIndex;
    bool dragQFactor = false;
    double qDragStartVal = 0;
    int qDragStartY = 0;
    IText text;
};


class BandHandles
{
public:

    BandHandles(FreqDisplay* display);
    void addHandle(int index);

    void AfterGuiResize(double newScale);
    void setTextSize(int newTextSize);
    void draw(IGraphics* pGraphics);

    float getGrabRad() const;
    BandHandle* getNearestHandle(float x, float y, float maxDist, bool onlyUnused);

private:

    float scale = 1.f;
    float grabRad = 0.f;
    FreqDisplay* freqDisplay;
    juce::OwnedArray<BandHandle> handles;
};

