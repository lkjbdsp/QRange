#pragma once

#include "../JuceIncludes.h"

class PresetManager
{
public:

    struct Preset
    {
        Preset(PresetManager* manager, const juce::String& initFolder, const juce::String& initName, bool isfolder)
            : presetManager(manager), folder(initFolder), name(initName), isFolder(isfolder)
        {
        }
        PresetManager* presetManager;
        juce::String folder;
        juce::String name;
        std::vector<Preset> presets;
        bool isFolder;

        juce::File getFile() const
        {
            juce::File f(presetManager->getFolder().getChildFile(folder.isEmpty() ? name : folder + "/" + name));
            return isFolder ? f : f.withFileExtension(".xml");
        }
    };

    PresetManager(const juce::String& pluginName)
        : presetFolder(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("lkjb").getChildFile(pluginName)), presets(this, "", "", true)
    {
        jassert(pluginName.isNotEmpty());
        presetFolder.createDirectory();
    }

    void setDefaultPresets(const char* data, int size)
    {
        juce::MemoryInputStream input(data, static_cast<size_t> (size), false);
        juce::ZipFile zip(&input, false);

        zip.uncompressTo(presetFolder, false);
    }

    void parse()
    {
        presets.presets.clear();
        parseFolder(presets, "");
    }

    juce::XmlElement* loadPreset(const Preset& preset)
    {
        jassert(! preset.isFolder);
        if (preset.name.isEmpty() || preset.isFolder)
            return nullptr;

        const juce::File f(preset.getFile());

        juce::XmlElement* xml = juce::XmlDocument::parse(f);
        return xml;
    }

    void savePreset(const juce::File& targetFile, const juce::XmlElement* xml)
    {
        if (xml != nullptr && ! targetFile.isDirectory())
        {
            xml->writeToFile(targetFile, "");
        }
    }

    const Preset& getPresets()
    {
        return presets;
    }

    juce::File getFolder() const
    {
        return presetFolder;
    }

private:

    void parseFolder(Preset& target, const juce::String& subFolder)
    {
        ;
        const juce::File folder(subFolder.isEmpty() ? presetFolder : presetFolder.getChildFile(subFolder));
        const juce::Array<juce::File> folders(folder.findChildFiles(juce::File::findDirectories, false));

        for (auto f : folders)
        {
            const juce::String name(f.getFileName());
            const juce::String newFolder(subFolder.isEmpty() ? name : subFolder + "/" + name);
            Preset p(this, subFolder, name, true);
            parseFolder(p, newFolder);
            target.presets.push_back(p);
        }

        const juce::Array<juce::File> files(folder.findChildFiles(juce::File::findFiles, false, "*.xml"));

        for (auto f : files)
        {
            const juce::String name(f.getFileNameWithoutExtension());
            Preset p(this, subFolder, name, false);
            target.presets.push_back(p);
        }
    }


    juce::File presetFolder;
    Preset presets;

};
