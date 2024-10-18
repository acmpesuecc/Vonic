#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

VonicRewriteAudioProcessor::VonicRewriteAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

VonicRewriteAudioProcessor::~VonicRewriteAudioProcessor() {}

//==============================================================================

const juce::String VonicRewriteAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VonicRewriteAudioProcessor::acceptsMidi() const { return JucePlugin_WantsMidiInput; }
bool VonicRewriteAudioProcessor::producesMidi() const { return JucePlugin_ProducesMidiOutput; }
bool VonicRewriteAudioProcessor::isMidiEffect() const { return JucePlugin_IsMidiEffect; }
double VonicRewriteAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int VonicRewriteAudioProcessor::getNumPrograms() { return 1; }
int VonicRewriteAudioProcessor::getCurrentProgram() { return 0; }
void VonicRewriteAudioProcessor::setCurrentProgram (int index) {}
const juce::String VonicRewriteAudioProcessor::getProgramName (int index) { return {}; }
void VonicRewriteAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

//==============================================================================

void VonicRewriteAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{ samplesPerBlock, sampleRate, 1 };
    prepareDSPChain(left, spec);
    prepareDSPChain(right, spec);
    updateFilterCoefficients(sampleRate);
}

void VonicRewriteAudioProcessor::releaseResources() {}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool VonicRewriteAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    #if JucePlugin_IsMidiEffect
        juce::ignoreUnused (layouts);
        return true;
    #else
        return isLayoutSupported(layouts);
    #endif
}
#endif

void VonicRewriteAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    clearUnusedChannels(buffer);
    processAudioBuffer(buffer);
}

//==============================================================================

bool VonicRewriteAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* VonicRewriteAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================

void VonicRewriteAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}

void VonicRewriteAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}

void VonicRewriteAudioProcessor::updateFilterCoefficients(double sampleRate)
{
    auto chainSettings = getFilterSet(bleh);
    updatePeakFilterCoefficients(chainSettings, sampleRate);
    updateLowCutFilterCoefficients(chainSettings, sampleRate);
}

void VonicRewriteAudioProcessor::setupLowCutFilter(DSPChain& chain, int slope, const std::array<std::shared_ptr<juce::dsp::IIR::Coefficients<float>>, 4>& coefficients)
{
    auto& lowCut = chain.get<ChainPositions::LowCut>();

    for (int i = 0; i < 4; ++i)
        lowCut.setBypassed<i>(true);

    for (int i = 0; i <= slope / 12; ++i) {
        *lowCut.get<i>().coefficients = *coefficients[i];
        lowCut.setBypassed<i>(false);
    }
}

FilterSet VonicRewriteAudioProcessor::getFilterSet(juce::AudioProcessorValueTreeState& bleh)
{
    FilterSet props;
    props.lowCutFreq = bleh.getRawParameterValue("HighPass")->load();
    props.highCutFreq = bleh.getRawParameterValue("LowPass")->load();
    props.peakFreq = bleh.getRawParameterValue("Peak")->load();
    props.peakGain = bleh.getRawParameterValue("Gain")->load();
    props.peakQual = bleh.getRawParameterValue("Quality")->load();
    props.lowCutSlope = static_cast<Gradient>(bleh.getRawParameterValue("HighPassGrad")->load());
    props.highCutSlope = static_cast<Gradient>(bleh.getRawParameterValue("LowPassGrad")->load());
    return props;
}

//==============================================================================

void VonicRewriteAudioProcessor::prepareDSPChain(MonoChain& chain, const juce::dsp::ProcessSpec& spec)
{
    chain.prepare(spec);
}

bool VonicRewriteAudioProcessor::isLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
}

void VonicRewriteAudioProcessor::clearUnusedChannels(juce::AudioBuffer<float>& buffer)
{
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

void VonicRewriteAudioProcessor::processAudioBuffer(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    left.process(leftContext);
    right.process(rightContext);
}

void VonicRewriteAudioProcessor::updatePeakFilterCoefficients(const FilterSet& chainSettings, double sampleRate)
{
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQual,
        juce::Decibels::decibelsToGain(chainSettings.peakGain));

    *left.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *right.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
}

void VonicRewriteAudioProcessor::updateLowCutFilterCoefficients(const FilterSet& chainSettings, double sampleRate)
{
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq, sampleRate, (chainSettings.lowCutSlope + 1) * 2);

    setupLowCutFilter(left, chainSettings.lowCutSlope, cutCoefficients);
    setupLowCutFilter(right, chainSettings.lowCutSlope, cutCoefficients);
}

