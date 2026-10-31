#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr int kSubBlock = 64;

    struct Over { const char* id; float v; };
    struct Factory { const char* name; std::vector<Over> over; };

    // Cada preset parte de los valores por defecto y cambia solo lo indicado.
    const std::vector<Factory>& factories()
    {
        static const std::vector<Factory> f =
        {
            { "DEFAULT - CUSTOM MODE", {} },

            { "NASCAR ALOE VIBE", {
                {"c_thr",-22},{"c_rat",6},{"c_att",3},{"c_rel",80},{"c_mku",6},
                {"e_lo",2},{"e_mid",3},{"e_hi",4},{"e_air",3},
                {"b_br",.5f},{"b_air",.4f},
                {"d_drv",.65f},{"d_sat",.6f},{"d_mix",.7f},
                {"ch_dep",.2f},{"ch_rate",1.2f},{"ch_mix",.15f},
                {"r_size",.3f},{"r_dec",.3f},{"r_mix",.12f},
                {"dl_time",180},{"dl_fb",.25f},{"dl_mix",.15f},
                {"w_wid",1.2f},{"w_amt",.2f},{"w_mono",.3f},{"m_gain",-1} } },

            { "SCARLXRD VIBE", {
                {"c_thr",-28},{"c_rat",10},{"c_att",1},{"c_rel",60},{"c_mku",9},
                {"e_lo",4},{"e_mid",2},{"e_hi",-1},{"e_air",0},{"e_lof",100},{"e_midf",1500},
                {"b_br",.15f},{"b_air",.1f},
                {"d_drv",.9f},{"d_sat",.85f},{"d_mix",.9f},
                {"ch_dep",.35f},{"ch_rate",.6f},{"ch_mix",.2f},
                {"r_size",.5f},{"r_dec",.45f},{"r_mix",.18f},
                {"dl_time",120},{"dl_fb",.3f},{"dl_mix",.1f},
                {"w_wid",1.1f},{"w_amt",.3f},{"w_mono",.6f},{"m_gain",-3} } },

            { "XXXTENTACION VIBE", {
                {"c_thr",-20},{"c_rat",4},{"c_att",15},{"c_rel",200},{"c_mku",4},
                {"e_lo",1},{"e_mid",1},{"e_hi",-3},{"e_air",-2},{"e_hif",4000},
                {"b_br",.1f},{"b_air",.1f},
                {"d_drv",.4f},{"d_sat",.7f},{"d_mix",.5f},
                {"ch_dep",.55f},{"ch_rate",.4f},{"ch_mix",.35f},
                {"r_size",.6f},{"r_dec",.6f},{"r_mix",.3f},
                {"dl_time",300},{"dl_fb",.4f},{"dl_mix",.25f},
                {"w_wid",1.3f},{"w_amt",.35f},{"w_mono",.4f},{"m_gain",-1} } },

            { "GHOSTEMANE VIBE", {
                {"c_thr",-24},{"c_rat",5},{"c_att",5},{"c_rel",150},{"c_mku",5},
                {"e_lo",3},{"e_mid",-2},{"e_hi",2},{"e_air",2},
                {"b_br",.3f},{"b_air",.5f},
                {"d_drv",.55f},{"d_sat",.5f},{"d_mix",.6f},
                {"ch_dep",.45f},{"ch_rate",.3f},{"ch_mix",.3f},
                {"r_size",.85f},{"r_dec",.8f},{"r_mix",.4f},
                {"dl_time",450},{"dl_fb",.5f},{"dl_mix",.3f},
                {"w_wid",1.5f},{"w_amt",.5f},{"w_mono",.5f},{"m_gain",-2} } },

            { "HARDTRAP DEMON", {
                {"c_thr",-26},{"c_rat",8},{"c_att",2},{"c_rel",70},{"c_mku",8},
                {"e_lo",5},{"e_mid",1},{"e_hi",2},{"e_air",1},
                {"b_br",.25f},{"b_air",.2f},
                {"d_drv",.85f},{"d_sat",.8f},{"d_mix",.85f},
                {"ch_dep",.3f},{"ch_rate",.5f},{"ch_mix",.2f},
                {"r_size",.55f},{"r_dec",.55f},{"r_mix",.25f},
                {"dl_time",250},{"dl_fb",.4f},{"dl_mix",.2f},
                {"w_wid",1.25f},{"w_amt",.3f},{"w_mono",.5f},{"m_gain",-3} } },

            { "HARDTRAP CLEAN", {
                {"c_thr",-18},{"c_rat",3},{"c_att",10},{"c_rel",120},{"c_mku",3},
                {"e_lo",-1},{"e_mid",1},{"e_hi",3},{"e_air",4},
                {"b_br",.5f},{"b_air",.6f},
                {"d_drv",.1f},{"d_sat",.1f},{"d_mix",.2f},
                {"ch_dep",.15f},{"ch_rate",.8f},{"ch_mix",.1f},
                {"r_size",.35f},{"r_dec",.35f},{"r_mix",.15f},
                {"dl_time",220},{"dl_fb",.2f},{"dl_mix",.1f},
                {"w_wid",1.15f},{"w_amt",.15f},{"w_mono",.2f},{"m_gain",0} } },

            { "HARDTRAP SCREAM", {
                {"c_thr",-30},{"c_rat",12},{"c_att",.5f},{"c_rel",50},{"c_mku",10},
                {"e_lo",-3},{"e_mid",5},{"e_hi",5},{"e_air",2},{"e_midf",2500},
                {"b_br",.6f},{"b_air",.3f},
                {"d_drv",1},{"d_sat",1},{"d_mix",1},
                {"ch_dep",0},{"ch_mix",0},
                {"r_size",.4f},{"r_dec",.3f},{"r_mix",.12f},
                {"dl_time",100},{"dl_fb",.2f},{"dl_mix",.08f},
                {"w_wid",1},{"w_amt",0},{"w_mono",.6f},{"m_gain",-4} } },

            { "HARDTRAP DARK", {
                {"c_thr",-22},{"c_rat",5},{"c_att",8},{"c_rel",160},{"c_mku",4},
                {"e_lo",3},{"e_mid",-3},{"e_hi",-4},{"e_air",-3},{"e_hif",3500},
                {"b_br",0},{"b_air",0},
                {"d_drv",.6f},{"d_sat",.6f},{"d_mix",.7f},
                {"ch_dep",.5f},{"ch_rate",.3f},{"ch_mix",.3f},
                {"r_size",.8f},{"r_dec",.85f},{"r_mix",.4f},
                {"dl_time",500},{"dl_fb",.55f},{"dl_mix",.3f},
                {"w_wid",1.4f},{"w_amt",.4f},{"w_mono",.5f},{"m_gain",-2} } },
        };
        return f;
    }

    int indexOfParam (const char* id)
    {
        for (int i = 0; i < P_COUNT; ++i)
            if (juce::String (kParams[i].id) == id) return i;
        return -1;
    }
}

