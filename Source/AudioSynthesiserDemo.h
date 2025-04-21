/*
  ==============================================================================

   This file is part of the JUCE framework examples.
   Copyright (c) Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
   REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
   INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
   OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   PERFORMANCE OF THIS SOFTWARE.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             AudioSynthesiserDemo
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Simple synthesiser application.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2022, linux_make, androidstudio, xcode_iphone

 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:             Component
 mainClass:        AudioSynthesiserDemo

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

#include "DemoUtilities.h"
#include "AudioLiveScrollingDisplay.h"

//==============================================================================
/** Our demo synth sound is just a basic sine wave.. */
struct SineWaveSound final : public SynthesiserSound
{
    bool appliesToNote (int /*midiNoteNumber*/) override    { return true; }
    bool appliesToChannel (int /*midiChannel*/) override    { return true; }
};

//==============================================================================
/** Our demo synth voice just plays a sine wave.. */
class SineWaveVoice : public juce::SynthesiserVoice
{
public:
    enum WaveType { Sine, Square, Sawtooth, Triangle };
    SineWaveVoice()
        : currentAngle(0.0), angleDelta(0.0), level(0.0), tailOff(0.0)
    {
        // Initialize ADSR parameters when the object is created
        adsrParams.attack = 0.5f;
        adsrParams.decay = 0.1f;
        adsrParams.sustain = 0.9f;
        adsrParams.release = 0.9f;
        adsr.setParameters(adsrParams);
    }

    ~SineWaveVoice() override {}

    
    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }
    void setAttack(float attack)
      {
          adsrParams.attack = attack;
        adsr.setParameters(adsrParams);
        
      }

      void setDecay(float decay)
      {
          adsrParams.decay = decay;
          adsr.setParameters(adsrParams);
          
      }

      void setSustain(float sustain)
      {
          adsrParams.sustain = sustain;
          adsr.setParameters(adsrParams);

      }

      void setRelease(float release)
      {
          adsrParams.release = release;
          adsr.setParameters(adsrParams);

      }
    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        isNoteOn = true;
            envelopeValue = 0.0f; // Reset envelope value
            currentAmplitude = 0.0f; // Reset current amplitude

        auto cyclesPerSecond = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();

        angleDelta = cyclesPerSample * MathConstants<double>::twoPi;
        adsr.noteOn(); // Start the ADSR envelope
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
         {
             adsr.noteOff(); // Start the release phase
         }
         else
         {
             clearCurrentNote();
             //adsr.reset(); // Reset the ADSR envelope
         }
    }

    

    void pitchWheelMoved (int /*newValue*/) override                              {}
    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) override    {}

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        
        if (!adsr.isActive()) // Stop rendering if the envelope is inactive
        {
            clearCurrentNote();
            return;
        }
        if (!approximatelyEqual(angleDelta, 0.0))
        {
            while (--numSamples >= 0)
            {
                float currentSample = 0.0f;

                switch (currentWaveType)
                {
                    case Sine:
                        currentSample = (float) (std::sin(currentAngle) * level);
                        break;
                    case Square:
                        currentSample = (float) (std::sin(currentAngle) >= 0 ? level : -level);
                        break;
                    case Sawtooth:
                        currentSample = (float) ((2.0 * (currentAngle / MathConstants<double>::twoPi) - 1.0) * level);
                        break;
                    case Triangle:
                        currentSample = (float) (2.0 * (currentAngle / MathConstants<double>::twoPi) - 1.0);
                        currentSample = (currentSample < 0) ? -currentSample : currentSample; // Absolute value for triangle
                        currentSample *= level; // Scale by level
                        break;
                }

                for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                {
                    adsr.applyEnvelopeToBuffer(outputBuffer, startSample, adsr.getNextSample() * currentSample);
                    outputBuffer.addSample(i, startSample, adsr.getNextSample() * currentSample);
                    
                }

                currentAngle += angleDelta;
                if (currentAngle >= MathConstants<float>::twoPi)
                    currentAngle -= MathConstants<float>::twoPi;
              //  adsr.applyEnvelopeToBuffer(outputBuffer, startSample, numSamples);
                ++startSample;
            }
          
        }

    }

    using SynthesiserVoice::renderNextBlock;
    void setWaveType(WaveType newType)
    {
        currentWaveType = newType;
    }
    
    void setADSRSampleRate(double sampleRate){
        adsr.setSampleRate (sampleRate);
    }
