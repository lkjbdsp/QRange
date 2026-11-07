#include "RoutingType.h"
#include "../QRange.h"

RoutingType::RoutingType(IPlugBase* pPlug, IRECT pR, int paramIdx)
    : QRangeIControl(pPlug, pR, paramIdx)
{
    IColor col(0xDDDDDD, 0xFF);
    mText = IText(&col);
}

bool RoutingType::Draw(IGraphics* pGraphics)
{
    if (const IParam* param = GetParam())
    {
        const int type = juce::jlimit(0, static_cast<int> (kNumRoutings), static_cast<int> (param->Value() + 0.5));
        pGraphics->DrawIText(&mText, RoutingTypeNames::routingNames[type], &mDrawRECT);
        return true;
    }

    return false;
}

void RoutingType::AfterGUIResize(double guiScaleRatio)
{
    mText = IText(mDrawRECT.H(), &mText.mColor, mText.mFont, mText.mStyle, mText.mAlign, mText.mOrientation, mText.mQuality, &mText.mTextEntryBGColor, &mText.mTextEntryFGColor);
}

IPopupMenu* RoutingType::createSelectionMenu()
{
    if (IParam* param = GetParam())
    {
        IPopupMenu* menu = new IPopupMenu();
        const int curIdx = static_cast<int> (param->Value() + 0.5);

        for (int i = 0; i < kNumRoutings; ++i)
        {
            std::function<void()> callback = [this, i]() { this->menuCallback(i); };
            const int flag = i == curIdx ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
            menu->AddItem(new IPopupMenuItem(RoutingTypeNames::routingNames[i], flag, callback));
        }

        return menu;
    }

    jassertfalse;
    return nullptr;
}


void RoutingType::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    if (IParam* param = GetParam())
    {
        std::unique_ptr<IPopupMenu> menu(createSelectionMenu());
        IRECT rect(mDrawRECT.Contains(x, y) ? mDrawRECT : IRECT(x, y, x, y));

        IPlugBase::ShowMenu(menu.get(), GetGUI(), rect);
    }
}

void RoutingType::OnMouseWheel(int /*x*/, int /*y*/, IMouseMod* pMod, int d)
{
    if (IParam* param = GetParam())
    {
        mValue = param->GetNormalized(param->Value() + d);
        SetDirty();
    }
}

void RoutingType::menuCallback(int value)
{
    if (IParam* param = GetParam())
    {
        mValue = param->GetNormalized(value);
        SetDirty();
        setFreqDisplayDirty();
    }
}



AnalyserRouting::AnalyserRouting(QRange* pPlug, IRECT pR, int textSize, IColor color, IColor bgCol)
    : QRangeIControl(pPlug, pR, -1), qRange(pPlug), size(textSize), bgColor(bgCol)
{
    mText = IText(size, &color, "Verdana");
}

bool AnalyserRouting::Draw(IGraphics* pGraphics)
{
    if (bgColor.A > 0)
        pGraphics->FillIRect(&bgColor, &mDrawRECT);

    const int type = juce::jlimit(0, static_cast<int> (kNumRoutings), static_cast<int> (qRange->getAnalyserRouting() + 0.5));
    IText iText(mText);
    iText.mAlign = IText::kAlignNear;
    IRECT textRect(mDrawRECT);
    const int textDist = mText.mSize / 5;
    textRect.L += textDist;
    textRect.R -= textDist;

    pGraphics->DrawIText(&iText, "Routing:", &textRect);
    iText.mAlign = IText::kAlignFar;
    pGraphics->DrawIText(&iText, RoutingTypeNames::routingNames[type], &textRect);
    return true;
}

void AnalyserRouting::AfterGUIResize(double guiScaleRatio)
{
    mText = IText(static_cast<int> (size * guiScaleRatio), &mText.mColor, mText.mFont, mText.mStyle, mText.mAlign, mText.mOrientation, mText.mQuality, &mText.mTextEntryBGColor, &mText.mTextEntryFGColor);
}

IPopupMenu* AnalyserRouting::createSelectionMenu()
{
    IPopupMenu* menu = new IPopupMenu();
    const int curIdx = static_cast<int> (qRange->getAnalyserRouting() + 0.5);

    for (int i = 1; i < kNumRoutings; ++i)
    {
        std::function<void()> callback = [this, i]() {
            qRange->setAnalyserRouting(static_cast<ERoutings>(i));
        };
        const int flag = i == curIdx ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
        menu->AddItem(new IPopupMenuItem(RoutingTypeNames::routingNames[i], flag, callback));
    }

    return menu;
}

void AnalyserRouting::setTextColor(IColor color)
{
    if (mText.mColor != color)
    {
        mText.mColor = color;
        SetDirty(false);
    }
}

void AnalyserRouting::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    std::unique_ptr<IPopupMenu> menu(createSelectionMenu());
    IRECT rect(mDrawRECT.Contains(x, y) ? mDrawRECT : IRECT(x, y, x, y));

    IPlugBase::ShowMenu(menu.get(), GetGUI(), rect);
}

void AnalyserRouting::OnMouseWheel(int /*x*/, int /*y*/, IMouseMod* /*pMod*/, int /*d*/)
{
}

