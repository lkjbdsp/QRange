#include "BandEnable.h"
#include "../QRange.h"

BandEnable::BandEnable(QRange* pPlug, IRECT pR, int paramIdx, int bandIdx)
    : IControl(pPlug, pR, paramIdx), bandIndex(bandIdx), qRange(pPlug)
{

}

bool BandEnable::Draw(IGraphics* pGraphics)
{
    if (const IParam* param = bandIndex >= 0 ? GetParam() : nullptr)
    {
        const IColor colEnabled(0xDDDDDD, 0xFF);
        const IColor colDisabled(0x666666, 0xFF);
        const IColor colSolo(0x229922, 0xFF);
        const IColor colNoSoloOn(0xBB3333, 0xFF);
        const IColor colNoSoloOff(0x661111, 0xFF);

        const int soloIdx = qRange->getSoloIndex();
        const juce::String text(bandIndex + 1);
        mText.mColor = soloIdx < 0 ? (param->Bool() ? colEnabled : colDisabled) : (soloIdx == ParamIdx() ? colSolo : param->Bool() ? colNoSoloOn : colNoSoloOff);
        pGraphics->DrawIText(&mText, text.toRawUTF8(), &mDrawRECT);
        return true;
    }

    return false;
}

void BandEnable::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    if (IParam* param = GetParam())
    {
        const int soloIdx = qRange->getSoloIndex();

        if (pMod->R && soloIdx != ParamIdx())
        {
            mValue = 1;
            qRange->setSoloIndex(ParamIdx());
            qRange->updateEnabledControls();
            SetDirty(true);
        }
        else
        {
            qRange->setSoloIndex(-1);
         
            if (soloIdx < 0)
                mValue = param->Bool() ? 0 : 1;

            SetDirty(true);
        }
    }
}

void BandEnable::AfterGUIResize(double guiScaleRatio)
{
    mText = IText(mDrawRECT.H(), &mText.mColor, mText.mFont, mText.mStyle, mText.mAlign, mText.mOrientation, mText.mQuality, &mText.mTextEntryBGColor, &mText.mTextEntryFGColor);
}