private:
    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
  
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;
    WaveType currentWaveType = Sine;
    float attackTime = 0.1f; // in seconds
       float decayTime = 0.1f;  // in seconds
       float sustainLevel = 0.5f; // 0.0 to 1.0
       float releaseTime = 0.2f; // in seconds

       float currentAmplitude = 0.0f; // Current amplitude based on ADSR
       float envelopeValue = 0.0f; // Current envelope value
       bool isNoteOn = false; // Track if the note is currently on
};

//==============================================================================
// This is an audio source that streams the output of our demo synth.
struct SynthAudioSource final : public AudioSource
{
    SynthAudioSource (MidiKeyboardState& keyState)  : keyboardState (keyState)
    {
        // Add some voices to our synth, to play the sounds..
        for (auto i = 0; i < 4; ++i)
        {
            synth.addVoice (new SineWaveVoice());   // These voices will play our custom sine-wave sounds..
            synth.addVoice (new SamplerVoice());    // and these ones play the sampled sounds
        }

        // ..and add a sound for them to play...
        setUsingSineWaveSound();
        filter.state = *dsp::IIR::Coefficients<float>::makeLowPass(44100, 1000.0f, 0.7f);
    }

    void setVolume(float newVolume)
    {
        volume = newVolume;
    }
    void setUsingSineWaveSound()
    {
        synth.clearSounds();
        synth.addSound (new SineWaveSound());
    }

    void setUsingSampledSound()
    {
        WavAudioFormat wavFormat;

        std::unique_ptr<AudioFormatReader> audioReader (wavFormat.createReaderFor (createAssetInputStream ("cello.wav").release(), true));

        BigInteger allNotes;
        allNotes.setRange (0, 128, true);

        synth.clearSounds();
        synth.addSound (new SamplerSound ("demo sound",
                                          *audioReader,
                                          allNotes,
                                          74,   // root midi note
                                          0.1,  // attack time
                                          0.1,  // release time
                                          10.0  // maximum sample length
                                          ));
    }

    void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override
    {
        midiCollector.reset (sampleRate);

        synth.setCurrentPlaybackSampleRate (sampleRate);
        dsp::ProcessSpec spec;
                spec.sampleRate = sampleRate;
                spec.maximumBlockSize = 512;
                spec.numChannels = 2;
                filter.prepare(spec);
        
        for (int i = 0; i < synth.getNumVoices(); ++i)
                {
                    auto* voice = synth.getVoice(i);

                    if (auto* sineWaveVoice = dynamic_cast<SineWaveVoice*>(voice))
                    {
                        sineWaveVoice->setADSRSampleRate(sampleRate);  // Set the sample rate for each voice
                    }
                }
       //  synth.getVoice(0)->setADSRSampleRate(sampleRate);
    }

    void releaseResources() override {}

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        // the synth always adds its output to the audio buffer, so we have to clear it
        // first..
        bufferToFill.clearActiveBufferRegion();

        // fill a midi buffer with incoming messages from the midi input.
        MidiBuffer incomingMidi;
        midiCollector.removeNextBlockOfMessages (incomingMidi, bufferToFill.numSamples);

        // pass these messages to the keyboard state so that it can update the component
        // to show on-screen which keys are being pressed on the physical midi keyboard.
        // This call will also add midi messages to the buffer which were generated by
        // the mouse-clicking on the on-screen keyboard.
        keyboardState.processNextMidiBuffer (incomingMidi, 0, bufferToFill.numSamples, true);

        // and now get the synth to process the midi events and generate its output.
        synth.renderNextBlock (*bufferToFill.buffer, incomingMidi, 0, bufferToFill.numSamples);
        dsp::AudioBlock<float> block (*bufferToFill.buffer);
               dsp::ProcessContextReplacing<float> context (block);
               filter.process(context);
        
     //   float rmsLevel = bufferToFill.buffer->getRMSLevel(0, 0, bufferToFill.numSamples);
      //  float gainCompensation = calculateGainCompensation(rmsLevel);
       // smoothedGainCompensation += 0.1f * (gainCompensation - smoothedGainCompensation);