// ---------------------------------------------------------------------------
juce::AudioProcessorValueTreeState::ParameterLayout HardVoxProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < P_COUNT; ++i)
    {
        const auto& d = kParams[i];
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { d.id, 1 }, d.name,
            juce::NormalisableRange<float> (d.mn, d.mx, 0.0f, d.skew), d.def));
    }
    for (int m = 0; m < M_COUNT; ++m)
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { kEnableIds[m], 1 }, juce::String (kModuleNames[m]) + " On", true));

    return layout;
}

HardVoxProcessor::HardVoxProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HARDVOX", createLayout())
{
    for (int i = 0; i < P_COUNT; ++i) raw[i] = apvts.getRawParameterValue (kParams[i].id);
    for (int m = 0; m < M_COUNT; ++m) rawEn[m] = apvts.getRawParameterValue (kEnableIds[m]);
    apvts.state.setProperty ("preset", "DEFAULT - CUSTOM MODE", nullptr);
    apvts.state.setProperty ("chain", (int) kStandardChain, nullptr);
}

bool HardVoxProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void HardVoxProcessor::prepareToPlay (double sr, int)
{
    currentSampleRate = sr;
    comp.prepare (sr); eq.prepare (sr); bright.prepare (sr); chorus.prepare (sr);
    reverb.prepare (sr); delay.prepare (sr); widener.prepare (sr);
    scratchL.assign (kSubBlock, 0.0f);
    scratchR.assign (kSubBlock, 0.0f);

    for (int i = 0; i < P_COUNT; ++i)
    {
        sm[i].reset (sr, 0.03);
        sm[i].setCurrentAndTargetValue (raw[i]->load());
    }
    for (int m = 0; m < M_COUNT; ++m)
    {
        enSm[m].reset (sr, 0.02);
        prevE[m] = rawEn[m]->load() > 0.5f ? 1.0f : 0.0f;
        enSm[m].setCurrentAndTargetValue (prevE[m]);
    }
    prevMaster = juce::Decibels::decibelsToGain (raw[P_M_GAIN]->load());
    appliedChain = chainPacked.load();
    chainFadeState = 0;
}

