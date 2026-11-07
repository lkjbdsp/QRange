#include "QRange.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"
#include "src/filterbandgroup.h"
#include "src/freqdisplay.h"
#include "src/QTextButton.h"
#include "src/BandSorter.h"
#include "src/PresetView.h"
#include "resources/PresetData.h"

const int QRange::xmlChunkMagicByte = 0x6C6B6A62;

class SettingsIcon : public IControl
{
public:

    SettingsIcon(QRange* plug, IRECT r)
        : IControl(qRange, r), qRange(plug)
    {
        bitmap = qRange->GetGUI()->LoadPointerToBitmap(SETTINGSICON_ID, SETTINGSICON_FN, 1);
    }

    bool Draw(IGraphics* pGraphics) override
    {
        pGraphics->DrawBitmap(bitmap, &mDrawRECT);
        return true;
    }

    void OnMouseDown(int x, int y, IMouseMod* /*pMod*/) override
    {
        std::unique_ptr<IPopupMenu> menu(new IPopupMenu());
        std::function<void(void)> toggleThick = [this]() {
            qRange->setThickCurves(! qRange->getThickCurves());
        };
        std::function<void(void)> toggleHint = [this]() {
            qRange->setColorHints(! qRange->getColorHints());
        };


        const int thickFlags = qRange->getThickCurves() ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
        const int hintFlags = qRange->getColorHints() ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;

        menu->AddItem(juce::String("lkjb QRange Version " + juce::String(VST3_VER_STR)).toRawUTF8(), -1, IPopupMenuItem::kDisabled);
        menu->AddSeparator();

        IPopupMenu* eq = new IPopupMenu();
        eq->AddItem(new IPopupMenuItem("Thick EQ Curves", thickFlags, toggleThick));
        eq->AddItem(new IPopupMenuItem("Band Color Hints", hintFlags, toggleHint));

        menu->AddItem("EQ Bands", eq);
        menu->AddItem("Analyser", qRange->getAnalyserMenu());
        menu->AddItem("Display Magnitude Range", qRange->getRangeMenu(false));
        menu->AddItem("Process Mode", qRange->getProcessModeMenu(false));
        menu->AddItem("Latency Setting", qRange->getLatencySettingsMenu(false));
        menu->AddItem("Defaults", qRange->getDefaultsMenu());

        IRECT menuRect(x, y, x, y);
        IPlugBase::ShowMenu(menu.get(), qRange->GetGUI(), menuRect);
    }

private:

    QRange* qRange;
    IBitmap* bitmap;
};

QRange::Curves::Curves()
{
    setSize(1000, 44100);
}

void QRange::Curves::setSize(int size, double newSamplesRate)
{
    if (size > mid.size())
    {
        normFreqs.resize(size);
        mid.resize(size);
        side.resize(size);
        left.resize(size);
        right.resize(size);
    }

    if (currentSize != size || sampleRate != newSamplesRate)
    {
        currentSize = size;
        for (int i = 0; i < currentSize; ++i)
            normFreqs[i] = (20.f * std::pow(10.f, 3.f * i / (currentSize - 1.f))) / static_cast<float> (sampleRate);

        sampleRate = newSamplesRate;
    }
}

void QRange::Curves::resetCurves(int numPoints, double newSamplesRate)
{
    setSize(numPoints, newSamplesRate);
    juce::FloatVectorOperations::fill(mid.data(), 1.f, numPoints);
    juce::FloatVectorOperations::fill(side.data(), 1.f, numPoints);
    juce::FloatVectorOperations::fill(left.data(), 1.f, numPoints);
    juce::FloatVectorOperations::fill(right.data(), 1.f, numPoints);
}

void QRange::Curves::convertToDb(int numPoints)
{
    std::function<void(float*, int)> batchToDb = [](float* data, int N) {
        for (int i = 0; i < N; ++i)
            data[i] = 2.f * 20.f * std::log10(data[i]);
    };

    batchToDb(mid.data(), numPoints);
    batchToDb(side.data(), numPoints);
    batchToDb(left.data(), numPoints);
    batchToDb(right.data(), numPoints);
}

QRange::Defaults::Defaults()
{
    const double defaultFreqs[kNumBands] = { 20., 37., 70., 133., 250., 465., 865., 1600., 3000., 5700., 10e3, 20e3 };

    for (int i = 0; i < kNumBands; ++i)
        bands[i].freq = defaultFreqs[i];
}

QRange::Defaults::Defaults(QRange* qRange)
{
    for (int i = 0; i < kNumBands; ++i)
    {
        const int offset = i * kNumParametersPerBand;

        bands[i].enabled = qRange->GetParam(offset + kEnabled)->Bool();
        bands[i].type = qRange->GetParam(offset + kType)->Int();
        bands[i].freq = qRange->GetParam(offset + kFreq)->Value();
        bands[i].gain = qRange->GetParam(offset + kGain)->Value();
        bands[i].q = qRange->GetParam(offset + kQ)->Value();
        bands[i].cutOrder = qRange->GetParam(offset + kCutOrder)->Int();
        bands[i].routing = qRange->GetParam(offset + kRouting)->Int();
    }

    latencySelection = qRange->getLatencySelection();
    outputGain = qRange->GetParam(kOutputGain)->Value();
    bypass = qRange->GetParam(kBypass)->Bool();
    analyserEnabled = qRange->getAnalyserEnabled();
    analyseInput = qRange->getAnalyseInput();;
    analyserMagStart = qRange->getAnalyserRange().getStart();
    analyserMagRange = qRange->getAnalyserRange().getLength();
    magRange = qRange->getFreqDisplay()->getMagRange();
    thickCurves = qRange->getThickCurves();
    colorHints = qRange->getColorHints();
    procMode = qRange->getProcMode();
}

juce::XmlElement* QRange::Defaults::toXml() const
{
    juce::XmlElement* xml(new juce::XmlElement("DEFAULT_STATE"));

    for (int i = 0; i < kNumBands; ++i)
    {
        const juce::String bandName("band" + juce::String(i + 1).paddedLeft('0', 2));

        xml->setAttribute(bandName + "enabled", bands[i].enabled);
        xml->setAttribute(bandName + "type", bands[i].type);
        xml->setAttribute(bandName + "freq", bands[i].freq);
        xml->setAttribute(bandName + "gain", bands[i].gain);
        xml->setAttribute(bandName + "q", bands[i].q);
        xml->setAttribute(bandName + "cutorder", bands[i].cutOrder);
        xml->setAttribute(bandName + "routing", bands[i].routing);
    }
    xml->setAttribute("outgain", outputGain);
    xml->setAttribute("bypass", bypass);
    xml->setAttribute("analyserenabled", analyserEnabled);
    xml->setAttribute("analyseinput", analyseInput);
    xml->setAttribute("analysermagstart", analyserMagStart);
    xml->setAttribute("analysermagrang", analyserMagRange);
    xml->setAttribute("magrange", magRange);
    xml->setAttribute("latencyselection", latencySelection);
    xml->setAttribute("thickcurves", thickCurves);
    xml->setAttribute("colorhints", colorHints);
    xml->setAttribute("processmode", procMode);
    return xml;
}

