#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    constexpr float DW = 1536.0f, DH = 1024.0f;
    constexpr float kA0 = -2.356194f, kA1 = 2.356194f;
    const juce::Colour kRed (0xffff2a2a), kDark (0xff0b0507), kPanel (0xff140709);
    const float kAnimLen = 1.25f;

    juce::Image makeCrop (const juce::Image& src, juce::Rectangle<int> r, float feather)
    {
        const float s = (float) src.getWidth() / DW;
        const juce::Rectangle<int> pr ((int) (r.getX() * s), (int) (r.getY() * s),
                                       (int) (r.getWidth() * s), (int) (r.getHeight() * s));
        juce::Image out (juce::Image::ARGB, pr.getWidth(), pr.getHeight(), true);
        juce::Image::BitmapData in (src, pr.getX(), pr.getY(), pr.getWidth(), pr.getHeight(), juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData od (out, juce::Image::BitmapData::writeOnly);
        const int w = pr.getWidth(), h = pr.getHeight();
        const float f = std::max (1.0f, feather * s);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                const float d = (float) std::min (std::min (x, w - 1 - x), std::min (y, h - 1 - y));
                float t = juce::jlimit (0.0f, 1.0f, d / f);
                t = t * t * (3.0f - 2.0f * t);
                od.setPixelColour (x, y, in.getPixelColour (x, y).withAlpha (t));
            }
        return out;
    }

    juce::Font fnt (float h, bool bold = false) { return juce::Font (h, bold ? juce::Font::bold : juce::Font::plain); }
}

// ---------------------------------------------------------------------------
HardVoxEditor::HardVoxEditor (HardVoxProcessor& p) : juce::AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < P_COUNT; ++i) params[(size_t) i] = proc.apvts.getParameter (kParams[i].id);

    bgFull = juce::ImageCache::getFromMemory (BinaryData::background_png, BinaryData::background_pngSize);
    if (bgFull.isNull()) bgFull = juce::Image (juce::Image::RGB, 1536, 1024, true);
    bgFull = bgFull.convertedToFormat (juce::Image::RGB);

    knobs = {
        { P_C_MKU, 118, 712, 50, true },  { P_C_THR, 72, 803, 26, true },   { P_C_RAT, 162, 803, 26, true },
        { P_E_LO, 278, 803, 28, true },   { P_E_MID, 368, 803, 28, true },  { P_E_HI, 458, 803, 28, true },
        { P_B_BR, 598, 720, 48, true },   { P_D_SAT, 771, 725, 50, true },  { P_R_DEC, 940, 722, 44, true },
        { P_DL_FB, 1107, 722, 44, true }, { P_CH_MIX, 1265, 722, 44, true },{ P_W_AMT, 1435, 725, 46, true },
        { P_M_GAIN, 845, 957, 30, true },
        // anillos sobre las etiquetas dibujadas en el fondo (AIR, DRIVE, MIX, TIME, DEPTH, WIDTH)
        { P_B_AIR, 598, 806, 30, false }, { P_D_DRV, 771, 815, 32, false }, { P_R_MIX, 940, 806, 30, false },
        { P_DL_TIME, 1107, 808, 30, false }, { P_CH_DEP, 1265, 806, 30, false }, { P_W_WID, 1435, 806, 30, false },
    };

    const juce::Rectangle<int> rects[4] = { { 150, 120, 260, 210 }, { 430, 100, 310, 230 },
                                            { 830, 90, 260, 250 },  { 1100, 125, 300, 270 } };
    for (int i = 0; i < 4; ++i)
    {
        chars[(size_t) i].rect = rects[i];
        chars[(size_t) i].img = makeCrop (bgFull, rects[i], 22.0f);
        chars[(size_t) i].phase = 1.7f * (float) i;
    }

    particles.reserve (256);
    buildOverlayLayout();
    lastPresetCounter = proc.presetChangeCounter.load();

    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (768, 512, 1536, 1024);
    getConstrainer()->setFixedAspectRatio (1.5);
    setSize (1152, 768);
    startTimerHz (30);
}

HardVoxEditor::~HardVoxEditor() { stopTimer(); }

void HardVoxEditor::resized()
{
    scale = (float) getWidth() / DW;
    bgScaled = bgFull.rescaled (getWidth(), getHeight(), juce::Graphics::highResamplingQuality);
}

juce::String HardVoxEditor::formatValue (int i) const
{
    return juce::String (value (i), kParams[i].decimals) + kParams[i].unit;
}

void HardVoxEditor::resetParam (int i)
{
    params[(size_t) i]->beginChangeGesture();
    params[(size_t) i]->setValueNotifyingHost (params[(size_t) i]->convertTo0to1 (kParams[i].def));
    params[(size_t) i]->endChangeGesture();
}

void HardVoxEditor::endDragParam()
{
    if (dragParam >= 0) params[(size_t) dragParam]->endChangeGesture();
    if (dragParam2 >= 0) params[(size_t) dragParam2]->endChangeGesture();
    dragParam = dragParam2 = -1;
}

void HardVoxEditor::getOrder (int* order) const
{
    if (! unpackChain (proc.getChain(), order)) unpackChain (kStandardChain, order);
}

