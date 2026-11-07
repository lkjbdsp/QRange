#pragma once

#include "IPlugGUIResize.h"
#include "../JuceIncludes.h"

class TextSlider : public IControl
{
public:
	TextSlider(IPlugBase* pPlug, IRECT pR, int paramIdx)
		: IControl(pPlug, pR, paramIdx)
	{
		mDisablePrompt = false;
		IColor col(0xDDDDDD, 0xFF);
		mText = IText(&col);
	}

	bool Draw(IGraphics* pGraphics) override
	{
		if (const IParam* param = GetParam())
		{
			const juce::String val(getDispText());
			pGraphics->DrawIText(&mText, val.toRawUTF8(), &mDrawRECT);
			return true;
		}
		
		return false;
	}

	void AfterGUIResize(double guiScaleRatio) override
	{
        const int size = textSize > 0 ? static_cast<int> (guiScaleRatio * textSize) : mDrawRECT.H();
		mText = IText(size, &mText.mColor, mText.mFont, mText.mStyle, mText.mAlign, mText.mOrientation, mText.mQuality, &mText.mTextEntryBGColor, &mText.mTextEntryFGColor);
	}

	void OnMouseDrag(int /*x*/, int /*y*/, int /*dX*/, int dY, IMouseMod* pMod) override 
	{
		const double mod = pMod != nullptr && pMod->S ? 0.0001 : 0.001;
		mValue -= mod * dY;
		SetDirty(true);
	}

	void OnMouseDblClick(int x, int y, IMouseMod* pMod) override
	{
		PromptUserInput();
	}

	void OnMouseWheel(int /*x*/, int /*y*/, IMouseMod* pMod, int d) override
	{
		const double mod = pMod != nullptr && pMod->S ? 0.001 : 0.01;
		mValue += mod * d;
		SetDirty(true);
	}

    void setTextSize(int nonScaledSize)
    {
        textSize = nonScaledSize;
    }

	bool isFreq = false;
	bool isQ = false;
	bool isGain = false;

private:

	juce::String getDispText()
	{
		juce::String text;

		if (const IParam* param = GetParam())
		{
			const double value = param->Value();

			if (isFreq)
				text = value < 100 ? juce::String(value, 1)+" Hz" : value < 1000 ? juce::String(static_cast<int> (value+0.5)) + " Hz" : juce::String(value*0.001, 2) + " kHz";
			else if (isQ)
				text = juce::String(value, 3) + " Q";
			else if (isGain)
				text = juce::String(value, 1) + " dB";
		}

		return text;
	}

    int textSize = -1;
};