void QRange::Defaults::fromXml(const juce::XmlElement* xml)
{
    if (xml != nullptr && ! xml->hasTagName("DEFAULT_STATE"))
        xml = xml->getChildByName("DEFAULT_STATE");

    if (xml == nullptr)
        return;

    for (int i = 0; i < kNumBands; ++i)
    {
        const juce::String bandName("band" + juce::String(i + 1).paddedLeft('0', 2));

        bands[i].enabled = xml->getBoolAttribute(bandName + "enabled", bands[i].enabled);
        bands[i].type = xml->getIntAttribute(bandName + "type", bands[i].type);
        bands[i].freq = xml->getDoubleAttribute(bandName + "freq", bands[i].freq);
        bands[i].gain = xml->getDoubleAttribute(bandName + "gain", bands[i].gain);
        bands[i].q = xml->getDoubleAttribute(bandName + "q", bands[i].q);
        bands[i].cutOrder = xml->getIntAttribute(bandName + "cutorder", bands[i].cutOrder);
        bands[i].routing = xml->getIntAttribute(bandName + "routing", bands[i].routing);
    }

    outputGain = xml->getDoubleAttribute("outgain", outputGain);
    bypass = xml->getBoolAttribute("bypass", bypass);
    analyserEnabled = xml->getBoolAttribute("analyserenabled", analyserEnabled);
    analyseInput = xml->getBoolAttribute("analyseinput", analyseInput);
    analyserMagStart = static_cast<float> (xml->getDoubleAttribute("analysermagstart", analyserMagStart));
    analyserMagRange = static_cast<float> (xml->getDoubleAttribute("analysermagrang", analyserMagRange));
    magRange = static_cast<float> (xml->getDoubleAttribute("magrange", magRange));
    latencySelection = static_cast<ELatencySelection> (xml->getIntAttribute("latencyselection", latencySelection));
    thickCurves = xml->getBoolAttribute("thickcurves", thickCurves);
    colorHints = xml->getBoolAttribute("colorhints", colorHints);
    procMode = static_cast<LinPhaseIIR::ProcMode> (xml->getIntAttribute("processmode", LinPhaseIIR::kWindowPost));
}

enum ELayout
{
    kWidth = GUI_WIDTH,
    kHeight = GUI_HEIGHT
};

enum viewSets
{
	defaultView
};