// ------------------------------ animacion -----------------------------------
void HardVoxEditor::triggerPresetAnimation()
{
    animT0 = now;
    for (int i = 0; i < 90; ++i)
    {
        Particle p;
        p.x = 60.0f + rng.nextFloat() * 1416.0f;
        p.y = 540.0f + rng.nextFloat() * 110.0f;
        p.vx = (rng.nextFloat() - 0.5f) * 60.0f;
        p.vy = -(70.0f + rng.nextFloat() * 220.0f);
        p.age = -rng.nextFloat() * 0.25f;
        p.life = 0.6f + rng.nextFloat() * 0.6f;
        p.size = 14.0f + rng.nextFloat() * 30.0f;
        if (particles.size() < 240) particles.push_back (p);
    }
}

void HardVoxEditor::timerCallback()
{
    now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    const float dt = (float) juce::jlimit (0.0, 0.1, now - lastTime);
    lastTime = now;

    const int c = proc.presetChangeCounter.load();
    if (c != lastPresetCounter) { lastPresetCounter = c; triggerPresetAnimation(); }

    levelIn  = std::max (proc.inLevel.exchange (0.0f),  levelIn * 0.82f);
    levelOut = std::max (proc.outLevel.exchange (0.0f), levelOut * 0.82f);

    for (auto& p : particles) { p.age += dt; if (p.age > 0) { p.x += p.vx * dt; p.y += p.vy * dt; } }
    particles.erase (std::remove_if (particles.begin(), particles.end(), [] (const Particle& p) { return p.age >= p.life; }),
                     particles.end());
    repaint();
}

void HardVoxEditor::drawCharacters (juce::Graphics& g)
{
    const float imgInv = DW / (float) bgFull.getWidth();
    const float t = (float) now;

    for (size_t i = 0; i < 4; ++i)
    {
        auto& ch = chars[i];
        const float ph = ch.phase;
        const float breath = std::sin (t * 1.6f + ph);
        float dx = std::sin (t * 0.9f + ph) * 1.2f;           // movimiento sutil de cuerpo
        float dy = breath * 1.8f;                              // respiracion
        float rot = std::sin (t * 0.7f + ph * 1.7f) * 0.012f;  // pequenos movimientos de cabeza
        float sc = 1.0f + breath * 0.004f;
        float shear = std::sin (t * 1.1f + ph) * 0.012f;       // "pelo"

        // animacion al cambiar de preset (coordinada, con ligero desfase por personaje)
        const float at = (float) (now - animT0) - 0.04f * (float) i;
        float glow = 0.0f;
        if (at >= 0.0f && at < kAnimLen)
        {
            const float e = std::exp (-3.0f * at);
            glow = e;
            switch (i)
            {
                case 0: rot += -0.10f * e * std::sin (at * 26.0f); dy += 12.0f * e * std::sin (at * 26.0f + 1.0f); break;  // cabezazos
                case 1: dx += 15.0f * e * std::sin (at * 38.0f); rot += 0.02f * e * std::sin (at * 38.0f); break;           // sacude la cabeza
                case 2: sc += 0.07f * e; dy -= 8.0f * e; rot += 0.06f * e * std::sin (at * 9.0f); break;                    // pose agresiva
                default: rot += 0.07f * e * std::sin (at * 14.0f); shear += 0.05f * e * std::sin (at * 18.0f);
                         dx += 8.0f * e * std::sin (at * 10.0f); break;                                                      // cabeza y pelo
            }
        }

        const float px = (float) ch.rect.getCentreX(), py = (float) ch.rect.getBottom();
        const auto tf = juce::AffineTransform::scale (imgInv)
                            .translated ((float) ch.rect.getX(), (float) ch.rect.getY())
                            .translated (-px, -py).sheared (shear, 0.0f).scaled (sc).rotated (rot)
                            .translated (px + dx, py + dy);
        g.drawImageTransformed (ch.img, tf);

        if (glow > 0.01f)
        {
            const float cx = (float) ch.rect.getCentreX(), cy = (float) ch.rect.getCentreY();
            const float rr = (float) ch.rect.getWidth() * 0.65f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff2010).withAlpha (0.28f * glow), cx, cy,
                                                     juce::Colour (0x00ff2010), cx + rr, cy, true));
            g.fillEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
        }
    }
}

void HardVoxEditor::drawFlames (juce::Graphics& g)
{
    for (const auto& p : particles)
    {
        if (p.age < 0.0f) continue;
        const float k = 1.0f - p.age / p.life;
        const auto inner = juce::Colour::fromFloatRGBA (1.0f, 0.35f + 0.4f * k, 0.1f, 0.75f * k);
        g.setGradientFill (juce::ColourGradient (inner, p.x, p.y, juce::Colour::fromFloatRGBA (0.9f, 0.05f, 0.05f, 0.0f),
                                                 p.x + p.size * k, p.y, true));
        const float s = p.size * (0.5f + 0.5f * k);
        g.fillEllipse (p.x - s, p.y - s, s * 2.0f, s * 2.0f);
    }
}

