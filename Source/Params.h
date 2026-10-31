#pragma once
#include "Common.h"

// ---------------------------------------------------------------------------
//  Definicion de todos los parametros (automatizables desde FL Studio)
// ---------------------------------------------------------------------------
enum ModuleId { M_COMP = 0, M_EQ, M_BRIGHT, M_DIST, M_CHORUS, M_REVERB, M_DELAY, M_WIDENER, M_COUNT };

enum ParamIndex
{
    P_C_THR, P_C_RAT, P_C_ATT, P_C_REL, P_C_MKU,
    P_E_LO, P_E_MID, P_E_HI, P_E_AIR, P_E_LOF, P_E_MIDF, P_E_HIF,
    P_B_BR, P_B_AIR,
    P_D_DRV, P_D_SAT, P_D_MIX,
    P_CH_DEP, P_CH_RATE, P_CH_MIX,
    P_R_SIZE, P_R_DEC, P_R_MIX,
    P_DL_TIME, P_DL_FB, P_DL_MIX,
    P_W_WID, P_W_AMT, P_W_MONO,
    P_M_GAIN,
    P_COUNT
};

struct ParamDef
{
    const char* id;
    const char* name;
    float mn, mx, def, skew;
    const char* unit;
    int decimals;
    int module; // -1 = master
};

inline constexpr ParamDef kParams[P_COUNT] =
{
    { "c_thr",  "Threshold",   -60.f,  0.f,   -12.f, 1.0f, " dB", 1, M_COMP },
    { "c_rat",  "Ratio",         1.f, 20.f,     2.f, 0.5f, ":1",  1, M_COMP },
    { "c_att",  "Attack",       0.1f,100.f,    10.f, 0.4f, " ms", 1, M_COMP },
    { "c_rel",  "Release",      10.f,1000.f,  120.f, 0.4f, " ms", 0, M_COMP },
    { "c_mku",  "Makeup",        0.f, 24.f,     0.f, 1.0f, " dB", 1, M_COMP },

    { "e_lo",   "Low",         -15.f, 15.f,     0.f, 1.0f, " dB", 1, M_EQ },
    { "e_mid",  "Mid",         -15.f, 15.f,     0.f, 1.0f, " dB", 1, M_EQ },
    { "e_hi",   "High",        -15.f, 15.f,     0.f, 1.0f, " dB", 1, M_EQ },
    { "e_air",  "Air",         -15.f, 15.f,     0.f, 1.0f, " dB", 1, M_EQ },
    { "e_lof",  "Low Freq",     40.f,400.f,   120.f, 0.5f, " Hz", 0, M_EQ },
    { "e_midf", "Mid Freq",    300.f,5000.f, 1200.f, 0.4f, " Hz", 0, M_EQ },
    { "e_hif",  "High Freq",  2000.f,12000.f,5000.f, 0.5f, " Hz", 0, M_EQ },

    { "b_br",   "Brightness",    0.f,  1.f,     0.f, 1.0f, "",    2, M_BRIGHT },
    { "b_air",  "Air Intensity", 0.f,  1.f,     0.f, 1.0f, "",    2, M_BRIGHT },

    { "d_drv",  "Drive",         0.f,  1.f,     0.f, 1.0f, "",    2, M_DIST },
    { "d_sat",  "Saturation",    0.f,  1.f,     0.f, 1.0f, "",    2, M_DIST },
    { "d_mix",  "Dist Mix",      0.f,  1.f,     1.f, 1.0f, "",    2, M_DIST },

    { "ch_dep", "Depth",         0.f,  1.f,     0.4f,1.0f, "",    2, M_CHORUS },
    { "ch_rate","Rate",         0.05f, 5.f,     0.8f,0.5f, " Hz", 2, M_CHORUS },
    { "ch_mix", "Chorus Mix",    0.f,  1.f,     0.f, 1.0f, "",    2, M_CHORUS },

    { "r_size", "Size",          0.f,  1.f,     0.5f,1.0f, "",    2, M_REVERB },
    { "r_dec",  "Decay",         0.f,  1.f,     0.5f,1.0f, "",    2, M_REVERB },
    { "r_mix",  "Reverb Mix",    0.f,  1.f,     0.f, 1.0f, "",    2, M_REVERB },

    { "dl_time","Time",         10.f,1500.f,  350.f, 0.5f, " ms", 0, M_DELAY },
    { "dl_fb",  "Feedback",      0.f, 0.95f,    0.35f,1.0f,"",    2, M_DELAY },
    { "dl_mix", "Delay Mix",     0.f,  1.f,     0.f, 1.0f, "",    2, M_DELAY },

    { "w_wid",  "Width",         0.f,  2.f,     1.f, 1.0f, "",    2, M_WIDENER },
    { "w_amt",  "Stereo Amount", 0.f,  1.f,     0.f, 1.0f, "",    2, M_WIDENER },
    { "w_mono", "Mono Compat",   0.f,  1.f,     0.f, 1.0f, "",    2, M_WIDENER },

    { "m_gain", "Master Gain", -24.f, 12.f,     0.f, 1.0f, " dB", 1, -1 },
};

inline constexpr const char* kEnableIds[M_COUNT] =
    { "en0", "en1", "en2", "en3", "en4", "en5", "en6", "en7" };

inline constexpr const char* kModuleNames[M_COUNT] =
    { "COMPRESOR", "EQ", "BRILLO / AIRE", "DISTORSION", "CHORUS", "REVERB", "DELAY", "STEREO WIDENER" };

// ---------------------------------------------------------------------------
//  Orden de la cadena de efectos: 8 modulos x 3 bits empaquetados en un uint32
//  (se lee/escribe de forma atomica, sin bloqueos en el hilo de audio)
// ---------------------------------------------------------------------------
inline constexpr uint32_t kStandardChain =
    (0u) | (1u << 3) | (2u << 6) | (3u << 9) | (4u << 12) | (5u << 15) | (6u << 18) | (7u << 21);

inline uint32_t packChain (const int* order)
{
    uint32_t p = 0;
    for (int i = 0; i < M_COUNT; ++i)
        p |= (uint32_t) (order[i] & 7) << (3 * i);
    return p;
}

inline bool unpackChain (uint32_t p, int* order)
{
    bool seen[M_COUNT] = {};
    for (int i = 0; i < M_COUNT; ++i)
    {
        order[i] = (int) ((p >> (3 * i)) & 7u);
        if (seen[order[i]]) return false;
        seen[order[i]] = true;
    }
    return true;
}
