#pragma once
#include "Common.h"
#include "PluginProcessor.h"

// Interfaz HARD VOX. Todo se dibuja sobre el fondo de referencia (Assets/background.png)
// en coordenadas de diseno 1536 x 1024; la ventana se puede escalar.
class HardVoxEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HardVoxEditor (HardVoxProcessor&);
    ~HardVoxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    using Pt = juce::Point<float>;
    using Rc = juce::Rectangle<float>;

    enum class Overlay { none, controls, chain };
    enum class Drag { none, knob, eqNode, gainBar, ovSlider, chainTile };

    struct KnobDef { int param; float cx, cy, r; bool cover; };
    struct Character { juce::Rectangle<int> rect; juce::Image img; float phase; };
    struct Particle { float x, y, vx, vy, age, life, size; };
    struct OvSlider { Rc hit, bar, label; int param; };
    struct OvToggle { Rc r; int module; };
    struct OvButton { Rc r; int id; juce::String text; };

    // utilidades de parametros
    float norm (int i) const { return params[(size_t) i]->getValue(); }
    float value (int i) const { return params[(size_t) i]->convertFrom0to1 (norm (i)); }
    void setNorm (int i, float n) { params[(size_t) i]->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, n)); }
    juce::String formatValue (int i) const;
    void resetParam (int i);

    // dibujo
    void timerCallback() override;
    void drawCharacters (juce::Graphics&);
    void drawTopBar (juce::Graphics&);
    void drawBottom (juce::Graphics&);
    void drawMeters (juce::Graphics&);
    void drawKnob (juce::Graphics&, const KnobDef&);
    void drawEq (juce::Graphics&);
    void drawGainBar (juce::Graphics&);
    void drawFlames (juce::Graphics&);
    void drawOverlayControls (juce::Graphics&);
    void drawOverlayChain (juce::Graphics&);
    void drawButton (juce::Graphics&, const Rc&, const juce::String&, bool hot = false);
    void drawSlider (juce::Graphics&, const OvSlider&);

    // interaccion
    void buildOverlayLayout();
    void triggerPresetAnimation();
    void stepPreset (int dir);
    void loadPresetByName (const juce::String& name);
    void showPresetMenu();
    void showUserListMenu (bool forDelete);
    void handleMenuResult (int r, const juce::StringArray& user);
    void showSaveDialog();
    void beginDragParam (int p) { dragParam = p; params[(size_t) p]->beginChangeGesture(); }
    void endDragParam();
    bool overlayMouseDown (Pt p);
    void getOrder (int* order) const;
    Rc tileRect (int slot) const { return { 178.0f + (float) slot * 150.0f, 330.0f, 130.0f, 150.0f }; }
    Rc tileToggle (int slot) const { auto t = tileRect (slot); return { t.getX() + 15.0f, t.getBottom() - 38.0f, 100.0f, 26.0f }; }
    int knobAt (Pt p) const;
    int eqNodeAt (Pt p) const;
    float eqX (float f) const;
    float eqY (float g) const;

    HardVoxProcessor& proc;
    std::array<juce::RangedAudioParameter*, P_COUNT> params {};
    std::vector<KnobDef> knobs;
    std::array<Character, 4> chars;
    std::vector<Particle> particles;
    std::vector<OvSlider> ovSliders;
    std::vector<OvToggle> ovToggles;
    std::vector<OvButton> ovButtons;
    std::vector<OvButton> chainButtons;
    juce::Image bgFull, bgScaled;
    juce::Random rng;

    float scale = 0.75f;
    double now = 0.0, lastTime = 0.0, animT0 = -10.0;
    int lastPresetCounter = 0;
    float levelIn = 0.0f, levelOut = 0.0f;

    Overlay overlay = Overlay::none;
    Drag drag = Drag::none;
    int dragParam = -1, dragParam2 = -1, dragNode = -1, dragSlot = -1, hoverKnob = -1;
    float dragStartNorm = 0.0f, dragX = 0.0f;
    const Rc eqRect { 243.0f, 658.0f, 250.0f, 100.0f };
    const Rc gainBarRect { 915.0f, 938.0f, 255.0f, 38.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HardVoxEditor)
};
