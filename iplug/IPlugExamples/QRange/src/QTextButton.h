#pragma once

#include "IPlugGUIResize.h"
#include "../JuceIncludes.h"
#include <functional>
#include <string>

class QTextButton : public IControl
{
public:

    QTextButton(IPlugBase* pPlug, int x, int y, int w, int h, int paramIdx, IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
        : IControl(pPlug, IRECT(x, y, x+w, y+h), paramIdx, blendMethod)
    {
        mDefaultValue = 0;
    }

    QTextButton(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
        :IControl(pPlug, IRECT(x, y, x+pBitmap->W, y+pBitmap->H/pBitmap->N), paramIdx, blendMethod), mBitmap(pBitmap)
    {
        mDefaultValue = 0;
    }

    void setButtonText(const juce::String& t, int height, IColor onCol, IColor offCol)
    {
        text = t;
        size = height;
        onColor = onCol;
        offColor = offCol;
    }

    void setCallback(std::function<void()> cb)
    {
        callback = cb;
    }    

    void setDirtyCallback(std::function<bool(void)> cb)
    {
        dirtyCallback = cb;
    }

    void AfterGUIResize(double guiScaleRatio) override
    {
        scale = guiScaleRatio;
        iText = IText(static_cast<int> (scale * size), &onColor, "Verdana", IText::kStyleNormal, IText::kAlignCenter, 0);
    }

    bool IsDirty() override
    {
        if (dirtyCallback != nullptr && dirtyCallback())
            return true;

        return IControl::IsDirty();
    }

    bool Draw(IGraphics* pGraphics) override
    {
        if (mBitmap != nullptr)
        {
            const int i = mBitmap->N > 1 ? BOUNDED(1 + int(0.5 + mValue * (double) (mBitmap->N - 1)), 1, mBitmap->N) : 1;
            pGraphics->DrawBitmap(mBitmap, &mDrawRECT, i, &mBlend);
        }
        else
        {
            IColor greyCol(0x773333);
            IColor bgOn(0xCCCCCC);
            IColor bgOff(0x444444);
            IColor bgOnMO(0xAAAAAA);
            IColor bgOffMO(0x333333);
            IColor colOn(mouseOver ? bgOnMO : bgOn);
            IColor colOff(mouseOver ? bgOffMO : bgOff);
            const bool state = mValue > 0.5;
            const bool greyed = dirtyCallback != nullptr && IsGrayed();
            pGraphics->FillIRect(greyed ? &greyCol : state ? &colOn : &colOff, &mDrawRECT);

            pGraphics->DrawRect(&bgOn, &mDrawRECT);
        }
        
        iText.mColor = mValue > 0.5 ? onColor : offColor;

        pGraphics->DrawIText(&iText, text.toRawUTF8(), &mDrawRECT);
        return true;
    }

    void OnMouseOver(int, int, IMouseMod*) override
    {
        if (! mouseOver)
        {
            mouseOver = true;
            SetDirty(false);
        }
    }
    
    void OnMouseOut() override
    {
        mouseOver = false;
        SetDirty(false);
    }

    void OnMouseDown(int x, int y, IMouseMod* pMod) override
    {
        if (mParamIdx < 0)
        {
            if (toggleCallback != nullptr)
            {
                mValue = mValue == 1 ? 0 : 1;
                toggleCallback(mValue > 0.5);
                SetDirty(true);
            }
            else
            {
                mValue = 1;
                SetDirty(false);
            }
        }
        else
        {
            if (mBitmap != nullptr && mBitmap->N > 1)
                mValue += 1.0 / (double) (mBitmap->N - 1);
            else
                mValue += 1.0;

            if (mValue > 1.001)
                mValue = 0.0;

            SetDirty();
        }
    }    

    void OnMouseUp(int x, int y, IMouseMod* pMod) override
    {
        if (mParamIdx < 0)
        {
            if (toggleCallback == nullptr)
            {
                mValue = 0;
                SetDirty(false);

                if (callback != nullptr && mDrawRECT.Contains(x, y))
                    callback();
            }
        }
    }

    void setToggleCallback(std::function<void(bool)> toggle)
    {
        toggleCallback = toggle;
    }

protected:

    juce::String text;
    IText iText;
    int size;
    double scale = 1;
    IColor onColor;
    IColor offColor;
    std::function<void()> callback;
    std::function<void(bool)> toggleCallback;
    std::function<bool()> dirtyCallback;
    IBitmap* mBitmap = nullptr;
    bool mouseOver = false;
};

class QLabel : public IControl
{
public:
    QLabel(IPlugBase* pPlug, IRECT pR, const char* labelText, int textSize, IColor color, IColor bgCol)
        : IControl(pPlug, pR), text(labelText), size(textSize), iText(textSize, &color, "Verdana"), bgColor(bgCol)
    {

    }

    void AfterGUIResize(double scale)
    {
        guiScale = scale;
        iText = IText(static_cast<int> (size * scale), &iText.mColor, iText.mFont, IText::kStyleNormal, alignment);
    }

    bool Draw(IGraphics* pGraphics)
    {
        if (bgColor.A > 0)
            pGraphics->FillIRect(&bgColor, &mDrawRECT);

        pGraphics->DrawIText(&iText, text.c_str(), &mDrawRECT);
        return true;
    }

    void setTextColor(IColor color)
    {
        if (iText.mColor != color)
        {
            iText.mColor = color;
            SetDirty(false);
        }
    }

    void setAlignment(IText::EAlign newAlignment)
    {
        alignment = newAlignment;
        AfterGUIResize(guiScale);
    }

private:

    IColor bgColor;
    std::string text;
    int size;
    IText iText;
    IText::EAlign alignment = IText::kAlignCenter;
    double guiScale = 1;
};