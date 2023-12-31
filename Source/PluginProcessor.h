
/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
enum Gradient{
  grad12,
  grad24,
  grad36,
  grad48
};
struct FilterSet{
  float peakFreq {0},peakGain{0},peakQual{1.f};
  float lowCutFreq {0},highCutFreq {0};
  Gradient lowCutSlope {Gradient::grad12},highCutSlope{Gradient::grad12};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& bleh);

//==============================================================================
/**
*/
class VonicRewriteAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    VonicRewriteAudioProcessor();
    ~VonicRewriteAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    juce::AudioProcessorValueTreeState bleh{*this,nullptr,"HOHO",createParams()};
private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using CutFilter = juce::dsp::ProcessorChain<Filter,Filter,Filter,Filter>;
    using MonoChain = juce::dsp::ProcessorChain<CutFilter,Filter,CutFilter>;
    MonoChain left,right;
    enum ChainPositions{
      LowCut,
      Peak,
      HighCut
    };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VonicRewriteAudioProcessor)
};