QRange::QRange(IPlugInstanceInfo instanceInfo)
    : IPLUG_CTOR(kNumPublicParams, kNumPrivateParams, kNumPrograms, instanceInfo),
    presetManager("QRange")
{
    TRACE;
    forwardIIR.reset(new LinPhaseIIR(filterBands));
    reverseIIR.reset(new LinPhaseIIR(filterBands));
    std::unique_ptr<juce::XmlElement> xmlDefs(juce::XmlDocument::parse(getDefaultsFile()));

    presetManager.setDefaultPresets(PresetData::lkjbpresets_zip, PresetData::lkjbpresets_zipSize);

    if (xmlDefs != nullptr)
        defaults.fromXml(xmlDefs.get());


    thickCurves = defaults.thickCurves;
    colorHints = defaults.colorHints;

	for (int i = 0; i < kNumBands; ++i)
	{
		const int offset = i * kNumParametersPerBand;
		const juce::String bandName("Band" + juce::String(i + 1).paddedLeft('0', 2));

		GetParam(offset + kEnabled)->InitBool(juce::String(bandName + "Enabled").toRawUTF8(), defaults.bands[i].enabled);
		GetParam(offset + kEnabled)->SetDisplayText(0, "Off");
		GetParam(offset + kEnabled)->SetDisplayText(1, "On");

		GetParam(offset + kType)->InitEnum(juce::String(bandName + "Type").toRawUTF8(), defaults.bands[i].type, kNumFilterTypes);
		for (int i = 0; i < kNumFilterTypes; ++i)
			GetParam(offset + kType)->SetDisplayText(i, FilterTypeNames::typeNames[i]);

		GetParam(offset + kFreq)->InitDouble(juce::String(bandName + "Freq").toRawUTF8(), defaults.bands[i].freq, 20., 20e3, 0.1, "", "", -1);
		GetParam(offset + kGain)->InitDouble(juce::String(bandName + "Gain").toRawUTF8(), defaults.bands[i].gain, -24, 24, 0.1);
		GetParam(offset + kQ)->InitDouble(juce::String(bandName + "Q").toRawUTF8(), defaults.bands[i].q, 0.3, 16, 0.001, "", "", -1);
        GetParam(offset + kCutOrder)->InitEnum(juce::String(bandName + "Order").toRawUTF8(), defaults.bands[i].cutOrder, kNumCutOrders);

		GetParam(offset + kRouting)->InitEnum(juce::String(bandName + "Routing").toRawUTF8(), defaults.bands[i].routing, kNumRoutings);
		for (int i = 0; i < kNumRoutings; ++i)
			GetParam(offset + kRouting)->SetDisplayText(i, RoutingTypeNames::routingNames[i]);
	}

    GetParam(kOutputGain)->InitDouble("OutGain", defaults.outputGain, -24., 24., 0.1, "dB");
    GetParam(kBypass)->InitBool("Bypass", defaults.bypass);

    IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
 
    AttachGUIResize(new IPlugGUIResize(this, pGraphics, true, 16, 16));

    GetGUIResize()->UsingBitmaps();

    GetGUIResize()->SelectViewMode(defaultView);

    GetGUIResize()->SetGUIScaleLimits(50, 150);
    GetGUIResize()->UseHandleForGUIScaling(true);

    GetGUIResize()->SetWindowSizeLimits(defaultView, GUI_WIDTH, GUI_HEIGHT, GUI_WIDTH, GUI_HEIGHT);

    pGraphics->SetBitmapOversample(1);

    IColor bgColor(0xFF, 0x66, 0x33, 0x00);
    pGraphics->AttachBackground(BACKGROUND_ID, BACKGROUND_FN);

    IColor textColor = IColor(255, 0, 0, 0);
    IText textProps(24, &textColor, "Arial", IText::kStyleNormal, IText::kAlignCenter, 0, IText::kQualityDefault);
    const IRECT freqDispRect(0, 78, GUI_WIDTH, 410);

    analyserView.reset(new BandAnalyserView(bandAnalyser));
    pGraphics->AttachControl(freqDisplay = new FreqDisplay(this, freqDispRect));

    {
        std::function<float(float)> freqToX = [this](float freq) {
            return freqDisplay->getPosFromFreq(freq);
        };
        analyserView->setFreqMapping(freqToX, nullptr);
        const juce::Range<float> magPosRange(freqDisplay->getMagPositionRange());
        analyserView->setMagnitudePositions(magPosRange.getStart(), magPosRange.getEnd());
        freqDisplay->setBandAnalyserView(analyserView.get());

        analyserView->setMagRange(juce::Range<float>(defaults.analyserMagStart, defaults.analyserMagStart + defaults.analyserMagRange));
        bandAnalyser.setEnabled(defaults.analyserEnabled);
        analyseInput = defaults.analyseInput;
    }

    {
        const int textSize = 16;
        const int groupDist = 15;
        const int bandDist = (FilterBandGroup::groupWidth + groupDist);
        const int x0 = (GUI_WIDTH - (kNumBands * FilterBandGroup::groupWidth + (kNumBands - 1) * groupDist)) / 2;

        for (int i = 0; i < kNumBands; ++i)
        {
            FilterBandGroup* band = new FilterBandGroup(this, x0 + i * bandDist, 440, i);
            pGraphics->AttachControl(band);
            band->attachControls(pGraphics, bandParams[i]);
        }

        {
            const IRECT verRect(310, 43, 550, 63);
            IColor bgCol(0, 0);
            IColor tCol(0x888888);
            QLabel* l = new QLabel(this, verRect, juce::String(juce::String("V") + VST3_VER_STR).toRawUTF8(), textSize, tCol, bgCol);
            l->setAlignment(IText::kAlignNear);
            pGraphics->AttachControl(l);
        }

        {
            const int wBut = 55;
            const int hBut = 20;

            std::function<void(bool)> callbackPre = [this](bool state) {
                setAnalyserEnable(state, false);
            };
            std::function<void(bool)> callbackPost = [this](bool state) {
                setAnalyserEnable(false, state);
            };
            std::function<void(void)> callbackMatch = [this]() {
                matchOutputLoudness();
            };
            std::function<bool(void)> callbackMatchDirty = [this]() {
                const bool curEn = ! matchButton->IsGrayed();
                const bool supEn = isMatchPossible();
                if (curEn != supEn)
                    matchButton->GrayOut(! supEn);
                return curEn != supEn;
            };

            const IColor textColor(0xCCCCCC);
            const IColor bgCol(0x333333, 0x77);
            const int y0 = 58;
            const int yBut = y0 - hBut;
            const int yLab = yBut - hBut;
            const int xR = 925;
            const int wGain = FilterBandGroup::groupWidth - 6;
            const int xMatch = xR - wGain;
            const int xGain = xMatch - wGain;
            const int hGain = 20;
            const int xPost = xGain - wBut;
            const int xPre = xPost - wBut - 1;
            IRECT gainRect(xGain, y0 - hGain, xGain+wGain, y0);
            TextSlider* outGain = new TextSlider(this, gainRect, kOutputGain);
            outGain->isGain = true;
            outGain->setTextSize(13);
            IRECT gainLabRect(xGain, yLab, xGain+wGain, yLab + hGain);
            pGraphics->AttachControl(outGain);
            pGraphics->AttachControl(new QLabel(this, gainLabRect, "Gain", textSize, textColor, bgCol));

            anaPreButton = new QTextButton(this, xPre, yBut, wBut, hBut - 1, -1);
            anaPostButton = new QTextButton(this, xPost, yBut, wBut, hBut - 1, -1);
            bypassButton = new QTextButton(this, xMatch, yLab, wBut, hBut - 1, kBypass);
            matchButton = new QTextButton(this, xMatch, yBut, wBut, hBut - 1, -1);

            anaPreButton->setButtonText("Pre", textSize, IColor(0x000000), textColor);
            anaPostButton->setButtonText("Post", textSize, IColor(0x000000), textColor);
            bypassButton->setButtonText("Bypass", textSize, IColor(0x000000), textColor);
            matchButton->setButtonText("Match", textSize, IColor(0x000000), textColor);
            anaPreButton->setToggleCallback(callbackPre);
            anaPostButton->setToggleCallback(callbackPost);
            matchButton->setCallback(callbackMatch);
            matchButton->setDirtyCallback(callbackMatchDirty);

            pGraphics->AttachControl(anaPreButton);
            pGraphics->AttachControl(anaPostButton);
            pGraphics->AttachControl(bypassButton);
            pGraphics->AttachControl(matchButton);

            IRECT labelRect(xPre, yBut - hBut, xGain, yBut);
            pGraphics->AttachControl(anaLabel = new QLabel(this, labelRect, "Analyser", textSize, textColor, bgCol));

            IRECT routingRect(xPre, yBut + hBut, xGain, yBut + 2 * hBut);
            pGraphics->AttachControl(anaRouting = new AnalyserRouting(this, routingRect, textSize, textColor, bgCol));

            const int wSettings = 30;
            const int hSettings = 30;
            IRECT settingsRect(xPre - wSettings - 10, yBut + hBut / 2 - hSettings / 2, xPre - 10, yBut + hBut / 2 + hSettings / 2);
            pGraphics->AttachControl(new SettingsIcon(this, settingsRect));

            const int hButWide = hBut - 1;
            const int xSortBut = xGain;
            const int wButWide = xR - xSortBut;
            const int ySortBut = yBut + hBut;
            std::function<void()> sort = [this]() { sortBands(); };

            QTextButton* sortButton = new QTextButton(this, xSortBut, ySortBut, wButWide, hButWide, -1);
            sortButton->setButtonText("Sort Bands", textSize, IColor(0x000000), IColor(0xCCCCCC));
            sortButton->setCallback(sort);
            pGraphics->AttachControl(sortButton);

            std::function<juce::XmlElement*(const juce::String&)> onPresetSave = [this](const juce::String& name) {
                juce::XmlElement* xml = getStateXml();
                xml->setAttribute("presetname", name);
                return xml;
            };

            std::function<void(const juce::XmlElement*)> onPresetLoad = [this](const juce::XmlElement* xml) {
                setStateXml(xml);
            };

            IRECT presetRect(430, y0 - hBut + 6, 580, y0 + 5);
            presetView = new PresetView(this, presetRect, presetManager, textSize);
            presetView->setOnPresetSave(onPresetSave);
            presetView->setOnPresetLoad(onPresetLoad);
            pGraphics->AttachControl(presetView);
        }
    }

    AttachGraphics(pGraphics);

    if (xmlDefs != nullptr)
    {
        freqDisplay->setMagRange(defaults.magRange);

        if (IPlugGUIResize* resize = xmlDefs->hasAttribute("size") ? GetGUIResize() : nullptr)
        {
            resize->SetGUIScaleRatio(xmlDefs->getDoubleAttribute("size"));
            resize->UpdateDrawRects();
            pGraphics->RescaleBitmaps(resize->GetGUIScaleRatio());
            resize->ResizeGraphics();
        }
    }

    pGraphics->HandleMouseOver(true);

    latencySelection = defaults.latencySelection;
    MakeDefaultPreset((char *) "-", kNumPrograms);
}

