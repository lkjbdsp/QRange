#include "IPlugGUIResize.h"
#include "../resource.h"
#include "../QRangeTypes.h"
#include "../JuceIncludes.h"
#include "QRangeIControl.h"

class CutOrder : public QRangeIControl
{
public:

    CutOrder(IPlugBase* pPlug, IRECT pR, int paramIdx)
        : QRangeIControl(pPlug, pR, paramIdx)
    {
        IColor col(0xDDDDDD, 0xFF);
        mText = IText(&col);
    }

    bool Draw(IGraphics* pGraphics) override
    {
        if (const IParam* param = GetParam())
        {
            const int type = juce::jlimit(0, static_cast<int> (kNumCutOrders), static_cast<int> (param->Value() + 0.5));
            pGraphics->DrawIText(&mText, CutOrderTypeNames::cutOrderNames[type], &mDrawRECT);
            return true;
        }

        return false;
    }

    void AfterGUIResize(double guiScaleRatio) override
    {
        mText = IText(mDrawRECT.H(), &mText.mColor, mText.mFont, mText.mStyle, mText.mAlign, mText.mOrientation, mText.mQuality, &mText.mTextEntryBGColor, &mText.mTextEntryFGColor);
    }

    IPopupMenu* createSelectionMenu()
    {
        if (IParam* param = GetParam())
        {
            IPopupMenu* menu = new IPopupMenu();
            const int curIdx = static_cast<int> (param->Value() + 0.5);

            for (int i = 0; i < kNumCutOrders; ++i)
            {
                std::function<void()> callback = [this, i]() { this->menuCallback(i); };
                const int flag = i == curIdx ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
                menu->AddItem(new IPopupMenuItem(CutOrderTypeNames::cutOrderNames[i], flag, callback));
            }

            return menu;
        }

        jassertfalse;
        return nullptr;
    }

    void OnMouseDown(int x, int y, IMouseMod* pMod) override
    {
        if (IParam* param = GetParam())
        {
            std::unique_ptr<IPopupMenu> menu(createSelectionMenu());
            IRECT rect(mDrawRECT.Contains(x, y) ? mDrawRECT : IRECT(x, y, x, y));
            IPlugBase::ShowMenu(menu.get(), GetGUI(), rect);
        }
    }

    void OnMouseWheel(int /*x*/, int /*y*/, IMouseMod* pMod, int d) override
    {
        if (IParam* param = GetParam())
        {
            mValue = param->GetNormalized(param->Value() + d);
            SetDirty();
        }
    }

private:

    void menuCallback(int value)
    {
        if (IParam* param = GetParam())
        {
            mValue = param->GetNormalized(value);
            SetDirty();
            setFreqDisplayDirty();
        }
    }
};