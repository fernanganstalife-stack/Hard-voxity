#pragma once
#include "Common.h"
#include "Params.h"
#include "Dsp.h"

class HardVoxProcessor : public juce::AudioProcessor
{
public:
    HardVoxProcessor();
    ~HardVoxProcessor() override = default;

    // --- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "HARD VOX"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- Estado compartido con la interfaz
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f };   // picos (la GUI los lee y pone a 0)
    std::atomic<int> presetChangeCounter { 0 };                // la GUI lanza la animacion al cambiar
    double currentSampleRate = 44100.0;

    // --- Cadena de efectos
    uint32_t getChain() const { return chainPacked.load(); }
    void setChain (uint32_t packed);

    // --- Presets (solo desde el hilo de mensajes / GUI)
    static int getNumFactoryPresets();
    static juce::String getFactoryName (int index);
    void applyFactoryPreset (int index);
    void resetToDefault();
    juce::File getUserPresetDir() const;
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    juce::String getCurrentPresetName() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setParamValue (int index, float value);
    void setEnable (int module, bool on);
    void readChainFromState();
    void runModule (int id, float* l, float* r, int n, const float* v);
    juce::File presetFile (const juce::String& name) const;

    std::atomic<uint32_t> chainPacked { kStandardChain };
    std::atomic<float>* raw[P_COUNT] {};
    std::atomic<float>* rawEn[M_COUNT] {};

    juce::SmoothedValue<float> sm[P_COUNT];
    juce::SmoothedValue<float> enSm[M_COUNT];
    float prevE[M_COUNT] {};
    float prevMaster = 1.0f;

    uint32_t appliedChain = kStandardChain;
    int chainFadeState = 0; // 0 normal, 1 fundido de salida, 2 fundido de entrada

    Compressor comp; EQ eq; Bright bright; Distortion dist; Chorus chorus; ReverbFx reverb; DelayFx delay; Widener widener;
    std::vector<float> scratchL, scratchR;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardVoxProcessor)
};
