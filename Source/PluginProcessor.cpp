#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

VonicRewriteAudioProcessor::VonicRewriteAudioProcessor()

#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
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

bool VonicRewriteAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VonicRewriteAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VonicRewriteAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VonicRewriteAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VonicRewriteAudioProcessor::getNumPrograms() { return 1; }

int VonicRewriteAudioProcessor::getCurrentProgram() { return 0; }

void VonicRewriteAudioProcessor::setCurrentProgram (int index) {}

const juce::String VonicRewriteAudioProcessor::getProgramName (int index) { return {}; }

void VonicRewriteAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

//==============================================================================

void VonicRewriteAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec set;
    set.maximumBlockSize = samplesPerBlock;
    set.numChannels = 1;
    set.sampleRate = sampleRate;

    left.prepare(set);
    right.prepare(set);

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
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void VonicRewriteAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    left.process(leftContext);
    right.process(rightContext);
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

    // Peak filter coefficients
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peakFreq,
        chainSettings.peakQual,
        juce::Decibels::decibelsToGain(chainSettings.peakGain));

    *left.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *right.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    // Low-cut filter coefficients
    auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq, sampleRate, (chainSettings.lowCutSlope + 1) * 2);

    setupLowCutFilter(left, chainSettings.lowCutSlope, cutCoefficients);
    setupLowCutFilter(right, chainSettings.lowCutSlope, cutCoefficients);
}

void VonicRewriteAudioProcessor::setupLowCutFilter(DSPChain& chain, int slope, const std::array<std::shared_ptr<juce::dsp::IIR::Coefficients<float>>, 4>& coefficients)
{
    auto& lowCut = chain.get<ChainPositions::LowCut>();

    lowCut.setBypassed<0>(true);
    lowCut.setBypassed<1>(true);
    lowCut.setBypassed<2>(true);
    lowCut.setBypassed<3>(true);

    switch (slope)
    {
        case grad12:
            *lowCut.get<0>().coefficients = *coefficients[0];
            lowCut.setBypassed<0>(false);
            break;
        case grad24:
            *lowCut.get<0>().coefficients = *coefficients[0];
            lowCut.setBypassed<0>(false);
            *lowCut.get<1>().coefficients = *coefficients[1];
            lowCut.setBypassed<1>(false);
            break;
        case grad36:
            *lowCut.get<0>().coefficients = *coefficients[0];
            lowCut.setBypassed<0>(false);
            *lowCut.get<1>().coefficients = *coefficients[1];
            lowCut.setBypassed<1>(false);
            *lowCut.get<2>().coefficients = *coefficients[2];
            lowCut.setBypassed<2>(false);
            break;
        case grad48:
            *lowCut.get<0>().coefficients = *coefficients[0];
            lowCut.setBypassed<0>(false);
            *lowCut.get<1>().coefficients = *coefficients[1];
            lowCut.setBypassed<1>(false);
            *lowCut.get<2>().coefficients = *coefficients[2];
            lowCut.setBypassed<2>(false);
            *lowCut.get<3>().coefficients = *coefficients[3];
            lowCut.setBypassed<3>(false);
            break;
    }
}

FilterSet getFilterSet(juce::AudioProcessorValueTreeState& bleh)
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

// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VonicRewriteAudioProcessor();
}