// -------------------------------- dibujo ------------------------------------
void HardVoxEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (bgScaled, 0, 0);
    g.addTransform (juce::AffineTransform::scale (scale));

    drawCharacters (g);
    drawFlames (g);
    drawTopBar (g);
    drawMeters (g);
    drawEq (g);
    for (const auto& k : knobs) drawKnob (g, k);
    drawGainBar (g);
    drawBottom (g);

    if (drag == Drag::knob && dragParam >= 0)       // burbuja con el valor mientras se arrastra
    {
        for (const auto& k : knobs)
            if (k.param == dragParam)
            {
                const Rc r (k.cx - 80.0f, k.cy - k.r - 38.0f, 160.0f, 26.0f);
                g.setColour (juce::Colour (0xee000000)); g.fillRoundedRectangle (r, 6.0f);
                g.setColour (kRed); g.drawRoundedRectangle (r, 6.0f, 1.2f);
                g.setColour (juce::Colours::white); g.setFont (fnt (15.0f, true));
                g.drawText (juce::String (kParams[dragParam].name).toUpperCase() + "  " + formatValue (dragParam),
                            r, juce::Justification::centred, false);
                break;
            }
    }

    if (overlay == Overlay::controls) drawOverlayControls (g);
    if (overlay == Overlay::chain)    drawOverlayChain (g);
}

void HardVoxEditor::drawTopBar (juce::Graphics& g)
{
    // nombre del preset (cubre el texto del fondo)
    g.setColour (kDark); g.fillRect (Rc (600.0f, 6.0f, 330.0f, 38.0f));
    g.setColour (juce::Colour (0xffe6dcdc)); g.setFont (fnt (22.0f, true));
    g.drawText ("Preset:  " + proc.getCurrentPresetName().toUpperCase(), Rc (600.0f, 6.0f, 330.0f, 38.0f),
                juce::Justification::centred, true);

    // botones CONTROLES / CADENA
    g.setColour (juce::Colour (0xf00a0406)); g.fillRoundedRectangle (Rc (1030.0f, 6.0f, 340.0f, 38.0f), 6.0f);
    const bool custom = proc.getChain() != kStandardChain;
    drawButton (g, Rc (1038.0f, 10.0f, 150.0f, 30.0f), "CONTROLES", overlay == Overlay::controls);
    drawButton (g, Rc (1200.0f, 10.0f, 160.0f, 30.0f), custom ? "CADENA: CUSTOM" : "CADENA: STD", overlay == Overlay::chain);
}

void HardVoxEditor::drawBottom (juce::Graphics& g)
{
    g.setColour (kDark); g.fillRect (Rc (386.0f, 908.0f, 306.0f, 52.0f));
    g.setColour (juce::Colour (0xffe6dcdc));
    g.setFont (fnt (19.0f)); g.drawText ("PRESET INFO:", Rc (388.0f, 910.0f, 300.0f, 22.0f), juce::Justification::left, false);
    g.setFont (fnt (16.0f)); g.drawText ("Hard Vox Preset - " + proc.getCurrentPresetName(), Rc (388.0f, 932.0f, 300.0f, 22.0f),
                                         juce::Justification::left, true);
}

