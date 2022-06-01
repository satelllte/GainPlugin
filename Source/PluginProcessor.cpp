/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
/** Helper function for generating the parameter layout. */
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout params (
        std::make_unique<juce::AudioParameterFloat>(
            "MainGain",
            "Gain",
            juce::NormalisableRange<float>(0.0, 1.0),
            0.8,
            juce::String(),
            juce::AudioProcessorParameter::genericParameter,
            [](float value, int /* maxLength */) {
                return juce::String(juce::Decibels::gainToDecibels(value), 1) + "dB";
            },
            nullptr
        ),
        std::make_unique<juce::AudioParameterBool>(
            "MainMute",
            "Mute",
            false
       )
    );

    return params;
}

//==============================================================================
GainPluginAudioProcessor::GainPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       params(*this, nullptr, JucePlugin_Name, createParameterLayout())
{
}

GainPluginAudioProcessor::~GainPluginAudioProcessor()
{
    stopTimer();
}

//==============================================================================
const juce::String GainPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GainPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GainPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GainPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GainPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GainPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int GainPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void GainPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String GainPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void GainPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void GainPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    gain.reset(sampleRate, 0.02);
}

void GainPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GainPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void GainPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.

    // Our intense dsp processing
    gain.setTargetValue(*params.getRawParameterValue("MainGain"));
    gain.applyGain(buffer, buffer.getNumSamples());

    if (auto *muteParam  = dynamic_cast<juce::AudioParameterBool*>(params.getParameter("MainMute")))
    {
        bool muted = muteParam->get();
        if (muted)
            buffer.applyGain(0.0f);
    }

    // Read current block gain peak value
    gainPeakValue = buffer.getMagnitude (0, buffer.getNumSamples());
}

//==============================================================================
bool GainPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* GainPluginAudioProcessor::createEditor()
{
    // The GainPlugin example uses the GenericEditor, which is a default
    // AudioProcessorEditor provided that will automatically bootstrap
    // your React root, install some native method hooks for parameter interaction
    // if you provide an AudioProcessorValueTreeState, and manage hot reloading
    // of the source bundle. You can always start with the GenericEditor
    // then switch to a custom editor when you need more explicit control.
    juce::File sourceDir = juce::File(GAINPLUGIN_SOURCE_DIR);
    juce::File bundle = sourceDir.getChildFile("jsui/build/js/main.js");

    auto* editor = new reactjuce::GenericEditor(*this, bundle);

    editor->setResizable(true, true);
    editor->setResizeLimits(400, 240, 400 * 2, 240 * 2);
    editor->getConstrainer()->setFixedAspectRatio(400.0 / 240.0);
    editor->setSize (400, 240);

    // Start timer to dispatch gainPeakValues event to update Meter values
    startTimer(100);

    return editor;
}

void GainPluginAudioProcessor::timerCallback()
{
    if (auto* editor = dynamic_cast<reactjuce::GenericEditor*>(getActiveEditor()))
    {
        // Dispatch gainPeakValues event used by Meter React component
        editor->getReactAppRoot().dispatchEvent(
            "gainPeakValues",
            static_cast<float>(gainPeakValue),
            static_cast<float>(gainPeakValue)
        );
    }
}

//==============================================================================
void GainPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void GainPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GainPluginAudioProcessor();
}
