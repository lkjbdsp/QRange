#include "FilterType.h"
#include "../QRange.h"
#include "freqdisplay.h"

FilterType::FilterType(IPlugBase* pPlug, IRECT pR, int paramIdx)
    : QRangeIControl(pPlug, pR, paramIdx)
{
}

bool FilterType::Draw(IGraphics* pGraphics)
{
    if (const IParam* param = GetParam())
    {
        const int type = juce::jlimit(0, static_cast<int> (kNumFilterTypes), static_cast<int> (param->Value() + 0.5));

        const IColor col(0xDDDDDD, 0xFF);
        const int x = mDrawRECT.L;
        const int w = mDrawRECT.W();
        const int y = mDrawRECT.T;
        const int h = mDrawRECT.H();
        const float xc = x + 0.5f * w;
        const float yc = y + 0.5f * h;
        const float scX = 1 / 6.f;
        const float scY = 1 / 4.f;

        switch (type)
        {
        case kTypePeak:
            pGraphics->DrawLine(&col, x + scX * w, yc, x + 2 * scX * w, yc);
            pGraphics->DrawLine(&col, x + (1.f - 2 * scX) * w, yc, x + (1.f - scX) * w, yc);
            pGraphics->DrawLine(&col, x + 2 * scX * w, yc, xc, y + scY * h);
            pGraphics->DrawLine(&col, x + 2 * scX * w, yc, xc, y + (1.f - scY) * h);
            pGraphics->DrawLine(&col, xc, y + scY * h, x + (1.f - 2 * scX) * w, yc);
            pGraphics->DrawLine(&col, xc, y + (1.f - scY) * h, x + (1.f - 2 * scX) * w, yc);
            break;
        case kTypeLoShelf:
            pGraphics->DrawLine(&col, x + scX * w, y + scY * h, x + 2 * scX * w, y + scY * h);
            pGraphics->DrawLine(&col, x + scX * w, y + (1.f - scY) * h, x + 2 * scX * w, y + (1.f - scY) * h);
            pGraphics->DrawLine(&col, x + 2 * scX * w, y + scY * h, xc, yc);
            pGraphics->DrawLine(&col, x + 2 * scX * w, y + (1.f - scY) * h, xc, yc);
            pGraphics->DrawLine(&col, xc, yc, x + (1.f - scX) * w, yc);
            break;
        case kTypeHiShelf:
            pGraphics->DrawLine(&col, x + scX * w, yc, xc, yc);
            pGraphics->DrawLine(&col, xc, yc, x + (1.f - 2 * scX) * w, y + scY * h);
            pGraphics->DrawLine(&col, xc, yc, x + (1.f - 2 * scX) * w, y + (1.f - scY) * h);
            pGraphics->DrawLine(&col, x + (1.f - 2 * scX) * w, y + scY * h, x + (1.f - scX) * w, y + scY * h);
            pGraphics->DrawLine(&col, x + (1.f - 2 * scX) * w, y + (1.f - scY) * h, x + (1.f - scX) * w, y + (1.f - scY) * h);
            break;
        case kTypeLoCut:
            pGraphics->DrawLine(&col, xc - scY * h, yc + scY * h, xc, yc);
            pGraphics->DrawLine(&col, xc, yc, x + (1.f - scX) * w, yc);
            break;
        case kTypeHiCut:
            pGraphics->DrawLine(&col, x + scX * w, yc, xc, yc);
            pGraphics->DrawLine(&col, xc + scY * h, yc + scY * h, xc, yc);
            break;
        default:
            jassertfalse;
            return false;
        }
    }

    return true;
}

void FilterType::AfterGUIResize(double guiScaleRatio)
{
}

IPopupMenu* FilterType::createSelectionMenu()
{
    if (IParam* param = GetParam())
    {
        IPopupMenu* menu = new IPopupMenu();
        const int curIdx = param->Int();

        for (int i = 0; i < kNumFilterTypes; ++i)
        {
            std::function<void()> callback = [this, i]() { this->menuCallback(i); };
            const int flag = i == curIdx ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
            menu->AddItem(new IPopupMenuItem(FilterTypeNames::typeNames[i], flag, callback));
        }

        return menu;
    }

    jassertfalse;
    return nullptr;
}

void FilterType::OnMouseDown(int x, int y, IMouseMod* /*pMod*/)
{
    if (IParam* param = GetParam())
    {
        std::unique_ptr<IPopupMenu> menu(createSelectionMenu());
        IRECT rect(mDrawRECT.Contains(x, y) ? mDrawRECT : IRECT(x, y, x, y));
        IPlugBase::ShowMenu(menu.get(), GetGUI(), rect);
    }
}

void FilterType::OnMouseWheel(int /*x*/, int /*y*/, IMouseMod* pMod, int d)
{
    if (IParam* param = GetParam())
    {
        mValue = param->GetNormalized(param->Value() + d);
        SetDirty();
        setFreqDisplayDirty();
    }
}

QRange* FilterType::getQRange()
{
    return dynamic_cast<QRange*> (GetPlug());
}

void FilterType::setFreqDisplayDirty()
{
    if (QRange* qRange = getQRange())
        qRange->getFreqDisplay()->triggerUpdateCurves();
}

void FilterType::menuCallback(int value)
{
    if (IParam* param = GetParam())
    {
        mValue = param->GetNormalized(value);
        SetDirty();
        setFreqDisplayDirty();
    }
}