void HardVoxEditor::drawButton (juce::Graphics& g, const Rc& r, const juce::String& text, bool hot)
{
    g.setGradientFill (juce::ColourGradient (hot ? juce::Colour (0xff7a0f16) : juce::Colour (0xff3a0a0e), r.getX(), r.getY(),
                                             juce::Colour (0xff120406), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (juce::Colour (0xffb01820)); g.drawRoundedRectangle (r, 5.0f, 1.4f);
    g.setColour (juce::Colour (0xfff0e6e6)); g.setFont (fnt (r.getHeight() * 0.5f, true));
    g.drawText (text, r, juce::Justification::centred, false);
}

void HardVoxEditor::drawMeters (juce::Graphics& g)
{
    auto draw = [&] (float x, float level)
    {
        const Rc area (x, 286.0f, 40.0f, 230.0f);
        g.setColour (juce::Colour (0xff0d0406)); g.fillRect (area.expanded (3.0f));
        const int segs = 22;
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        const float sh = area.getHeight() / (float) segs;
        for (int i = 0; i < segs; ++i)
        {
            const float f = (float) i / (float) segs;
            const bool lit = frac > f;
            const auto col = lit ? juce::Colour (0xff7a0a12).interpolatedWith (juce::Colour (0xffff5a4a), f)
                                 : juce::Colour (0xff2a0a0e);
            g.setColour (col);
            g.fillRect (Rc (area.getX(), area.getBottom() - (float) (i + 1) * sh + 1.5f, area.getWidth(), sh - 3.0f));
        }
    };
    draw (28.0f, levelIn);
    draw (1471.0f, levelOut);
}

void HardVoxEditor::drawKnob (juce::Graphics& g, const KnobDef& k)
{
    const float n = norm (k.param);
    const float ang = kA0 + (kA1 - kA0) * n;
    const float cx = k.cx, cy = k.cy;

    if (k.cover)
    {
        const float rd = k.r * 0.78f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3c3c42), cx - rd * 0.35f, cy - rd * 0.45f,
                                                 juce::Colour (0xff050506), cx + rd, cy + rd, true));
        g.fillEllipse (cx - rd, cy - rd, rd * 2.0f, rd * 2.0f);
        g.setColour (juce::Colour (0xff8a1018)); g.drawEllipse (cx - rd, cy - rd, rd * 2.0f, rd * 2.0f, 1.5f);
        g.setColour (kRed);
        g.drawLine (cx + std::sin (ang) * rd * 0.25f, cy - std::cos (ang) * rd * 0.25f,
                    cx + std::sin (ang) * rd * 0.90f, cy - std::cos (ang) * rd * 0.90f, 3.2f);
    }

    const float ra = k.cover ? k.r * 0.93f : k.r;
    juce::Path track; track.addCentredArc (cx, cy, ra, ra, 0.0f, kA0, kA1, true);
    g.setColour (juce::Colour (k.cover ? 0x66000000 : 0x66aa1a1a));
    g.strokePath (track, juce::PathStrokeType (k.cover ? 5.0f : 3.0f));

    if (n > 0.002f)
    {
        juce::Path val; val.addCentredArc (cx, cy, ra, ra, 0.0f, kA0, ang, true);
        g.setColour (kRed.withAlpha (0.28f));
        g.strokePath (val, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (kRed);
        g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    if (! k.cover)
    {
        g.setColour (juce::Colour (0xffffd0c8));
        g.fillEllipse (cx + std::sin (ang) * ra - 3.5f, cy - std::cos (ang) * ra - 3.5f, 7.0f, 7.0f);
    }
}

float HardVoxEditor::eqX (float f) const { return eqRect.getX() + eqRect.getWidth() * std::log (f / 20.0f) / std::log (1000.0f); }
float HardVoxEditor::eqY (float gdb) const { return eqRect.getCentreY() - gdb / 15.0f * (eqRect.getHeight() * 0.5f - 8.0f); }

void HardVoxEditor::drawEq (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff0b0507)); g.fillRect (eqRect);
    g.setColour (juce::Colour (0xff3a1014));
    for (float f : { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f })
        g.drawVerticalLine ((int) eqX (f), eqRect.getY(), eqRect.getBottom());
    for (float db : { -10.0f, -5.0f, 5.0f, 10.0f })
        g.drawHorizontalLine ((int) eqY (db), eqRect.getX(), eqRect.getRight());
    g.setColour (juce::Colour (0xff7a1a20)); g.drawHorizontalLine ((int) eqY (0.0f), eqRect.getX(), eqRect.getRight());

    const double sr = proc.currentSampleRate;
    Biquad b[4];
    b[0].makeLowShelf (sr, value (P_E_LOF), value (P_E_LO));
    b[1].makePeak (sr, value (P_E_MIDF), 0.9, value (P_E_MID));
    b[2].makeHighShelf (sr, value (P_E_HIF), value (P_E_HI));
    b[3].makeHighShelf (sr, 12000.0, value (P_E_AIR));

    juce::Path curve, fill;
    const float cy = eqY (0.0f);
    for (float x = 0.0f; x <= eqRect.getWidth(); x += 2.0f)
    {
        const double f = 20.0 * std::pow (1000.0, (double) x / (double) eqRect.getWidth());
        double db = 0.0;
        for (auto& bq : b) db += bq.magnitudeDb (f, sr);
        const float y = juce::jlimit (eqRect.getY() + 2.0f, eqRect.getBottom() - 2.0f, eqY ((float) db));
        const float px = eqRect.getX() + x;
        if (x == 0.0f) { curve.startNewSubPath (px, y); fill.startNewSubPath (px, cy); fill.lineTo (px, y); }
        else           { curve.lineTo (px, y); fill.lineTo (px, y); }
    }
    fill.lineTo (eqRect.getRight(), cy); fill.closeSubPath();
    g.setColour (kRed.withAlpha (0.22f)); g.fillPath (fill);
    g.setColour (kRed); g.strokePath (curve, juce::PathStrokeType (2.0f));

    const int gp[4] = { P_E_LO, P_E_MID, P_E_HI, P_E_AIR };
    const int fp[4] = { P_E_LOF, P_E_MIDF, P_E_HIF, -1 };
    for (int i = 0; i < 4; ++i)
    {
        const float x = eqX (fp[i] >= 0 ? value (fp[i]) : 12000.0f), y = eqY (value (gp[i]));
        g.setColour (i == dragNode && drag == Drag::eqNode ? juce::Colours::white : juce::Colour (0xffff6a5a));
        g.fillEllipse (x - 5.5f, y - 5.5f, 11.0f, 11.0f);
        g.setColour (juce::Colour (0xff200406)); g.drawEllipse (x - 5.5f, y - 5.5f, 11.0f, 11.0f, 1.5f);
    }
}

void HardVoxEditor::drawGainBar (juce::Graphics& g)
{
    const Rc inner = gainBarRect.reduced (5.0f, 6.0f);
    g.setColour (juce::Colour (0xff0a0305)); g.fillRect (inner);
    const float n = norm (P_M_GAIN);
    const Rc fillR = inner.withWidth (inner.getWidth() * n);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7a0a12), fillR.getX(), 0.0f, juce::Colour (0xffff3a2a), inner.getRight(), 0.0f, false));
    g.fillRect (fillR);
    g.setColour (juce::Colour (0xff0a0305));
    for (float x = inner.getX() + 6.0f; x < inner.getRight(); x += 7.0f) g.drawVerticalLine ((int) x, inner.getY(), inner.getBottom());
    g.setColour (juce::Colours::white); g.fillRect (Rc (fillR.getRight() - 2.0f, inner.getY() - 2.0f, 3.0f, inner.getHeight() + 4.0f));
    g.setColour (juce::Colour (0xffe6dcdc)); g.setFont (fnt (16.0f, true));
    g.drawText (formatValue (P_M_GAIN), Rc (1010.0f, 912.0f, 160.0f, 24.0f), juce::Justification::right, false);
}

