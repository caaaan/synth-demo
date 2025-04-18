//
//  FilterProcessor.h
//  AudioSynthesiserDemo
//
//  Created by Can Borcbakan on 07/01/2025.
//  Copyright © 2025 JUCE. All rights reserved.
//

#ifndef FilterProcessor_h
#define FilterProcessor_h

// FilterProcessor.h
#pragma once

#include <JuceHeader.h>

class FilterProcessor
{
public:
    FilterProcessor()
    {
        updateCoefficients(1000.0f, 0.7f); // Default cutoff and resonance
    }

    void updateCoefficients(float cutoffFrequency, float resonance)
    {
        auto sampleRate = 44100.0; // Assume a default sample rate
        auto q = 1.0f / (2.0f * resonance);

        filterCoefficients = juce::IIRCoefficients::makeLowPass(sampleRate, cutoffFrequency, q);
        filter.setCoefficients(filterCoefficients);
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        juce::ScopedNoDenormals noDenormals;
        auto totalNumInputChannels  = buffer.getNumChannels();

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            filter.processSamples(channelData, buffer.getNumSamples());
        }
    }

private:
    juce::IIRFilter filter;
    juce::IIRCoefficients filterCoefficients;
};
#endif /* FilterProcessor_h */