        // Apply volume and gain compensation to the audio buffer
        //bufferToFill.buffer->applyGain(volume);
       // applySoftLimiter(*bufferToFill.buffer);
    }
    /*
    float calculateGainCompensation(float rmsLevel)
    {
        float targetLevel = 0.1f; // Target RMS level
        float maxGain = 2.0f; // Maximum gain compensation
        float gain = targetLevel / (rmsLevel + 0.0001f);
        return jmin(gain, maxGain); // Limit the gain
    }
    void applySoftLimiter(AudioBuffer<float>& buffer)
    {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        {
            auto* samples = buffer.getWritePointer(channel);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                samples[i] = tanh(samples[i]); // Simple soft clipping
            }
        }
    }

*/
    void updateFilterCoefficients(double frequency, double resonance)
    {
        *filter.state = *dsp::IIR::Coefficients<float>::makeLowPass(synth.getSampleRate(), frequency, resonance);
    }
    //==============================================================================
    // this collects real-time midi messages from the midi input device, and
    // turns them into blocks that we can process in our audio callback
    MidiMessageCollector midiCollector;

    // this represents the state of which keys on our on-screen keyboard are held
    // down. When the mouse is clicked on the keyboard component, this object also
    // generates midi messages for this, which we can pass on to our synth.
    MidiKeyboardState& keyboardState;

    // the synth itself!
    Synthesiser synth;
    float volume = 1; // Default volume
    float smoothedGainCompensation = 1.0f; // Smoothed gain compensation

    dsp::ProcessorDuplicator<dsp::IIR::Filter<float>, dsp::IIR::Coefficients<float>> filter;
};

//==============================================================================
class Callback final : public AudioIODeviceCallback
{
public:
    Callback (AudioSourcePlayer& playerIn, LiveScrollingAudioDisplay& displayIn)
        : player (playerIn), display (displayIn) {}

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                           int numInputChannels,
                                           float* const* outputChannelData,
                                           int numOutputChannels,
                                           int numSamples,
                                           const AudioIODeviceCallbackContext& context) override
    {
        player.audioDeviceIOCallbackWithContext (inputChannelData,
                                                 numInputChannels,
                                                 outputChannelData,
                                                 numOutputChannels,
                                                 numSamples,
                                                 context);
        display.audioDeviceIOCallbackWithContext (outputChannelData,
                                                  numOutputChannels,
                                                  nullptr,
                                                  0,
                                                  numSamples,
                                                  context);
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        player.audioDeviceAboutToStart (device);
        display.audioDeviceAboutToStart (device);
    }

    void audioDeviceStopped() override
    {
        player.audioDeviceStopped();
        display.audioDeviceStopped();
    }

private:
    AudioSourcePlayer& player;
    LiveScrollingAudioDisplay& display;
};

struct MidiLogger  : public MidiInputCallback
{
    void handleIncomingMidiMessage (MidiInput* /*source*/,
                                    const MidiMessage& m) override
    {
        DBG ("MIDI Received: " << m.getDescription());
    }
};

