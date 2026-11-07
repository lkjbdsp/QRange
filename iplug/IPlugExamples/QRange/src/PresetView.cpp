#include "PresetView.h"

PresetView::PresetView(IPlugBase* pBase, IRECT r, PresetManager& m, int textsize)
    : IControl(pBase, r), manager(m), textSize(textsize)
{
    IColor c(0xCCCCCC);
    mText = IText(textSize, &c, "Verdana");
}

bool PresetView::Draw(IGraphics* pGraphics)
{
    IColor textOn(0xCCCCCC);
    IColor frameCol(0x777777);

    std::function<void(const char* , IRECT, Button )> drawButton = [this, pGraphics, textOn] (const char* text, IRECT rect, Button onState) {
        IColor textOff(0x333333);
        IColor bgOn(0xCCCCCC);
        IColor bgOff(0x444444);
        IColor bgOnMO(0xAAAAAA);
        IColor bgOffMO(0x333333);
        IColor colOn(mouseOver == onState ? bgOnMO : bgOn);
        IColor colOff(mouseOver == onState ? bgOffMO : bgOff);
        pGraphics->FillIRect(state == onState ? &colOn : &colOff, &rect);
        pGraphics->DrawRect(&bgOn, &rect);
        mText.mColor = state == onState ? textOff : textOn;
        mText.mAlign = IText::kAlignCenter;
        pGraphics->DrawIText(&mText, text, &rect);
    };

    drawButton("Load", loadRect, kLoad);
    drawButton("Save", saveRect, kSave);

    mText.mAlign = IText::kAlignNear;
    mText.mColor = textOn;
    pGraphics->DrawIText(&mText, "Presets", &textRect);

    return true;
}

void PresetView::AfterGUIResize(double scale)
{
    mText = IText(static_cast<int> (scale * textSize), &mText.mColor, "Verdana", IText::kStyleNormal, IText::kAlignCenter, 0);
    shownPresetName = currentPresetName;

    if (IGraphics* gui = GetGUI())
    {
        const int dist = mText.mSize / 3;
        IRECT measureRect;
        gui->DrawIText(&mText, "Save", &measureRect, true);
        const int wSave = measureRect.W() + 2 * dist;
        measureRect = { 0, 0, 0, 0 };
        gui->DrawIText(&mText, "Load", &measureRect, true);
        const int wLoad = measureRect.W() + 2 * dist;

        saveRect = IRECT(mDrawRECT.R - wSave, mDrawRECT.T, mDrawRECT.R, mDrawRECT.B);
        loadRect = IRECT(saveRect.L - wLoad - 1, mDrawRECT.T, saveRect.L - 1, mDrawRECT.B);
        textRect = IRECT(mDrawRECT.L, mDrawRECT.T, loadRect.L - dist, mDrawRECT.B);
    }
}

void PresetView::OnMouseOver(int x, int y, IMouseMod* pMod)
{
    const Button inButton = loadRect.Contains(x, y) ? kLoad : saveRect.Contains(x, y) ? kSave : kNone;

    if (mouseOver != inButton)
    {
        mouseOver = inButton;
        SetDirty(false);    
    }
}

void PresetView::OnMouseOut()
{
    if (mouseOver)
    {
        mouseOver = kNone;
        SetDirty(false);
    }
}

void PresetView::OnMouseDown(int x, int y, IMouseMod* /*pMod*/)
{
    const Button inButton = loadRect.Contains(x, y) ? kLoad : saveRect.Contains(x, y) ? kSave : kNone;

    if (state != inButton)
    {
        state = inButton;
        SetDirty(false);
    }
}

void PresetView::OnMouseUp(int x, int y, IMouseMod* /*pMod*/)
{
    if (state == kLoad)
    {
        showLoadMenu(x, y);
    }
    else if (state == kSave && onPresetSave != nullptr)
    {
        const juce::File presetDir(manager.getFolder());
        WDL_String filename(presetDir.getNonexistentChildFile("New Preset", ".xml").getFullPathName().toRawUTF8());
        WDL_String directory(presetDir.getFullPathName().toRawUTF8());
        GetGUI()->PromptForFile(&filename, kFileSave, &directory, "xml");

        const juce::String fn(filename.Get());
        const juce::String dir(directory.Get());

        if (fn.isNotEmpty())
        {
            juce::File target(juce::File(dir).getChildFile(fn));
            std::unique_ptr<juce::XmlElement> xml(onPresetSave(target.getFileNameWithoutExtension()));

            manager.savePreset(target, xml.get());
        }
    }

    state = kNone;
    SetDirty(false);
}

void PresetView::setOnPresetLoad(std::function<void(const juce::XmlElement*)> f)
{
    onPresetLoad = f;
}

void PresetView::setOnPresetSave(std::function<juce::XmlElement*(const juce::String& name)> f)
{
    onPresetSave = f;
}

void PresetView::showLoadMenu(int x, int y)
{
    manager.parse();
    std::unique_ptr<IPopupMenu> menu(createMenu(manager.getPresets()));

    IRECT rect(x, y, x, y);
    IPlugBase::ShowMenu(menu.get(), GetGUI(), rect);
}

IPopupMenu* PresetView::createMenu(const PresetManager::Preset& preset) const
{
    std::unique_ptr<IPopupMenu> menu(new IPopupMenu());

    for (auto p : preset.presets)
    {
        if (p.isFolder && p.name.isNotEmpty())
        {
            std::unique_ptr<IPopupMenu> sub(createMenu(p));
            if (sub->GetNItems() > 0)
                menu->AddItem(p.name.toRawUTF8(), sub.release());
        }
    }

    for (auto p : preset.presets)
    {
        if (p.isFolder || p.name.isEmpty())
            continue;

        std::function<void(void)> load = [this, p]() {
            std::unique_ptr<juce::XmlElement> xml(manager.loadPreset(p));
            if (onPresetLoad != nullptr)
                onPresetLoad(xml.get());
        };

        menu->AddItem(new IPopupMenuItem(p.name.toRawUTF8(), IPopupMenuItem::kNoFlags, load));
    }

    return menu.release();
}