QRange::~QRange()
{
    freqDisplay->setBandAnalyserView(nullptr);
}

void QRange::SetGUILayout(int /*viewMode*/, double /*windowWidth*/, double /*windowHeight*/)
{
}

void QRange::OnGUIOpen()
{
    IPlugBase::OnGUIOpen();

    if (! processAnalyserWhenHidden)
        bandAnalyser.clear();

    for (int i = 0; i < kNumBands; ++i)
        updateHideControls(i);
}

void QRange::updateParameters()
{
    const int soloIdxEn = soloIndex < 0 ? -1 : (soloIndex - kEnabled) / kNumParametersPerBand;
    const bool iir = latencySelection == kLatencyIIR;

    for (int i = 0; i < kNumBands; ++i)
    {
        const int offset = i * kNumParametersPerBand;
        const bool enabled = soloIdxEn < 0 ? GetParam(offset + kEnabled)->Bool() : i == soloIdxEn;
        const double freq = GetParam(offset + kFreq)->Value();
        double qFactor = GetParam(offset + kQ)->Value();
        const double gain = 0.5 * GetParam(offset + kGain)->Value();
        const EFilterTypes type = static_cast<EFilterTypes> (GetParam(offset + kType)->Int());
        const ERoutings routing = static_cast<ERoutings> (GetParam(offset + kRouting)->Int());
        const ECutOrders cutOrder = static_cast<ECutOrders> (GetParam(offset + kCutOrder)->Int());

        if (type != kTypePeak && qFactor > 1)
            qFactor = std::sqrt(qFactor);

        filterBands.bands[i].setFilter(enabled, freq, qFactor, gain, type, routing, cutOrder, mSampleRate);

        if (iir)
            filterBandsIIR.bands[i].setFilter(enabled, freq, qFactor, gain, type, routing, cutOrder, mSampleRate);
    }

    outputGain = std::pow(10., 0.05 * GetParam(kOutputGain)->Value());
    bypassProc.setBypass(GetParam(kBypass)->Bool());
}

void QRange::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
    SimdBandFilter::ScopedCSR csr;
    updateParameters();
    const bool iir = latencySelection == kLatencyIIR;

    if (GetSampleRate() < 44099)
      return;
  
    // Mutex is already locked for us.
    double* inL = inputs[0];
    double* inR = IsInChannelConnected(1) ? inputs[1] : nullptr;
    double* outL = outputs[0];
    double* outR = IsOutChannelConnected(1) ? outputs[1] : nullptr;
    
    if (analyseInput)
        processAnalyser(inL, inR, nFrames);

    ebuIn.processBlock(inL, inR, nFrames);

    bypassProc.processBlockPre(inL, inR, nFrames);

    if (iir)
    {
        filterBands.processBlock(outL, outR, inL, inR, nFrames);
        filterBandsIIR.processBlock(outL, outR, outL, outR, nFrames);
    }
    else
    {
        reverseIIR->processBlockReversed(outL, outR, inL, inR, nFrames);
        forwardIIR->processBlockForward(outL, outR, outL, outR, nFrames);
    }

    ebuOut.processBlock(outL, outR, nFrames);

    if (outputGain != 1)
    {
        juce::FloatVectorOperations::multiply(outL, outputGain, nFrames);

        if (outR != outL && outR != nullptr)
            juce::FloatVectorOperations::multiply(outR, outputGain, nFrames);
    }

    bypassProc.processBlockPost(outL, outR, outL, outR, nFrames);

    if (! analyseInput)
        processAnalyser(outL, outR, nFrames);
}

void QRange::SetBlockSize(int blockSize)
{
    WDL_MutexLock lock(&mMutex);

    IPlug::SetBlockSize(blockSize);
    bandAnalyser.setBlockSize(blockSize);
    bypassProc.setBlockSize(blockSize);
}

void QRange::SetSampleRate(double sampleRate)
{
    IPlug::SetSampleRate(sampleRate);

    bypassProc.setSampleRate(sampleRate);
    ebuIn.setSampleRate(sampleRate);
    ebuOut.setSampleRate(sampleRate);

    updateLatency();

    {
        WDL_MutexLock lock(&mMutex);

        const int numBands = 80;
        const int order = 5;
        bandAnalyser.setup(numBands, 20., 20e3, sampleRate, order);
        analyserView->updateNumCurvePoints();
    }
}

void QRange::setLatencySelection(ELatencySelection newSelection)
{
    if (newSelection != latencySelection)
    {
        latencySelection = newSelection;
        updateLatency();
    }
}

void QRange::updateLatency()
{
    WDL_MutexLock lock(&mMutex);

    const double sampleRate = GetSampleRate();
    const bool iir = latencySelection == kLatencyIIR;

    if (! iir)
    {
        const int defBlockSize = latencySelection == kLatencyShort ? 2048
                               : latencySelection == kLatencyMedium ? 4096
                               : latencySelection == kLatencyLong ? 8192
                               : 16384;

        const int blockSize = defBlockSize * juce::jmax(1, static_cast<int> (sampleRate / 44100));
        forwardIIR->setBlockSize(blockSize);
        reverseIIR->setBlockSize(blockSize);
    }

    const int latency = iir ? 0 : forwardIIR->getLatencySamples();
    SetLatency(latency);
    analyserDelay.setSize(latency);
    bypassProc.setLatency(latency);
    ebuIn.setDelay(latency);
    Reset();
}