//==============================================================================
class AudioSynthesiserDemo final : public Component
{
public:
    AudioSynthesiserDemo()
    {
        
        
        addAndMakeVisible (keyboardComponent);
        addAndMakeVisible (cutoffSlider);
            cutoffSlider.setRange (20.0, 20000.0);
            cutoffSlider.setSkewFactorFromMidPoint (1000.0);
            cutoffSlider.setValue (1000.0);
            cutoffSlider.onValueChange = [this] { synthAudioSource.updateFilterCoefficients(cutoffSlider.getValue(), resonanceSlider.getValue()); };

            // Add and configure the resonance slider
            addAndMakeVisible (resonanceSlider);
            resonanceSlider.setRange (0.1, 40.0);
            resonanceSlider.setValue (0.7);
            resonanceSlider.onValueChange = [this] { synthAudioSource.updateFilterCoefficients(cutoffSlider.getValue(), resonanceSlider.getValue()); };
        addAndMakeVisible (sineButton);
        sineButton.setRadioGroupId (321);
        sineButton.setToggleState (true, dontSendNotification);
        sineButton.onClick = [this] { synthAudioSource.setUsingSineWaveSound(); };

        addAndMakeVisible (sampledButton);
        sampledButton.setRadioGroupId (321);
        sampledButton.onClick = [this] { synthAudioSource.setUsingSampledSound(); };
        addAndMakeVisible(volumeSlider);
               volumeSlider.setRange(0.0, 1.0);
               volumeSlider.setValue(0.5); // Default to 50% volume
               volumeSlider.setSliderStyle(Slider::Rotary);
               volumeSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
               volumeSlider.onValueChange = [this] { synthAudioSource.setVolume(volumeSlider.getValue()); };
        
        addAndMakeVisible (liveAudioDisplayComp);
        audioSourcePlayer.setSource (&synthAudioSource);
        addAndMakeVisible(waveTypeSelector);
        waveTypeSelector.addItem("Sine", 1);
        waveTypeSelector.addItem("Square", 2);
        waveTypeSelector.addItem("Sawtooth", 3);
        waveTypeSelector.addItem("Triangle", 4);
        waveTypeSelector.onChange = [this] { updateWaveType(); };
        waveTypeSelector.setSelectedId(1);

        addAndMakeVisible(attackSlider);
               attackSlider.setRange(0.1f,5.0f); // Range from 10ms to 5 seconds
               attackSlider.setValue(0.1f); // Default to 100ms
               attackSlider.setSliderStyle(Slider::LinearVertical);
               attackSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
        attackSlider.onValueChange = [this]
        {
            for (int i = 0; i < synthAudioSource.synth.getNumVoices(); ++i){
                if (auto* voice = dynamic_cast<SineWaveVoice*>(synthAudioSource.synth.getVoice(i)))
                {
                    voice->setAttack(attackSlider.getValue());
                }
            }
            
        };

               addAndMakeVisible(decaySlider);
               decaySlider.setRange(0.1f, 2.0f); // Range from 10ms to 5 seconds
               decaySlider.setValue(0.8f); // Default to 100ms
               decaySlider.setSliderStyle(Slider::LinearVertical);
               decaySlider.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
        decaySlider.onValueChange = [this]
        {
            for (int i = 0; i < synthAudioSource.synth.getNumVoices(); ++i){
                if (auto* voice = dynamic_cast<SineWaveVoice*>(synthAudioSource.synth.getVoice(i)))
                {
                    voice->setDecay(decaySlider.getValue());
                }
                
            }
            
        };

               addAndMakeVisible(sustainSlider);
               sustainSlider.setRange(0.0f, 1.0f); // Range from 0 to 1
               sustainSlider.setValue(0.8f); // Default to 50%
               sustainSlider.setSliderStyle(Slider::LinearVertical);
               sustainSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
        sustainSlider.onValueChange = [this]
        {
            for (int i = 0; i < synthAudioSource.synth.getNumVoices(); ++i){
                if (auto* voice = dynamic_cast<SineWaveVoice*>(synthAudioSource.synth.getVoice(i)))
                            {
                                voice->setSustain(sustainSlider.getValue());
                            }
            }
            
        };


               addAndMakeVisible(releaseSlider);
               releaseSlider.setRange(0.1f, 10.0f); // Range from 10ms to 5 seconds
               releaseSlider.setValue(0.8f); // Default to 200ms
               releaseSlider.setSliderStyle(Slider::LinearVertical);
               releaseSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 50, 20);
        releaseSlider.onValueChange = [this]
        {
            for (int i = 0; i < synthAudioSource.synth.getNumVoices(); ++i){
                if (auto* voice = dynamic_cast<SineWaveVoice*>(synthAudioSource.synth.getVoice(i)))
                            {
                                voice->setRelease(releaseSlider.getValue());
                            }
            }
            
        };

    

       #ifndef JUCE_DEMO_RUNNER
        audioDeviceManager.initialise (0, 2, nullptr, true, {}, nullptr);
       #endif
        
        // 1) Build the list of available MIDI inputs
        auto devices = MidiInput::getAvailableDevices();
        int id = 1;
        for (auto& d : devices)
            midiInputList.addItem (d.name, id++);
        midiInputList.setSelectedId (1);

        
        midiInputList.onChange = [this] { setMidiInputDevice(); };

      
        addAndMakeVisible (midiInputList);
        midiInputList.setBounds (400, 420, 200, 24);  // adjust to suit your layout

        // 4) Open the default device immediately
        setMidiInputDevice();


        audioDeviceManager.addAudioCallback (&callback);
        audioDeviceManager.addMidiInputDeviceCallback ({}, &(synthAudioSource.midiCollector));