void HardVoxProcessor::runModule (int id, float* l, float* r, int n, const float* v)
{
    switch (id)
    {
        case M_COMP:    comp.process (l, r, n, v[P_C_THR], v[P_C_RAT], v[P_C_ATT], v[P_C_REL], v[P_C_MKU]); break;
        case M_EQ:      eq.process (l, r, n, v[P_E_LO], v[P_E_MID], v[P_E_HI], v[P_E_AIR], v[P_E_LOF], v[P_E_MIDF], v[P_E_HIF]); break;
        case M_BRIGHT:  bright.process (l, r, n, v[P_B_BR], v[P_B_AIR]); break;
        case M_DIST:    dist.process (l, r, n, v[P_D_DRV], v[P_D_SAT], v[P_D_MIX]); break;
        case M_CHORUS:  chorus.process (l, r, n, v[P_CH_DEP], v[P_CH_RATE], v[P_CH_MIX]); break;
        case M_REVERB:  reverb.process (l, r, n, v[P_R_SIZE], v[P_R_DEC], v[P_R_MIX]); break;
        case M_DELAY:   delay.process (l, r, n, v[P_DL_TIME], v[P_DL_FB], v[P_DL_MIX]); break;
        case M_WIDENER: widener.process (l, r, n, v[P_W_WID], v[P_W_AMT], v[P_W_MONO]); break;
        default: break;
    }
}

void HardVoxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int total = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2 || total == 0) return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    // medidor de entrada
    {
        float pk = 0.0f;
        for (int i = 0; i < total; ++i) pk = std::max (pk, std::max (std::abs (L[i]), std::abs (R[i])));
        inLevel.store (std::max (inLevel.load(), pk));
    }

    // orden de la cadena (con cambio suave: fundido de salida -> cambio -> fundido de entrada)
    const uint32_t wanted = chainPacked.load();
    if (chainFadeState == 0 && wanted != appliedChain) chainFadeState = 1;
    int order[M_COUNT];
    if (! unpackChain (appliedChain, order)) unpackChain (kStandardChain, order);

    float v[P_COUNT];

    for (int pos = 0; pos < total; pos += kSubBlock)
    {
        const int n = std::min (kSubBlock, total - pos);
        float* l = L + pos;
        float* r = R + pos;

        for (int i = 0; i < P_COUNT; ++i)
        {
            sm[i].setTargetValue (raw[i]->load());
            v[i] = sm[i].skip (n);
        }

        for (int s = 0; s < M_COUNT; ++s)
        {
            const int id = order[s];
            enSm[id].setTargetValue (rawEn[id]->load() > 0.5f ? 1.0f : 0.0f);
            const float e = enSm[id].skip (n);
            const float pe = prevE[id];
            prevE[id] = e;

            if (e <= 0.0001f && pe <= 0.0001f) continue;              // apagado

            if (e >= 0.9999f && pe >= 0.9999f)                        // encendido
            {
                runModule (id, l, r, n, v);
            }
            else                                                       // transicion suave on/off
            {
                std::copy (l, l + n, scratchL.begin());
                std::copy (r, r + n, scratchR.begin());
                runModule (id, l, r, n, v);
                for (int i = 0; i < n; ++i)
                {
                    const float m = pe + (e - pe) * (float) (i + 1) / (float) n;
                    l[i] = scratchL[(size_t) i] + (l[i] - scratchL[(size_t) i]) * m;
                    r[i] = scratchR[(size_t) i] + (r[i] - scratchR[(size_t) i]) * m;
                }
            }
        }

        // ganancia master con rampa
        const float g = juce::Decibels::decibelsToGain (v[P_M_GAIN]);
        for (int i = 0; i < n; ++i)
        {
            const float gi = prevMaster + (g - prevMaster) * (float) (i + 1) / (float) n;
            l[i] *= gi; r[i] *= gi;
        }
        prevMaster = g;
    }

    // rampas de fundido al cambiar el orden de la cadena
    if (chainFadeState == 1)
    {
        for (int i = 0; i < total; ++i)
        {
            const float g = 1.0f - (float) (i + 1) / (float) total;
            L[i] *= g; R[i] *= g;
        }
        appliedChain = wanted;
        chainFadeState = 2;
    }
    else if (chainFadeState == 2)
    {
        for (int i = 0; i < total; ++i)
        {
            const float g = (float) (i + 1) / (float) total;
            L[i] *= g; R[i] *= g;
        }
        chainFadeState = 0;
    }

    // medidor de salida
    float pk = 0.0f;
    for (int i = 0; i < total; ++i) pk = std::max (pk, std::max (std::abs (L[i]), std::abs (R[i])));
    outLevel.store (std::max (outLevel.load(), pk));
}