// ------------------------------ interaccion ---------------------------------
int HardVoxEditor::knobAt (Pt p) const
{
    int best = -1; float bestScore = 1.0f;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const float d = p.getDistanceFrom (Pt (knobs[i].cx, knobs[i].cy)) / (knobs[i].r * 1.08f);
        if (d < bestScore) { bestScore = d; best = (int) i; }
    }
    return best;
}

int HardVoxEditor::eqNodeAt (Pt p) const
{
    const int gp[4] = { P_E_LO, P_E_MID, P_E_HI, P_E_AIR };
    const int fp[4] = { P_E_LOF, P_E_MIDF, P_E_HIF, -1 };
    for (int i = 0; i < 4; ++i)
        if (p.getDistanceFrom (Pt (eqX (fp[i] >= 0 ? value (fp[i]) : 12000.0f), eqY (value (gp[i])))) < 14.0f) return i;
    return -1;
}

void HardVoxEditor::mouseDown (const juce::MouseEvent& e)
{
    const Pt p = e.position / scale;
    dragStartNorm = 0.0f;

    if (overlay != Overlay::none) { overlayMouseDown (p); return; }

    if (p.getDistanceFrom (Pt (563.0f, 25.0f)) < 28.0f) { stepPreset (-1); return; }
    if (p.getDistanceFrom (Pt (968.0f, 25.0f)) < 28.0f) { stepPreset (+1); return; }
    if (Rc (600.0f, 6.0f, 330.0f, 38.0f).contains (p)) { showPresetMenu(); return; }
    if (Rc (1038.0f, 10.0f, 150.0f, 30.0f).contains (p)) { overlay = Overlay::controls; return; }
    if (Rc (1200.0f, 10.0f, 160.0f, 30.0f).contains (p)) { overlay = Overlay::chain; return; }

    const int node = eqNodeAt (p);
    if (node >= 0)
    {
        const int gp[4] = { P_E_LO, P_E_MID, P_E_HI, P_E_AIR };
        const int fp[4] = { P_E_LOF, P_E_MIDF, P_E_HIF, -1 };
        dragNode = node; drag = Drag::eqNode;
        beginDragParam (gp[node]);
        if (fp[node] >= 0) { dragParam2 = fp[node]; params[(size_t) dragParam2]->beginChangeGesture(); }
        return;
    }
    if (gainBarRect.contains (p))
    {
        drag = Drag::gainBar; beginDragParam (P_M_GAIN);
        setNorm (P_M_GAIN, (p.x - (gainBarRect.getX() + 5.0f)) / (gainBarRect.getWidth() - 10.0f));
        return;
    }
    const int k = knobAt (p);
    if (k >= 0)
    {
        drag = Drag::knob; beginDragParam (knobs[(size_t) k].param);
        dragStartNorm = norm (dragParam);
    }
}

void HardVoxEditor::mouseDrag (const juce::MouseEvent& e)
{
    const Pt p = e.position / scale;
    switch (drag)
    {
        case Drag::knob:
        {
            const float fine = e.mods.isShiftDown() ? 0.15f : 1.0f;
            setNorm (dragParam, dragStartNorm - (float) e.getDistanceFromDragStartY() / (scale * 220.0f) * fine);
            break;
        }
        case Drag::eqNode:
        {
            const int gp[4] = { P_E_LO, P_E_MID, P_E_HI, P_E_AIR };
            const float gdb = (eqRect.getCentreY() - p.y) / (eqRect.getHeight() * 0.5f - 8.0f) * 15.0f;
            params[(size_t) gp[dragNode]]->setValueNotifyingHost (params[(size_t) gp[dragNode]]->convertTo0to1 (juce::jlimit (-15.0f, 15.0f, gdb)));
            if (dragParam2 >= 0)
            {
                const float f = 20.0f * std::pow (1000.0f, (p.x - eqRect.getX()) / eqRect.getWidth());
                params[(size_t) dragParam2]->setValueNotifyingHost (params[(size_t) dragParam2]->convertTo0to1 (f));
            }
            break;
        }
        case Drag::gainBar:
            setNorm (P_M_GAIN, (p.x - (gainBarRect.getX() + 5.0f)) / (gainBarRect.getWidth() - 10.0f));
            break;
        case Drag::ovSlider:
            for (const auto& s : ovSliders)
                if (s.param == dragParam) setNorm (dragParam, (p.x - s.bar.getX()) / s.bar.getWidth());
            break;
        case Drag::chainTile:
            dragX = p.x; break;
        default: break;
    }
}

void HardVoxEditor::mouseUp (const juce::MouseEvent&)
{
    if (drag == Drag::chainTile)
    {
        int order[M_COUNT]; getOrder (order);
        const int target = juce::jlimit (0, M_COUNT - 1, (int) ((dragX - 178.0f + 10.0f) / 150.0f));
        if (target != dragSlot && dragSlot >= 0)
        {
            std::vector<int> o (order, order + M_COUNT);
            const int m = o[(size_t) dragSlot];
            o.erase (o.begin() + dragSlot);
            o.insert (o.begin() + target, m);
            proc.setChain (packChain (o.data()));
        }
    }
    endDragParam();
    drag = Drag::none; dragNode = -1; dragSlot = -1;
}

void HardVoxEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const Pt p = e.position / scale;
    if (overlay == Overlay::controls)
    {
        for (const auto& s : ovSliders) if (s.hit.contains (p)) { resetParam (s.param); return; }
        return;
    }
    if (overlay != Overlay::none) return;

    const int node = eqNodeAt (p);
    if (node >= 0) { const int gp[4] = { P_E_LO, P_E_MID, P_E_HI, P_E_AIR }; resetParam (gp[node]); return; }
    if (gainBarRect.contains (p)) { resetParam (P_M_GAIN); return; }
    const int k = knobAt (p);
    if (k >= 0) resetParam (knobs[(size_t) k].param);
}

void HardVoxEditor::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (overlay != Overlay::none) return;
    const int k = knobAt (e.position / scale);
    if (k < 0) return;
    const int pi = knobs[(size_t) k].param;
    params[(size_t) pi]->beginChangeGesture();
    setNorm (pi, norm (pi) + w.deltaY * 0.15f);
    params[(size_t) pi]->endChangeGesture();
}

// -------------------------------- presets -----------------------------------
void HardVoxEditor::loadPresetByName (const juce::String& name)
{
    for (int i = 0; i < HardVoxProcessor::getNumFactoryPresets(); ++i)
        if (HardVoxProcessor::getFactoryName (i) == name) { proc.applyFactoryPreset (i); return; }
    proc.loadUserPreset (name);
}

void HardVoxEditor::stepPreset (int dir)
{
    juce::StringArray all;
    for (int i = 0; i < HardVoxProcessor::getNumFactoryPresets(); ++i) all.add (HardVoxProcessor::getFactoryName (i));
    all.addArray (proc.getUserPresetNames());
    int idx = all.indexOf (proc.getCurrentPresetName());
    if (idx < 0) idx = 0;
    idx = (idx + dir + all.size()) % all.size();
    loadPresetByName (all[idx]);
}

void HardVoxEditor::showPresetMenu()
{
    juce::PopupMenu m;
    const auto cur = proc.getCurrentPresetName();
    for (int i = 0; i < HardVoxProcessor::getNumFactoryPresets(); ++i)
        m.addItem (1 + i, HardVoxProcessor::getFactoryName (i), true, cur == HardVoxProcessor::getFactoryName (i));

    const auto user = proc.getUserPresetNames();
    if (user.size() > 0)
    {
        m.addSeparator();
        m.addSectionHeader ("Mis presets");
        for (int i = 0; i < user.size(); ++i) m.addItem (100 + i, user[i], true, cur == user[i]);
    }
    m.addSeparator();
    m.addItem (1000, "Guardar preset actual...");
    if (user.size() > 0)
    {
        juce::PopupMenu del;
        for (int i = 0; i < user.size(); ++i) del.addItem (2000 + i, user[i]);
        m.addSubMenu ("Borrar preset", del);
    }
    m.addItem (3000, "Resetear a DEFAULT");

    const auto area = localAreaToGlobal (juce::Rectangle<int> ((int) (600 * scale), (int) (40 * scale), (int) (330 * scale), 1));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (area),
                     [safe = juce::Component::SafePointer<HardVoxEditor> (this), user] (int r)
                     { if (safe != nullptr && r != 0) safe->handleMenuResult (r, user); });
}

void HardVoxEditor::showUserListMenu (bool forDelete)
{
    const auto user = proc.getUserPresetNames();
    juce::PopupMenu m;
    if (user.isEmpty()) m.addItem (-1, "(no hay presets propios)", false);
    for (int i = 0; i < user.size(); ++i) m.addItem ((forDelete ? 2000 : 100) + i, user[i]);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [safe = juce::Component::SafePointer<HardVoxEditor> (this), user] (int r)
                     { if (safe != nullptr && r > 0) safe->handleMenuResult (r, user); });
}

void HardVoxEditor::handleMenuResult (int r, const juce::StringArray& user)
{
    const int nf = HardVoxProcessor::getNumFactoryPresets();
    if (r >= 1 && r <= nf)                         proc.applyFactoryPreset (r - 1);
    else if (r >= 100 && r < 100 + user.size())    proc.loadUserPreset (user[r - 100]);
    else if (r == 1000)                            showSaveDialog();
    else if (r >= 2000 && r < 2000 + user.size())  proc.deleteUserPreset (user[r - 2000]);
    else if (r == 3000)                            proc.resetToDefault();
}

