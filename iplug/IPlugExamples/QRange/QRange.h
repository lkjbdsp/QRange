#ifndef __QRANGE__
#define __QRANGE__

#include "IPlug_include_in_plug_hdr.h"
#include "src/FilterBandDsp.h"
#include "JuceIncludes.h"
#include "src/LinPhaseIIR.h"
#include "BandAnalyser/BandAnalyserView.h"
#include "PluginBypass.h"
#include "QRangeTypes.h"
#include "src/PresetManager.h"

#include "ebur128/EbuMeasure.h"

class FreqDisplay;
class QRange;
class QTextButton;
class QLabel;
class AnalyserRouting;
class PresetView;

class FilterBands : public LinPhaseIIR::Filter
{
public:

    void clearFilter() override
    {
        for (int i = 0; i < kNumBands; ++i)
            bands[i].clear();
    }

    void processFilter(double* samples, int numSamples) override
    {
        for (int i = 0; i < kNumBands; ++i)
            bands[i].processBlock(samples, numSamples);
    }

    void processFilter(double* samplesL, double* samplesR, int numSamples) override
    {
        for (int i = 0; i < kNumBands; ++i)
            bands[i].processBlock(samplesL, samplesR, numSamples);
    }

    void processBlock(double* outL, double* outR, const double* inL, const double* inR, int numSamples)
    {
        juce::FloatVectorOperations::copy(outL, inL, numSamples);
        
        if (inR != nullptr && outR != nullptr)
        {
            juce::FloatVectorOperations::copy(outR, inR, numSamples);
            processFilter(outL, outR, numSamples);
        }
        else
        {
            processFilter(outL, numSamples);
        }
    }

    void getResponse(FilterBandDsp::ResponseData& data, const float* sin2w2, int numPoints)
    {
        for (int i = 0; i < kNumBands; ++i)
            bands[i].getResponse(data, sin2w2, numPoints);
    }

private:

    FilterBandDsp bands[kNumBands];
    friend class QRange;
};

class QRange : public IPlug
{
public:
    QRange(IPlugInstanceInfo instanceInfo);
    ~QRange();

    void SetGUILayout(int viewMode, double windowWidth, double windowHeight) override;
    void OnGUIOpen() override;

    void SetBlockSize(int blockSize) override;
    void SetSampleRate(double sampleRate) override;
    void setLatencySelection(ELatencySelection newSelection);
    void updateLatency();
    void Reset() override;
    void OnParamChange(int paramIdx, ParamChangeFrom changeFrom) override;
    void updateParameters();
    void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;
    void AfterGUIResize(double guiScaleRatio) override;
    juce::XmlElement* getStateXml();
    void setStateXml(const juce::XmlElement* state);
    juce::XmlElement* getSettingsXml();
    void setSettingsXml(const juce::XmlElement* state);
    bool SerializeState(ByteChunk* pChunk) override;
    int UnserializeState(ByteChunk* pChunk, int startPos) override;

    void getFreqResponse(FilterBandDsp::ResponseData& data, int numPoints);
    bool usesRouting(ERoutings routing);

    void setSoloIndex(int idx);
    int getSoloIndex() const;
    void updateEnabledControls();

    FreqDisplay* getFreqDisplay();
    IControl* getControlForIndex(int idx);
    void setControlValue(int idx, double normalized);
    bool getAnalyserEnabled() const;
    bool getAnalyseInput() const;
    juce::Range<float> getAnalyserRange() const;
    void updateAnalyserButtons();
    BandAnalyser& getBandAnalyser();
    ERoutings getAnalyserRouting() const;
    ELatencySelection getLatencySelection() const;
    void setAnalyserRouting(ERoutings routing);
    IColor getColourForRouting(ERoutings routing) const;
    IColor getBandColor(int bandIdx) const;
    void setThickCurves(bool thick);
    bool getThickCurves() const;
    void setColorHints(bool show);
    bool getColorHints() const;

    IPopupMenu* getAnalyserMenu();
    IPopupMenu* getAnalyserRangeMenu(bool includeCaption);
    IPopupMenu* getLatencySettingsMenu(bool includeCaption);
    IPopupMenu* getRangeMenu(bool includeCaption);
    IPopupMenu* getDefaultsMenu();
    void matchOutputLoudness();
    bool isMatchPossible() const;

private:

    struct Defaults
    {
        Defaults();
        Defaults(QRange* qRange);
        juce::XmlElement* toXml() const;
        void fromXml(const juce::XmlElement* xml);

        struct Band
        {
            bool enabled = false;
            int type = 0;
            double freq = 0;
            double gain = 0;
            double q = 0.707;
            int cutOrder = 0;
            int routing = 0;
        };

        Band bands[kNumBands];
        ELatencySelection latencySelection = kLatencyLong;
        double outputGain = 0;
        bool bypass = false;
        bool analyserEnabled = true;
        bool analyseInput = true;
        float analyserMagStart = -54.f;
        float analyserMagRange = 60.f;
        float magRange = 24.f;
        bool thickCurves = false;
        bool colorHints = false;
    };

    struct Curves
    {
        Curves();
        void setSize(int size, double newSamplesRate);
        void resetCurves(int numPoints, double newSamplesRate);
        void convertToDb(int numPoints);
        int currentSize = 0;
        double sampleRate = 0;
        std::vector<float> normFreqs;
        std::vector<float> mid;
        std::vector<float> side;
        std::vector<float> left;
        std::vector<float> right;
    };

    void processAnalyser(const double* inL, const double* inR, int numSamples);
    void updateHideControls(int bandIndex);
    void sortBands();
    void setAnalyserEnable(bool enPre, bool enPost);
    juce::File getDefaultsFile() const;
    void writeDefaultsFile(const juce::XmlElement* xml) const;
    juce::XmlElement* getDefaultsXml() const;

    BandParameters bandParams[kNumBands];
    FilterBands filterBands;
    FilterBands filterBandsIIR;
    std::unique_ptr<LinPhaseIIR> forwardIIR;
    std::unique_ptr<LinPhaseIIR> reverseIIR;
    FreqDisplay* freqDisplay = nullptr;
    QTextButton* anaPreButton = nullptr;
    QTextButton* anaPostButton = nullptr;
    QTextButton* bypassButton = nullptr;
    QTextButton* matchButton = nullptr;
    QLabel* anaLabel = nullptr;
    AnalyserRouting* anaRouting = nullptr;

    SampleDelay<double> analyserDelay;

    EbuMeasure ebuIn;
    EbuMeasure ebuOut;
    PresetView* presetView;
    PresetManager presetManager;

    BandAnalyser bandAnalyser;
    std::unique_ptr<BandAnalyserView> analyserView;
    Curves curves;
    bool analyseInput = true;
    ERoutings analyserRouting = kRoutingMid;
    ELatencySelection latencySelection = kLatencyLong;
    double outputGain = 1.;
    bool processAnalyserWhenHidden = false;

    int soloIndex = -1;
    bool thickCurves = false;
    bool colorHints = false;

    PluginBypass bypassProc;
    Defaults defaults;
    static const int xmlChunkMagicByte;
};

#endif
