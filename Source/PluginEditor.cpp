
/*

  ==============================================================================



    This file contains the basic framework code for a JUCE plugin editor.



  ==============================================================================

*/



#include "PluginProcessor.h"

#include "PluginEditor.h"



//==============================================================================

VonicRewriteAudioProcessorEditor::VonicRewriteAudioProcessorEditor (VonicRewriteAudioProcessor& p)

    : AudioProcessorEditor (&p), audioProcessor (p)

{

    // Make sure that before the constructor has finished, you've set the

    // editor's size to whatever you need it to be.

    setSize (400, 300);

}



VonicRewriteAudioProcessorEditor::~VonicRewriteAudioProcessorEditor()

{

}



//==============================================================================

void VonicRewriteAudioProcessorEditor::paint (juce::Graphics& g)

{

    // (Our component is opaque, so we must completely fill the background with a solid colour)

    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));



    g.setColour (juce::Colours::white);

    g.setFont (15.0f);

    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);

}



void VonicRewriteAudioProcessorEditor::resized()

{

    // This is generally where you'll want to lay out the positions of any

    // subcomponents in your editor..

}
class MyPluginEditor : public AudioProcessorEditor
{
public:
    MyPluginEditor(MyPluginAudioProcessor& processor)
        : AudioProcessorEditor(processor), audioProcessor(processor)
    {
        // Create a ToggleButton
        addAndMakeVisible(toggleButton);
        toggleButton.setButtonText("Toggle State");
        toggleButton.addListener(this);

        // Set the initial state of the button based on the plugin's state
        toggleButton.setToggleState(audioProcessor.getPluginState(), NotificationType::dontSendNotification);

        setSize(300, 200); // Set the size of your plugin window
    }

    void paint(Graphics& g) override
    {
        // Paint your plugin's GUI here
    }

    void resized() override
    {
        // Set the size and position of the toggle button
        toggleButton.setBounds(getWidth() / 2 - 50, getHeight() / 2 - 25, 100, 50);
    }

    void buttonClicked(Button* button) override
    {
        if (button == &toggleButton)
        {
            // Handle the button click event and toggle the state
            audioProcessor.togglePluginState();
            toggleButton.setToggleState(audioProcessor.getPluginState(), NotificationType::dontSendNotification);
        }
    }

private:
    MyPluginAudioProcessor& audioProcessor;
    ToggleButton toggleButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyPluginEditor)
};