void HardVoxEditor::showSaveDialog()
{
    auto* aw = new juce::AlertWindow ("Guardar preset", "Escribe un nombre para tu preset:", juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", "", "Nombre:");
    aw->addButton ("Guardar", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancelar", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true,
        juce::ModalCallbackFunction::create ([safe = juce::Component::SafePointer<HardVoxEditor> (this), aw] (int result)
        {
            if (result == 1 && safe != nullptr) safe->proc.saveUserPreset (aw->getTextEditorContents ("name"));
            delete aw;
        }),
        false);
}

// -------------------------------- overlays ----------------------------------
void HardVoxEditor::buildOverlayLayout()
{
    ovSliders.clear(); ovToggles.clear(); ovButtons.clear(); chainButtons.clear();
    const int mods[2][4] = { { M_COMP, M_EQ, M_BRIGHT, M_DIST }, { M_CHORUS, M_REVERB, M_DELAY, M_WIDENER } };
    const float rowY[2] = { 110.0f, 440.0f };

    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 4; ++col)
        {
            const int m = mods[row][col];
            const float x = 150.0f + (float) col * 318.0f, y = rowY[row];
            ovToggles.push_back ({ Rc (x + 296.0f - 62.0f, y + 2.0f, 60.0f, 22.0f), m });
            float sy = y + 36.0f;
            for (int i = 0; i < P_COUNT; ++i)
                if (kParams[i].module == m)
                {
                    ovSliders.push_back ({ Rc (x, sy, 296.0f, 32.0f), Rc (x, sy + 19.0f, 296.0f, 10.0f), Rc (x, sy, 296.0f, 17.0f), i });
                    sy += 36.0f;
                }
        }
    ovSliders.push_back ({ Rc (150.0f, 620.0f, 296.0f, 32.0f), Rc (150.0f, 639.0f, 296.0f, 10.0f), Rc (150.0f, 620.0f, 296.0f, 17.0f), P_M_GAIN });

    const char* names[5] = { "RESET DEFAULT", "GUARDAR PRESET", "CARGAR PRESET", "BORRAR PRESET", "CERRAR" };
    for (int i = 0; i < 5; ++i) ovButtons.push_back ({ Rc (150.0f + (float) i * 240.0f, 700.0f, 220.0f, 40.0f), i, names[i] });

    chainButtons.push_back ({ Rc (178.0f, 560.0f, 280.0f, 44.0f), 0, "ORDEN ESTANDAR" });
    chainButtons.push_back ({ Rc (1160.0f, 560.0f, 198.0f, 44.0f), 1, "CERRAR" });
}

bool HardVoxEditor::overlayMouseDown (Pt p)
{
    if (overlay == Overlay::controls)
    {
        for (const auto& b : ovButtons)
            if (b.r.contains (p))
            {
                switch (b.id)
                {
                    case 0: proc.resetToDefault(); break;
                    case 1: showSaveDialog(); break;
                    case 2: showUserListMenu (false); break;
                    case 3: showUserListMenu (true); break;
                    default: overlay = Overlay::none; break;
                }
                return true;
            }
        for (const auto& t : ovToggles)
            if (t.r.contains (p))
            {
                auto* ep = proc.apvts.getParameter (kEnableIds[t.module]);
                ep->beginChangeGesture(); ep->setValueNotifyingHost (ep->getValue() > 0.5f ? 0.0f : 1.0f); ep->endChangeGesture();
                return true;
            }
        for (const auto& s : ovSliders)
            if (s.hit.contains (p))
            {
                drag = Drag::ovSlider; beginDragParam (s.param);
                setNorm (s.param, (p.x - s.bar.getX()) / s.bar.getWidth());
                return true;
            }
    }
    else if (overlay == Overlay::chain)
    {
        for (const auto& b : chainButtons)
            if (b.r.contains (p))
            {
                if (b.id == 0) proc.setChain (kStandardChain); else overlay = Overlay::none;
                return true;
            }
        int order[M_COUNT]; getOrder (order);
        for (int s = 0; s < M_COUNT; ++s)
        {
            if (tileToggle (s).contains (p))
            {
                auto* ep = proc.apvts.getParameter (kEnableIds[order[s]]);
                ep->beginChangeGesture(); ep->setValueNotifyingHost (ep->getValue() > 0.5f ? 0.0f : 1.0f); ep->endChangeGesture();
                return true;
            }
            if (tileRect (s).contains (p)) { drag = Drag::chainTile; dragSlot = s; dragX = p.x; return true; }
        }
    }
    return false;
}

void HardVoxEditor::drawSlider (juce::Graphics& g, const OvSlider& s)
{
    const float n = norm (s.param);
    g.setColour (juce::Colour (0xffd8cccc)); g.setFont (fnt (14.0f));
    g.drawText (kParams[s.param].name, s.label, juce::Justification::left, false);
    g.setColour (juce::Colour (0xffff8a7a));
    g.drawText (formatValue (s.param), s.label, juce::Justification::right, false);
    g.setColour (juce::Colour (0xff2a0a0e)); g.fillRoundedRectangle (s.bar, 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7a0a12), s.bar.getX(), 0.0f, juce::Colour (0xffff3a2a), s.bar.getRight(), 0.0f, false));
    g.fillRoundedRectangle (s.bar.withWidth (std::max (4.0f, s.bar.getWidth() * n)), 4.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (s.bar.getX() + s.bar.getWidth() * n - 6.0f, s.bar.getCentreY() - 6.0f, 12.0f, 12.0f);
}