void QRange::Reset()
{
    TRACE;
    IMutexLock lock(this);
    filterBands.clearFilter();

    if (latencySelection == kLatencyIIR)
        filterBandsIIR.clearFilter();

    if (reverseIIR != nullptr)
        reverseIIR->clear();

    if (forwardIIR != nullptr)
        forwardIIR->clear();

    bandAnalyser.clear();
}

void QRange::OnParamChange(int paramIdx, ParamChangeFrom changeFrom)
{
    IMutexLock lock(this);

    if (paramIdx < kBandParametersEnd)
    {
        const int bandIdx = paramIdx / kNumParametersPerBand;
        jassert(bandIdx >= 0 && bandIdx < kNumBands);
        const int bandParam = paramIdx - (bandIdx * kNumParametersPerBand);
        jassert(bandParam >= 0 && bandParam < kNumParametersPerBand);

        if (bandParam == kType || bandParam == kCutOrder)
            updateHideControls(bandIdx);

        if (freqDisplay != nullptr)
            freqDisplay->triggerUpdateCurves();

        GetGUI()->SetGlobalDirty();
    }
}

void QRange::AfterGUIResize(double guiScaleRatio)
{
    for (int i = 0; i < kNumBands; ++i)
        updateHideControls(i);

    if (analyserView != nullptr && freqDisplay != nullptr)
    {
        const juce::Range<float> magPosRange(freqDisplay->getMagPositionRange());
        analyserView->setMagnitudePositions(magPosRange.getStart(), magPosRange.getEnd());
    }
}

juce::XmlElement* QRange::getStateXml()
{
    juce::XmlElement* state(new juce::XmlElement("QRANGE"));
    state->setAttribute("version", VST3_VER_STR);
    state->setAttribute("latencyselection", latencySelection);
    state->setAttribute("processmode", getProcMode());
    
    for (int i = 0; i < NParams(); ++i)
    {
        IParam* param = GetParam(i);
        const juce::String name(param->GetNameForHost());
        state->setAttribute(name.toLowerCase().replace(" ", "_"), param->Value());
    }

    return state;
}

void QRange::setStateXml(const juce::XmlElement* state)
{
    if (state == nullptr || ! state->hasTagName("QRANGE"))
        return;

    for (int i = 0; i < NParams(); ++i)
    {
        IParam* param = GetParam(i);
        const juce::String name(param->GetNameForHost());
        const juce::String att(name.toLowerCase().replace(" ", "_"));

        if (state->hasAttribute(att))
            param->Set(state->getDoubleAttribute(att));
    }

    if (state->hasAttribute("latencyselection"))
    {
        const bool oldVersion = ! state->hasAttribute("version");
        ELatencySelection newLatSel = oldVersion ? kLatencyLong : static_cast<ELatencySelection> (state->getIntAttribute("latencyselection"));
        setLatencySelection(newLatSel);
    }

    {
        const auto procMode = static_cast<LinPhaseIIR::ProcMode> (state->getIntAttribute("processmode", LinPhaseIIR::kWindowPost));
        setProcMode(procMode);
    }

    OnParamReset();
}

juce::XmlElement* QRange::getSettingsXml()
{
    juce::XmlElement* state = new juce::XmlElement("SETTINGS");

    if (IPlugGUIResize* resize = GetGUIResize())
        state->setAttribute("size", resize->GetGUIScaleRatio());

    state->setAttribute("analyserenabled", bandAnalyser.getEnabled());
    state->setAttribute("analyseinput", analyseInput);
    state->setAttribute("analyserrangestart", analyserView->getMagRange().getStart());
    state->setAttribute("analyserrangeend", analyserView->getMagRange().getEnd());
    state->setAttribute("analyserrouting", static_cast<int> (analyserRouting));
    state->setAttribute("magrange", freqDisplay->getMagRange());
    state->setAttribute("showpeak", analyserView->getShowPeak());
    state->setAttribute("showhold", analyserView->getShowHold());
    state->setAttribute("showfilledpeak", analyserView->getShowFilledPeak());
    state->setAttribute("processanalyserhidden", processAnalyserWhenHidden);
    state->setAttribute("thickcurves", thickCurves);
    state->setAttribute("colorhints", colorHints);
    return state;
}

void QRange::setSettingsXml(const juce::XmlElement* state)
{
    if (state != nullptr && ! state->hasTagName("SETTINGS"))
        state = state->getChildByName("SETTINGS");

    if (state == nullptr)
        return;

    if (IPlugGUIResize* resize = state->hasAttribute("size") ? GetGUIResize() : nullptr)
    {
        resize->SetGUIScaleRatio(state->getDoubleAttribute("size"));
        resize->UpdateDrawRects();
        GetGUI()->RescaleBitmaps(resize->GetGUIScaleRatio());
        resize->ResizeGraphics();
    }

    if (state->hasAttribute("analyserenabled"))
    {
        bandAnalyser.setEnabled(state->getBoolAttribute("analyserenabled", bandAnalyser.getEnabled()));
        analyseInput = state->getBoolAttribute("analyseinput", analyseInput);
        analyserRouting = juce::jlimit(kRoutingLeft, kRoutingSide, static_cast<ERoutings> (state->getIntAttribute("analyserrouting", static_cast<int> (analyserRouting))));;

        const float rngStart = static_cast<float> (state->getDoubleAttribute("analyserrangestart", analyserView->getMagRange().getStart()));
        const float rngEnd = static_cast<float> (state->getDoubleAttribute("analyserrangeend", analyserView->getMagRange().getEnd()));
        analyserView->setMagRange(juce::Range<float>(rngStart, rngEnd));
        updateAnalyserButtons();
    }

    if (analyserView != nullptr)
    {
        analyserView->setShowPeak(state->getBoolAttribute("showpeak", analyserView->getShowPeak()));
        analyserView->setShowHold(state->getBoolAttribute("showhold", analyserView->getShowHold()));
        analyserView->setShowFilledPeak(state->getBoolAttribute("showfilledpeak", analyserView->getShowFilledPeak()));
        processAnalyserWhenHidden = state->getBoolAttribute("processanalyserhidden", processAnalyserWhenHidden);
    }

    if (state->hasAttribute("magrange"))
        freqDisplay->setMagRange(static_cast<float> (state->getDoubleAttribute("magRange")));

    thickCurves = state->getBoolAttribute("thickcurves", false);
    colorHints = state->getBoolAttribute("colorhints", colorHints);
}