//==============================================================================

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VonicRewriteAudioProcessor();
}
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

VonicRewriteAudioProcessor::VonicRewriteAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

VonicRewriteAudioProcessor::~VonicRewriteAudioProcessor() {}

//==============================================================================

const juce::String VonicRewriteAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VonicRewriteAudioProcessor::acceptsMidi() const { return JucePlugin_WantsMidiInput; }
bool VonicRewriteAudioProcessor::producesMidi() const { return JucePlugin_ProducesMidiOutput; }
bool VonicRewriteAudioProcessor::isMidiEffect() const { return JucePlugin_IsMidiEffect; }
double VonicRewriteAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int VonicRewriteAudioProcessor::getNumPrograms() { return 1; }
int VonicRewriteAudioProcessor::getCurrentProgram() { return 0; }
void VonicRewriteAudioProcessor::setCurrentProgram (int index) {}
const juce::String VonicRewriteAudioProcessor::getProgramName (int index) { return {}; }
void VonicRewriteAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

//==============================================================================

void VonicRewriteAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{ samplesPerBlock, sampleRate, 1 };
    prepareDSPChain(left, spec);
    prepareDSPChain(right, spec);
    updateFilterCoefficients(sampleRate);
}

void VonicRewriteAudioProcessor::releaseResources() {}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool VonicRewriteAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    #if JucePlugin_IsMidiEffect
        juce::ignoreUnused (layouts);
        return true;
    #else
        return isLayoutSupported(layouts);
    #endif
}
#endif

void VonicRewriteAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    clearUnusedChannels(buffer);
    processAudioBuffer(buffer);
}

//==============================================================================

bool VonicRewriteAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* VonicRewriteAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================

void VonicRewriteAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}

void VonicRewriteAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}

void VonicRewriteAudioProcessor::updateFilterCoefficients(double sampleRate)
{
    auto chainSettings = getFilterSet(bleh);
    updatePeakFilterCoefficients(chainSettings, sampleRate);
    updateLowCutFilterCoefficients(chainSettings, sampleRate);
}

void VonicRewriteAudioProcessor::setupLowCutFilter(DSPChain& chain, int slope, const std::array<std::shared_ptr<juce::dsp::IIR::Coefficients<float>>, 4>& coefficients)
{
    auto& lowCut = chain.get<ChainPositions::LowCut>();

    for (int i = 0; i < 4; ++i)
        lowCut.setBypassed<i>(true);

    for (int i = 0; i <= slope / 12; ++i) {
        *lowCut.get<i>().coefficients = *coefficients[i];
        lowCut.setBypassed<i>(false);
    }
}

FilterSet VonicRewriteAudioProcessor::getFilterSet(juce::AudioProcessorValueTreeState& bleh)
{
    FilterSet props;
    props.lowCutFreq = bleh.getRawParameterValue("HighPass")->load();
    props.highCutFreq = bleh.getRawParameterValue("LowPass")->load();
    props.peakFreq = bleh.getRawParameterValue("Peak")->load();
    props.peakGain = bleh.getRawParameterValue("Gain")->load();
    props.peakQual = bleh.getRawParameterValue("Quality")->load();
    props.lowCutSlope = static_cast<Gradient>(bleh.getRawParameterValue("HighPassGrad")->load());
    props.highCutSlope = static_cast<Gradient>(bleh.getRawParameterValue("LowPassGrad")->load());
    return props;
}

//==============================================================================

void VonicRewriteAudioProcessor::prepareDSPChain(MonoChain& chain, const juce::dsp::ProcessSpec& spec)
{
    chain.prepare(spec);
}

bool VonicRewriteAudioProcessor::isLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
}

void VonicRewriteAudioProcessor::clearUnusedChannels(juce::AudioBuffer<float>& buffer)
{
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

void VonicRewriteAudioProcessor::processAudioBuffer(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    left.process(leftContext);
    right.process(rightContext);
}

void VonicRewriteAudioProcessor::updatePeakFilterCoefficients(const FilterSet& chainSettings, double sampleRate)
{
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQual,
        juce::Decibels::decibelsToGain(chainSettings.peakGain));

    *left.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *right.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
}

void VonicRewriteAudioProcessor::updateLowCutFilterCoefficients(const FilterSet& chainSettings, double sampleRate)
{
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq, sampleRate, (chainSettings.lowCutSlope + 1) * 2);

    setupLowCutFilter(left, chainSettings.lowCutSlope, cutCoefficients);
    setupLowCutFilter(right, chainSettings.lowCutSlope, cutCoefficients);
}

//==============================================================================

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VonicRewriteAudioProcessor();
}
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