void HardVoxEditor::drawOverlayControls (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xe6070204)); g.fillRect (0.0f, 0.0f, DW, DH);
    const Rc panel (120.0f, 70.0f, 1296.0f, 700.0f);
    g.setColour (kPanel); g.fillRoundedRectangle (panel, 12.0f);
    g.setColour (juce::Colour (0xffb01820)); g.drawRoundedRectangle (panel, 12.0f, 2.0f);

    const float rowY[2] = { 110.0f, 440.0f };
    const int mods[2][4] = { { M_COMP, M_EQ, M_BRIGHT, M_DIST }, { M_CHORUS, M_REVERB, M_DELAY, M_WIDENER } };
    for (int row = 0; row < 2; ++row)
        for (int col = 0; col < 4; ++col)
        {
            const float x = 150.0f + (float) col * 318.0f;
            g.setColour (juce::Colour (0xfff0e6e6)); g.setFont (fnt (20.0f, true));
            g.drawText (kModuleNames[mods[row][col]], Rc (x, rowY[row], 220.0f, 26.0f), juce::Justification::left, false);
            g.setColour (juce::Colour (0xff7a1a20)); g.drawHorizontalLine ((int) (rowY[row] + 30.0f), x, x + 296.0f);
        }
    for (const auto& t : ovToggles)
    {
        const bool on = proc.apvts.getParameter (kEnableIds[t.module])->getValue() > 0.5f;
        drawButton (g, t.r, on ? "ON" : "OFF", on);
    }
    for (const auto& s : ovSliders) drawSlider (g, s);
    g.setColour (juce::Colour (0xfff0e6e6)); g.setFont (fnt (20.0f, true));
    g.drawText ("MASTER", Rc (150.0f, 590.0f, 200.0f, 26.0f), juce::Justification::left, false);
    for (const auto& b : ovButtons) drawButton (g, b.r, b.text);

    g.setColour (juce::Colour (0xff9a8a8a)); g.setFont (fnt (13.0f));
    g.drawText ("Doble clic en un control = valor por defecto.  Shift + arrastrar en una perilla = ajuste fino.",
                Rc (150.0f, 746.0f, 1100.0f, 18.0f), juce::Justification::left, false);
}

void HardVoxEditor::drawOverlayChain (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xe6070204)); g.fillRect (0.0f, 0.0f, DW, DH);
    const Rc panel (120.0f, 200.0f, 1296.0f, 460.0f);
    g.setColour (kPanel); g.fillRoundedRectangle (panel, 12.0f);
    g.setColour (juce::Colour (0xffb01820)); g.drawRoundedRectangle (panel, 12.0f, 2.0f);

    int order[M_COUNT]; getOrder (order);
    const bool custom = proc.getChain() != kStandardChain;

    g.setColour (juce::Colour (0xfff0e6e6)); g.setFont (fnt (26.0f, true));
    g.drawText ("CADENA DE EFECTOS  -  " + juce::String (custom ? "MODO PERSONALIZADO" : "ORDEN ESTANDAR"),
                Rc (150.0f, 215.0f, 1000.0f, 34.0f), juce::Justification::left, false);
    g.setColour (juce::Colour (0xffa89a9a)); g.setFont (fnt (14.0f));
    g.drawText ("Arrastra un bloque a otra posicion para cambiar el orden. Pulsa ON/OFF para activar o desactivar un modulo.",
                Rc (150.0f, 252.0f, 1200.0f, 20.0f), juce::Justification::left, false);
    g.setColour (kRed); g.setFont (fnt (16.0f, true));
    g.drawText ("ENTRADA  >>", Rc (178.0f, 298.0f, 300.0f, 22.0f), juce::Justification::left, false);
    g.drawText (">>  GANANCIA / SALIDA", Rc (1000.0f, 298.0f, 358.0f, 22.0f), juce::Justification::right, false);

    auto drawTile = [&] (Rc t, int slot, int mod, bool lifted)
    {
        const bool on = proc.apvts.getParameter (kEnableIds[mod])->getValue() > 0.5f;
        g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff3a0e12) : juce::Colour (0xff1c1c1e), t.getX(), t.getY(),
                                                 juce::Colour (0xff0c0305), t.getX(), t.getBottom(), false));
        g.fillRoundedRectangle (t, 8.0f);
        g.setColour (lifted ? juce::Colours::white : juce::Colour (on ? 0xffc01a24 : 0xff555555));
        g.drawRoundedRectangle (t, 8.0f, lifted ? 2.5f : 1.6f);
        g.setColour (juce::Colour (0xffff8a7a)); g.setFont (fnt (28.0f, true));
        g.drawText (juce::String (slot + 1), Rc (t.getX(), t.getY() + 8.0f, t.getWidth(), 32.0f), juce::Justification::centred, false);
        g.setColour (juce::Colour (on ? 0xfff0e6e6 : 0xff8a8a8a)); g.setFont (fnt (16.0f, true));
        g.drawText (kModuleNames[mod], Rc (t.getX() + 4.0f, t.getY() + 50.0f, t.getWidth() - 8.0f, 40.0f), juce::Justification::centred, true);
        drawButton (g, Rc (t.getX() + 15.0f, t.getBottom() - 38.0f, 100.0f, 26.0f), on ? "ON" : "OFF", on);
    };

    int target = -1;
    if (drag == Drag::chainTile) target = juce::jlimit (0, M_COUNT - 1, (int) ((dragX - 178.0f + 10.0f) / 150.0f));
    for (int s = 0; s < M_COUNT; ++s)
    {
        if (drag == Drag::chainTile && s == dragSlot) continue;
        drawTile (tileRect (s), s, order[s], false);
        if (s < M_COUNT - 1) { g.setColour (juce::Colour (0xff7a1a20)); g.setFont (fnt (20.0f, true));
                               g.drawText (">", Rc (tileRect (s).getRight(), 390.0f, 20.0f, 24.0f), juce::Justification::centred, false); }
    }
    if (drag == Drag::chainTile && dragSlot >= 0)
    {
        g.setColour (kRed); g.fillRect (Rc (178.0f + (float) target * 150.0f - 12.0f, 322.0f, 4.0f, 166.0f));
        auto t = tileRect (dragSlot).withX (dragX - 65.0f).translated (0.0f, -10.0f);
        drawTile (t, dragSlot, order[dragSlot], true);
    }
    for (const auto& b : chainButtons) drawButton (g, b.r, b.text);
}