bool QRange::SerializeState(ByteChunk* pChunk)
{
    std::unique_ptr<juce::XmlElement> stateXml(getStateXml());

    if (stateXml.get() == nullptr)
        return false;

    stateXml->addChildElement(getSettingsXml());

    const juce::String state(stateXml->createDocument("", true, false));
    const int size = state.getNumBytesAsUTF8();
    const char* text = state.toRawUTF8();

    pChunk->Put(&xmlChunkMagicByte);
    pChunk->Put(&size);
    pChunk->PutBytes(text, size);

    return true;
}

int QRange::UnserializeState(ByteChunk* pChunk, int startPos)
{
    int pos = startPos;
    int magicByte = 0;
    int size = 0;
    pos = pChunk->Get(&magicByte, pos);
    pos = pChunk->Get(&size, pos);

    if (magicByte != xmlChunkMagicByte)
        return startPos;

    juce::HeapBlock<char> text(size);
    pos = pChunk->GetBytes(text.get(), size, pos);
    const juce::String state(juce::String::fromUTF8(text.get(), size));

    if (state.getNumBytesAsUTF8() != size)
        return startPos;

    std::unique_ptr<juce::XmlElement> xmlState(juce::XmlDocument::parse(state));

    if (xmlState.get() != nullptr)
    {
        setStateXml(xmlState.get());
        setSettingsXml(xmlState.get());
    }

    return pos;
}

void QRange::getFreqResponse(FilterBandDsp::ResponseData& data, int numPoints)
{
    curves.resetCurves(numPoints, mSampleRate);

    data.mid = curves.mid.data();
    data.side = curves.side.data();
    data.left = curves.left.data();
    data.right = curves.right.data();

    filterBands.getResponse(data, curves.normFreqs.data(), numPoints);

    curves.convertToDb(numPoints);
}

bool QRange::usesRouting(ERoutings routing)
{
    for (int i = 0; i < kNumBands; ++i)
    {
        const int offset = i * kNumParametersPerBand;
        const bool enabled = GetParam(offset + kEnabled)->Bool();

        if (! enabled)
            continue;

        const int val = GetParam(offset + kRouting)->Int();

        if (static_cast<ERoutings>(val) == routing)
            return true;
    }

    return false;
}

void QRange::setSoloIndex(int idx)
{
    soloIndex = idx;
}

int QRange::getSoloIndex() const
{
    return soloIndex;
}

void QRange::updateEnabledControls()
{
    freqDisplay->triggerUpdateCurves();

    for (int i = 0; i < kNumBands; ++i)
    {
        const int idx = i * kNumParametersPerBand + kEnabled;
        getControlForIndex(idx)->SetDirty(false);
    }
}

FreqDisplay* QRange::getFreqDisplay()
{
    return freqDisplay;
}

IControl* QRange::getControlForIndex(int idx)
{
    IGraphics* gui = GetGUI();

    for (int i = 0; i < gui->GetNControls(); ++i)
        if (gui->GetControl(i)->ParamIdx() == idx)
            return gui->GetControl(i);

    return nullptr;
}

void QRange::setControlValue(int idx, double normalized)
{
    if (IControl* c = getControlForIndex(idx))
        c->SetValueFromPlug(normalized);
}

bool QRange::getAnalyserEnabled() const
{
    return bandAnalyser.getEnabled();
}

bool QRange::getAnalyseInput() const
{
    return analyseInput;
}

juce::Range<float> QRange::getAnalyserRange() const
{
    return analyserView->getMagRange();
}

void QRange::updateAnalyserButtons()
{
    const bool preState = anaPreButton->GetValue() > 0.5;
    const bool postState = anaPostButton->GetValue() > 0.5;

    const bool preSupState = analyseInput && bandAnalyser.getEnabled();
    const bool postSupState = ! analyseInput && bandAnalyser.getEnabled();

    if (preState != preSupState || postState != postSupState)
        setAnalyserEnable(preSupState, postSupState);

    const IColor textCol(bandAnalyser.getEnabled() ? 0xCCCCCC : 0x777777);

    if (anaLabel != nullptr)
        anaLabel->setTextColor(textCol);

    if (anaRouting != nullptr)
        anaRouting->setTextColor(textCol);
}

BandAnalyser& QRange::getBandAnalyser()
{
    return bandAnalyser;
}

ERoutings QRange::getAnalyserRouting() const
{
    return analyserRouting;
}

ELatencySelection QRange::getLatencySelection() const
{
    return latencySelection;
}

void QRange::setAnalyserRouting(ERoutings routing)
{
    if (analyserRouting != routing)
    {
        analyserRouting = routing;
        bandAnalyser.clear();
    }
}

IColor QRange::getColourForRouting(ERoutings routing) const
{
    switch (routing)
    {
    default:
    case kRoutingStereo:
        return IColor(0x664411, 0xFF);
    case kRoutingLeft:
        return IColor(0x8888FF, 0xFF);
    case kRoutingRight:
        return IColor(0xFF8888, 0xFF);
    case kRoutingMid:
        return IColor(0x448844, 0xFF);
    case kRoutingSide:
        return IColor(0xCCCC44, 0xFF);
    }
}

IColor QRange::getBandColor(int bandIdx) const
{
    const int cols[12] = { 0x0075DC, 0x00998F, 0x2BCE48, 0x5EF1F2, 0x740AFF, 0x94FFB5, 0x9DCC00, 0xC20088, 0xFF0010, 0xFF5005, 0xF0A3FF, 0xFFA405 };

    if (bandIdx >= 0 && bandIdx < 12)
        return IColor(cols[bandIdx]);

    return IColor(0xCCCCCC);
}

void QRange::setThickCurves(bool thick)
{
    if (thickCurves != thick)
    {
        thickCurves = thick;
        GetGUI()->SetAllControlsDirty();
    }
}

bool QRange::getThickCurves() const
{
    return thickCurves;
}

void QRange::setColorHints(bool show)
{
    if (colorHints != show)
    {
        colorHints = show;
        GetGUI()->SetAllControlsDirty();
    }
}

bool QRange::getColorHints() const
{
    return colorHints;
}

LinPhaseIIR::ProcMode QRange::getProcMode() const
{
    return forwardIIR != nullptr ? forwardIIR->getProcMode() : LinPhaseIIR::kWindowPost;
}

void QRange::setProcMode(LinPhaseIIR::ProcMode procMode)
{
    if (auto p = forwardIIR.get())
        p->setProcMode(procMode);

    if (auto p = reverseIIR.get())
        p->setProcMode(procMode);
}