        setOpaque (true);
        setSize (640, 480);
        
        
    }

    ~AudioSynthesiserDemo() override
    {
        audioSourcePlayer.setSource (nullptr);
        audioDeviceManager.removeMidiInputDeviceCallback ({}, &(synthAudioSource.midiCollector));
        audioDeviceManager.removeAudioCallback (&callback);
    }

    //==============================================================================
    void paint (Graphics& g) override
    {
        g.fillAll (getUIColourIfAvailable (LookAndFeel_V4::ColourScheme::UIColour::windowBackground));
    }

    void resized() override
    {
        volumeSlider.setBounds(16, 300, getWidth() - 32, 50);
        keyboardComponent   .setBounds (8, 96, getWidth() - 16, 64);
        sineButton          .setBounds (16, 176, 150, 24);
        sampledButton       .setBounds (16, 200, 150, 24);
        liveAudioDisplayComp.setBounds (8, 8, getWidth() - 16, 64);
        attackSlider.setBounds(16, 350, 50, 120); // X, Y, Width, Height
        decaySlider.setBounds(80, 350, 50, 120);
        sustainSlider.setBounds(144, 350, 50, 120);
        releaseSlider.setBounds(208, 350, 50, 120);
        cutoffSlider.setBounds(16, 240, getWidth() - 32, 24);
        resonanceSlider.setBounds(16, 270, getWidth() - 32, 24);
        waveTypeSelector.setBounds(16, 330, getWidth() - 32, 24);


    }
    void setMidiInputDevice()
    {
        auto devices = MidiInput::getAvailableDevices();
        int idx = midiInputList.getSelectedId() - 1;
        if (idx < 0 || idx >= devices.size())
            return;

        auto newID = devices[idx].identifier;
        audioDeviceManager.addMidiInputDeviceCallback (newID, &midiLogger);

        // 1) Disable the previous port
        if (currentMidiInput.isNotEmpty())
            audioDeviceManager.setMidiInputDeviceEnabled (currentMidiInput, false);

        audioDeviceManager.removeMidiInputDeviceCallback (currentMidiInput,
                                                          &synthAudioSource.midiCollector);

        // 2) Enable & register the new port
        audioDeviceManager.setMidiInputDeviceEnabled (newID, true);
        audioDeviceManager.addMidiInputDeviceCallback    (newID,
                                                          &synthAudioSource.midiCollector);

        currentMidiInput = newID;
    }


private:
    // if this PIP is running inside the demo runner, we'll use the shared device manager instead
   #ifndef JUCE_DEMO_RUNNER
    AudioDeviceManager audioDeviceManager;
   #else
    AudioDeviceManager& audioDeviceManager { getSharedAudioDeviceManager (0, 2) };
   #endif

    MidiKeyboardState keyboardState;
    AudioSourcePlayer audioSourcePlayer;
    SynthAudioSource synthAudioSource        { keyboardState };
    MidiKeyboardComponent keyboardComponent  { keyboardState, MidiKeyboardComponent::horizontalKeyboard};

    ToggleButton sineButton     { "Use sine wave" };
    ToggleButton sampledButton  { "Use sampled sound" };

    LiveScrollingAudioDisplay liveAudioDisplayComp;

    Callback callback { audioSourcePlayer, liveAudioDisplayComp };
    
    juce::Slider cutoffSlider;
    juce::Slider resonanceSlider;
    juce::Slider volumeSlider;
    ComboBox waveTypeSelector;
    Slider attackSlider;
    Slider decaySlider;
    Slider sustainSlider;
    Slider releaseSlider;
    ComboBox midiInputList;
    String   currentMidiInput;
    MidiLogger midiLogger;  // as a member alongside midiInputList
    juce::AudioDeviceManager deviceManager; // [1]
    juce::Label midiInputListLabel;
    int lastInputIndex = 0; // [3]
    bool isAddingFromMidiInput = false; // [4]
    juce::TextEditor midiMessagesBox;
    double startTime;

    
    void updateWaveType()
    {
        auto selectedWave = static_cast<SineWaveVoice::WaveType>(waveTypeSelector.getSelectedId() - 1);
        for (auto i = 0; i < synthAudioSource.synth.getNumVoices(); ++i)
        {
            if (auto* voice = dynamic_cast<SineWaveVoice*>(synthAudioSource.synth.getVoice(i)))
                voice->setWaveType(selectedWave);
        }
    }
    


   

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioSynthesiserDemo)
};
