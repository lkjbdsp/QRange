#pragma once

#include "IPlugGUIResize.h"
#include "PresetManager.h"

class PresetView : public IControl
{
public:

    PresetView(IPlugBase* pBase, IRECT r, PresetManager& m, int textsize);

    bool Draw(IGraphics* pGraphics) override;
    void AfterGUIResize(double scale) override;

    void OnMouseOver(int x, int y, IMouseMod* pMod) override;
    void OnMouseOut() override;
    void OnMouseDown(int x, int y, IMouseMod* pMod) override;
    void OnMouseUp(int x, int y, IMouseMod* pMod) override;

    void setOnPresetLoad(std::function<void(const juce::XmlElement*)> f);
    void setOnPresetSave(std::function<juce::XmlElement*(const juce::String& name)> f);

private:

    void showLoadMenu(int x, int y);
    IPopupMenu* createMenu(const PresetManager::Preset& preset) const;

    enum Button
    {
        kNone,
        kLoad,
        kSave
    };

    PresetManager& manager;
    int textSize;
    juce::String currentPresetName;
    juce::String shownPresetName;
    IRECT textRect;
    IRECT loadRect;
    IRECT saveRect;
    std::function<void(const juce::XmlElement*)> onPresetLoad;
    std::function<juce::XmlElement*(const juce::String& name)> onPresetSave;

    Button mouseOver = kNone;
    Button state = kNone;
};