IPopupMenu* QRange::getAnalyserMenu()
{
    IPopupMenu* menu = new IPopupMenu();

    const bool showPeak = analyserView->getShowPeak();
    const bool showPeakFilled = analyserView->getShowFilledPeak();
    const bool showHold = analyserView->getShowHold();

    std::function<void(void)> togglePeak = [this, showPeak]() {
        analyserView->setShowPeak(! showPeak);
    };
    std::function<void(void)> toggleFilledPeak = [this, showPeakFilled]() {
        analyserView->setShowFilledPeak(! showPeakFilled);
    };
    std::function<void(void)> toggleHold = [this, showHold]() {
        analyserView->setShowHold(! showHold);
    };
    std::function<void(void)> processHidden = [this] {
        processAnalyserWhenHidden = ! processAnalyserWhenHidden;
    };


    const int enPeak = showHold ? IPopupMenuItem::kNoFlags : IPopupMenuItem::kDisabled;
    const int enHold = showPeak ? IPopupMenuItem::kNoFlags : IPopupMenuItem::kDisabled;
    const int enProc = processAnalyserWhenHidden ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;

    menu->AddItem(new IPopupMenuItem("Show Peak Curve", enPeak | (showPeak ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags), togglePeak));
    menu->AddItem(new IPopupMenuItem("Fill Peak Curve", showPeakFilled ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags, toggleFilledPeak));
    menu->AddItem(new IPopupMenuItem("Show Hold Curve", enHold | (showHold ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags), toggleHold));

    menu->AddSeparator();
    menu->AddItem(new IPopupMenuItem("Process When GUI Is Closed", enProc, processHidden));
    menu->AddItem("Range", getAnalyserRangeMenu(false));

    return menu;
}

IPopupMenu* QRange::getAnalyserRangeMenu(bool includeCaption)
{
    IPopupMenu* menu = new IPopupMenu();

    if (includeCaption)
    {
        menu->AddItem("Analyser Range", -1, IPopupMenuItem::kDisabled);
        menu->AddSeparator();
    }

    juce::Range<float> curRng(analyserView->getMagRange());

    for (int i = 0; i < 6; ++i)
    {
        const int range = 40 + i * 10;
        const juce::String text(juce::String(range) + " dB");
        std::function<void(void)> f = [this, range, curRng]() {
            const juce::Range<float> rng(curRng.getEnd() - range, curRng.getEnd());
            analyserView->setMagRange(rng);
            freqDisplay->SetDirty(false);
        };

        const int flags = std::abs(curRng.getLength() - range) < 1.f ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;

        menu->AddItem(new IPopupMenuItem(text.toRawUTF8(), flags, f));
    }

    return menu;
}

IPopupMenu* QRange::getLatencySettingsMenu(bool includeCaption)
{
    IPopupMenu* menu = new IPopupMenu();
    if (includeCaption)
    {
        menu->AddItem("Latency Setting", -1, IPopupMenuItem::kDisabled);
        menu->AddSeparator();
    }

    jassert(kNumLatencySelections == 5);
    const char* labels[kNumLatencySelections] = { "None (Minimum Phase)", "Short (Linear Phase)", "Medium (Linear Phase)", "Long (Linear Phase)", "Insane (Linear Phase)" };
    const ELatencySelection curSel = getLatencySelection();

    for (int i = 0; i < kNumLatencySelections; ++i)
    {
        const ELatencySelection sel = static_cast<ELatencySelection> (i);
        std::function<void(void)> f = [this, sel]() {
            setLatencySelection(sel);
        };

        const int flags = sel == curSel ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;

        menu->AddItem(new IPopupMenuItem(labels[i], flags, f));
    }

    return menu;
}

IPopupMenu* QRange::getProcessModeMenu(bool includeCaption)
{
    IPopupMenu* menu = new IPopupMenu();
    if (includeCaption)
    {
        menu->AddItem("Process Mode", -1, IPopupMenuItem::kDisabled);
        menu->AddSeparator();
    }

    const char* labels[LinPhaseIIR::ProcMode::kNumProcModes] = { "Low Distortion", "Medium Distortion, OK Frequency Response", "Good Frequency Response" };

    for (int i = 0; i < LinPhaseIIR::ProcMode::kNumProcModes; ++i)
    {
        const int flags = getProcMode() == static_cast<LinPhaseIIR::ProcMode> (i) ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;

        std::function<void()> setMode = [this, i]() {
            setProcMode(static_cast<LinPhaseIIR::ProcMode> (i));
        };

        menu->AddItem(new IPopupMenuItem(labels[i], flags, setMode));
    }

    return menu;
}

IPopupMenu* QRange::getRangeMenu(bool includeCaption)
{
    IPopupMenu* menu = new IPopupMenu();
    if (includeCaption)
    {
        menu->AddItem("Display Magnitude Range", -1, IPopupMenuItem::kDisabled);
        menu->AddSeparator();
    }

    const float curRange = freqDisplay->getMagRange();

    for (int i = 1; i <= 6; ++i)
    {
        const float range = 4.f * i;
        const int flags = curRange == range ? IPopupMenuItem::kChecked : IPopupMenuItem::kNoFlags;
        std::function<void(void)> f = [this, range] {
            freqDisplay->setMagRange(range);
        };

        const juce::String t("+/- " + juce::String(static_cast<int> (range)) + " dB");
        menu->AddItem(new IPopupMenuItem(t.toRawUTF8(), flags, f));
    }

    return menu;
}