juce::AudioProcessorEditor* HardVoxProcessor::createEditor() { return new HardVoxEditor (*this); }

// ------------------------------ estado --------------------------------------
void HardVoxProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    apvts.state.setProperty ("chain", (int) chainPacked.load(), nullptr);
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void HardVoxProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            readChainFromState();
        }
}

void HardVoxProcessor::readChainFromState()
{
    int order[M_COUNT];
    const uint32_t p = (uint32_t) (int) apvts.state.getProperty ("chain", (int) kStandardChain);
    chainPacked.store (unpackChain (p, order) ? p : kStandardChain);
}

void HardVoxProcessor::setChain (uint32_t packed)
{
    int order[M_COUNT];
    if (! unpackChain (packed, order)) packed = kStandardChain;
    chainPacked.store (packed);
    apvts.state.setProperty ("chain", (int) packed, nullptr);
}

// ------------------------------ presets -------------------------------------
int HardVoxProcessor::getNumFactoryPresets() { return (int) factories().size(); }
juce::String HardVoxProcessor::getFactoryName (int i) { return factories()[(size_t) juce::jlimit (0, getNumFactoryPresets() - 1, i)].name; }

void HardVoxProcessor::setParamValue (int index, float value)
{
    if (auto* p = apvts.getParameter (kParams[index].id))
        p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (kParams[index].mn, kParams[index].mx, value)));
}

void HardVoxProcessor::setEnable (int module, bool on)
{
    if (auto* p = apvts.getParameter (kEnableIds[module]))
        p->setValueNotifyingHost (on ? 1.0f : 0.0f);
}

void HardVoxProcessor::applyFactoryPreset (int index)
{
    index = juce::jlimit (0, getNumFactoryPresets() - 1, index);
    const auto& f = factories()[(size_t) index];

    for (int i = 0; i < P_COUNT; ++i) setParamValue (i, kParams[i].def);
    for (const auto& o : f.over)
    {
        const int idx = indexOfParam (o.id);
        if (idx >= 0) setParamValue (idx, o.v);
    }
    for (int m = 0; m < M_COUNT; ++m) setEnable (m, true);
    setChain (kStandardChain);
    apvts.state.setProperty ("preset", juce::String (f.name), nullptr);
    ++presetChangeCounter;
}

void HardVoxProcessor::resetToDefault() { applyFactoryPreset (0); }

juce::String HardVoxProcessor::getCurrentPresetName() const
{
    return apvts.state.getProperty ("preset", "DEFAULT - CUSTOM MODE").toString();
}

juce::File HardVoxProcessor::getUserPresetDir() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("HardVox").getChildFile ("Presets");
}

juce::File HardVoxProcessor::presetFile (const juce::String& name) const
{
    return getUserPresetDir().getChildFile (juce::File::createLegalFileName (name.trim()) + ".hvpreset");
}

juce::StringArray HardVoxProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getUserPresetDir().findChildFiles (juce::File::findFiles, false, "*.hvpreset"))
        names.add (f.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool HardVoxProcessor::saveUserPreset (const juce::String& name)
{
    if (name.trim().isEmpty()) return false;
    const auto dir = getUserPresetDir();
    if (! dir.createDirectory().wasOk()) return false;

    apvts.state.setProperty ("chain", (int) chainPacked.load(), nullptr);
    apvts.state.setProperty ("preset", name.trim(), nullptr);
    if (auto xml = apvts.copyState().createXml())
        return presetFile (name).replaceWithText (xml->toString());
    return false;
}

bool HardVoxProcessor::loadUserPreset (const juce::String& name)
{
    const auto file = presetFile (name);
    if (! file.existsAsFile()) return false;
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;

    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    readChainFromState();
    apvts.state.setProperty ("preset", name.trim(), nullptr);
    ++presetChangeCounter;
    return true;
}

bool HardVoxProcessor::deleteUserPreset (const juce::String& name)
{
    return presetFile (name).deleteFile();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HardVoxProcessor(); }