VonicRewriteAudioProcessor::VonicRewriteAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

VonicRewriteAudioProcessor::~VonicRewriteAudioProcessor() {}

//==============================================================================

const juce::String VonicRewriteAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VonicRewriteAudioProcessor::acceptsMidi() const { return JucePlugin_WantsMidiInput; }
bool VonicRewriteAudioProcessor::producesMidi() const { return JucePlugin_ProducesMidiOutput; }
bool VonicRewriteAudioProcessor::isMidiEffect() const { return JucePlugin_IsMidiEffect; }
double VonicRewriteAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int VonicRewriteAudioProcessor::getNumPrograms() { return 1; }
int VonicRewriteAudioProcessor::getCurrentProgram() { return 0; }
void VonicRewriteAudioProcessor::setCurrentProgram (int index) {}
const juce::String VonicRewriteAudioProcessor::getProgramName (int index) { return {}; }
void VonicRewriteAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

//==============================================================================

void VonicRewriteAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{ samplesPerBlock, sampleRate, 1 };
    prepareDSPChain(left, spec);
    prepareDSPChain(right, spec);
    updateFilterCoefficients(sampleRate);
}

void VonicRewriteAudioProcessor::releaseResources() {}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool VonicRewriteAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    #if JucePlugin_IsMidiEffect
        juce::ignoreUnused (layouts);
        return true;
    #else
        return isLayoutSupported(layouts);
    #endif
}
#endif

void VonicRewriteAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    clearUnusedChannels(buffer);
    processAudioBuffer(buffer);
}

//==============================================================================

bool VonicRewriteAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* VonicRewriteAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================

void VonicRewriteAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}

void VonicRewriteAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}

void VonicRewriteAudioProcessor::updateFilterCoefficients(double sampleRate)
{
    auto chainSettings = getFilterSet(bleh);
    updatePeakFilterCoefficients(chainSettings, sampleRate);
    updateLowCutFilterCoefficients(chainSettings, sampleRate);
}

void VonicRewriteAudioProcessor::setupLowCutFilter(DSPChain& chain, int slope, const std::array<std::shared_ptr<juce::dsp::IIR::Coefficients<float>>, 4>& coefficients)
{
    auto& lowCut = chain.get<ChainPositions::LowCut>();

    for (int i = 0; i < 4; ++i)
        lowCut.setBypassed<i>(true);

    for (int i = 0; i <= slope / 12; ++i) {
        *lowCut.get<i>().coefficients = *coefficients[i];
        lowCut.setBypassed<i>(false);
    }
}

FilterSet VonicRewriteAudioProcessor::getFilterSet(juce::AudioProcessorValueTreeState& bleh)
{
    FilterSet props;
    props.lowCutFreq = bleh.getRawParameterValue("HighPass")->load();
    props.highCutFreq = bleh.getRawParameterValue("LowPass")->load();
    props.peakFreq = bleh.getRawParameterValue("Peak")->load();
    props.peakGain = bleh.getRawParameterValue("Gain")->load();
    props.peakQual = bleh.getRawParameterValue("Quality")->load();
    props.lowCutSlope = static_cast<Gradient>(bleh.getRawParameterValue("HighPassGrad")->load());
    props.highCutSlope = static_cast<Gradient>(bleh.getRawParameterValue("LowPassGrad")->load());
    return props;
}

//==============================================================================

void VonicRewriteAudioProcessor::prepareDSPChain(MonoChain& chain, const juce::dsp::ProcessSpec& spec)
{
    chain.prepare(spec);
}

bool VonicRewriteAudioProcessor::isLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
}

void VonicRewriteAudioProcessor::clearUnusedChannels(juce::AudioBuffer<float>& buffer)
{
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
}

void VonicRewriteAudioProcessor::processAudioBuffer(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    left.process(leftContext);
    right.process(rightContext);
}

void VonicRewriteAudioProcessor::updatePeakFilterCoefficients(const FilterSet& chainSettings, double sampleRate)
{
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQual,
        juce::Decibels::decibelsToGain(chainSettings.peakGain));

    *left.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *right.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
}

void VonicRewriteAudioProcessor::updateLowCutFilterCoefficients(const FilterSet& chainSettings, double sampleRate)
{
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq, sampleRate, (chainSettings.lowCutSlope + 1) * 2);

    setupLowCutFilter(left, chainSettings.lowCutSlope, cutCoefficients);
    setupLowCutFilter(right, chainSettings.lowCutSlope, cutCoefficients);
}

//==============================================================================

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VonicRewriteAudioProcessor();
}