IPopupMenu* QRange::getDefaultsMenu()
{
    IPopupMenu* menu = new IPopupMenu();
    menu->AddItem("Default Settings (only apply for new instances of QRange)", -1, IPopupMenuItem::kDisabled);
    menu->AddSeparator();

    std::unique_ptr<juce::XmlElement> xmlDefs(getDefaultsXml());
    const bool sizeExists = xmlDefs != nullptr && xmlDefs->hasAttribute("size");
    const bool defsExist = xmlDefs != nullptr && xmlDefs->getChildByName("DEFAULT_STATE") != nullptr;

    std::function<void(void)> setDefaultSettings = [this]() {
        std::unique_ptr<juce::XmlElement> xml(getDefaultsXml());
        if (xml != nullptr)
        {
            Defaults d(this);            

            while (juce::XmlElement* e = xml->getChildByName("DEFAULT_STATE"))
                xml->removeChildElement(e, true);

            xml->addChildElement(d.toXml());
            writeDefaultsFile(xml.get());
        }
    };
    std::function<void(void)> setDefaultSize = [this]() {
        std::unique_ptr<juce::XmlElement> xml(getDefaultsXml());
        if (xml != nullptr)
        {
            xml->setAttribute("size", GetGUIResize()->GetGUIScaleRatio());
            writeDefaultsFile(xml.get());
        }
    };
    std::function<void(void)> resetDefaultSize = [this]() {
        std::unique_ptr<juce::XmlElement> xml(getDefaultsXml());
        if (xml != nullptr)
        {
            if (xml->getChildByName("DEFAULT_STATE") == nullptr)
            {
                getDefaultsFile().deleteFile();
            }
            else
            {
                xml->removeAttribute("size");
                writeDefaultsFile(xml.get());
            }
        }
    };
    std::function<void(void)> resetDefaultSettings = [this]() {
        std::unique_ptr<juce::XmlElement> xml(getDefaultsXml());
        if (xml != nullptr)
        {
            if (! xml->hasAttribute("size"))
            {
                getDefaultsFile().deleteFile();
            }
            else
            {
                xml->removeChildElement(xml->getChildByName("DEFAULT_STATE"), true);
                writeDefaultsFile(xml.get());
            }
        }
    };

    menu->AddItem(new IPopupMenuItem("Set Current State as Default", IPopupMenuItem::kNoFlags, setDefaultSettings));
    menu->AddItem(new IPopupMenuItem("Reset Default State to Factory", defsExist ? IPopupMenuItem::kNoFlags : IPopupMenuItem::kDisabled, resetDefaultSettings));
    menu->AddSeparator();
    menu->AddItem(new IPopupMenuItem("Set Current Size as Default", IPopupMenuItem::kNoFlags, setDefaultSize));
    menu->AddItem(new IPopupMenuItem("Reset Default Size to Factory", sizeExists ? IPopupMenuItem::kNoFlags : IPopupMenuItem::kDisabled, resetDefaultSize));

    return menu;
}

void QRange::matchOutputLoudness()
{
    const double levIn = ebuIn.getValue();
    const double levOut = ebuOut.getValue();

    if (levIn < -40 || levOut < -40)
        return;

    if (IParam* p = GetParam(kOutputGain))
    {        
        const double gain = juce::jlimit(p->GetMin(), p->GetMax(), 0.1 * std::floor(10. * (levIn - levOut)));
        p->Set(gain);
        OnParamChange(kOutputGain, IPlugBase::kGui);

        if (IControl* c = getControlForIndex(kOutputGain))
            c->SetDirty(false);
    }
}

bool QRange::isMatchPossible() const
{
    const double levIn = ebuIn.getValue();
    const double levOut = ebuOut.getValue();

    return levIn > -40 && levOut > -40;
}

void QRange::processAnalyser(const double* inL, const double* inR, int numSamples)
{
    if (! processAnalyserWhenHidden && ! IsGUIOpened())
        return;

    if (inL == inR || inR == nullptr)
    {
        bandAnalyser.processBlock(inL, numSamples);
        return;
    }

    double gainL = 0.5;
    double gainR = 0.5;

    switch (analyserRouting)
    {
    case kRoutingLeft:
        gainL = 1;
        gainR = 0;
        break;
    case kRoutingRight:
        gainL = 0;
        gainR = 1;
        break;
    case kRoutingSide:
        gainR = -0.5;
        break;
    default:
        break;
    }

    const int blockSize = 128;
    double tmp[blockSize];

    while (numSamples > 0)
    {
        const int curNumSamples = juce::jmin(blockSize, numSamples);
        juce::FloatVectorOperations::copyWithMultiply(tmp, inL, gainL, curNumSamples);
        juce::FloatVectorOperations::addWithMultiply(tmp, inR, gainR, curNumSamples);

        static int splIdx = 0;

        if (analyseInput)
            analyserDelay.processBlock(tmp, tmp, curNumSamples);

        bandAnalyser.processBlock(tmp, curNumSamples);

        numSamples -= curNumSamples;
        inL += curNumSamples;
        inR += curNumSamples;
    }
}

void QRange::updateHideControls(int bandIndex)
{
    const int typeIdx = bandIndex * kNumParametersPerBand + kType;
    const int orderIdx = bandIndex * kNumParametersPerBand + kCutOrder;

    const EFilterTypes type = static_cast<EFilterTypes> (GetParam(typeIdx)->Int());
    const ECutOrders cutOrder = static_cast<ECutOrders> (GetParam(orderIdx)->Int());
    const bool hideGain = type == kTypeLoCut || type == kTypeHiCut;
    const bool hideQ = hideGain && cutOrder == kCutOrder48dB;

    if (IControl* gainControl = bandParams[bandIndex].gain != nullptr ? GetGUI()->GetControl(*bandParams[bandIndex].gain) : nullptr)
        gainControl->Hide(hideGain);

    if (IControl* cutOrder = bandParams[bandIndex].cutOrder != nullptr ? GetGUI()->GetControl(*bandParams[bandIndex].cutOrder) : nullptr)
        cutOrder->Hide(! hideGain);

    if (IControl* qFactor = bandParams[bandIndex].Q != nullptr ? GetGUI()->GetControl(*bandParams[bandIndex].Q) : nullptr)
        qFactor->Hide(hideQ);
}

void QRange::sortBands()
{
    BandSorter::sortBands(this);
    OnParamReset();
}

void QRange::setAnalyserEnable(bool enPre, bool enPost)
{
    const bool wasEnabled = bandAnalyser.getEnabled();
    const bool wasPre = analyseInput;
    const bool nowEnabeld = enPre || enPost;
    bandAnalyser.setEnabled(nowEnabeld);
    analyseInput = enPre;

    if (enPre != wasPre || (! wasEnabled && nowEnabeld))
        bandAnalyser.clear();

    if (anaPreButton != nullptr)
        anaPreButton->SetValueFromPlug(enPre);

    if (anaPostButton != nullptr)
        anaPostButton->SetValueFromPlug(enPost);

    if (wasEnabled && freqDisplay != nullptr)
        freqDisplay->SetDirty(false);
}

juce::File QRange::getDefaultsFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("lkjb").getChildFile("qrangedefaults.xml");
}

void QRange::writeDefaultsFile(const juce::XmlElement* xml) const
{
    if (xml != nullptr)
    {
        getDefaultsFile().getParentDirectory().createDirectory();
        xml->writeToFile(getDefaultsFile(), "");
    }
}

juce::XmlElement* QRange::getDefaultsXml() const
{
    juce::XmlElement* xml = juce::XmlDocument::parse(getDefaultsFile());
    if (xml == nullptr)
        xml = new juce::XmlElement("QRANGE");

    return xml;
}
