#pragma once
#include "Common.h"

// ---------------------------------------------------------------------------
//  Modulos DSP. Ninguno reserva memoria dentro de process(): todo se prepara
//  en prepare() (fuera del hilo de audio).
// ---------------------------------------------------------------------------

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }

    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void copyCoeffs (const Biquad& o) { b0 = o.b0; b1 = o.b1; b2 = o.b2; a1 = o.a1; a2 = o.a2; }

    void set (double nb0, double nb1, double nb2, double a0, double na1, double na2)
    {
        b0 = (float) (nb0 / a0); b1 = (float) (nb1 / a0); b2 = (float) (nb2 / a0);
        a1 = (float) (na1 / a0); a2 = (float) (na2 / a0);
    }

    static double clampF (double sr, double f) { return std::min (std::max (f, 10.0), sr * 0.45); }

    void makePeak (double sr, double f, double q, double gDb)
    {
        f = clampF (sr, f);
        const double A = std::pow (10.0, gDb / 40.0), w0 = 6.283185307179586 * f / sr;
        const double al = std::sin (w0) / (2.0 * q), c = std::cos (w0);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }

    void makeLowShelf (double sr, double f, double gDb)
    {
        f = clampF (sr, f);
        const double A = std::pow (10.0, gDb / 40.0), w0 = 6.283185307179586 * f / sr;
        const double c = std::cos (w0), s = std::sin (w0);
        const double alpha = s * 0.5 * std::sqrt (2.0);
        const double t = 2.0 * std::sqrt (A) * alpha;
        set (A * ((A + 1) - (A - 1) * c + t), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - t),
             (A + 1) + (A - 1) * c + t, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t);
    }

    void makeHighShelf (double sr, double f, double gDb)
    {
        f = clampF (sr, f);
        const double A = std::pow (10.0, gDb / 40.0), w0 = 6.283185307179586 * f / sr;
        const double c = std::cos (w0), s = std::sin (w0);
        const double alpha = s * 0.5 * std::sqrt (2.0);
        const double t = 2.0 * std::sqrt (A) * alpha;
        set (A * ((A + 1) + (A - 1) * c + t), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - t),
             (A + 1) - (A - 1) * c + t, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t);
    }

    void makeLowpass (double sr, double f, double q)
    {
        f = clampF (sr, f);
        const double w0 = 6.283185307179586 * f / sr, c = std::cos (w0), al = std::sin (w0) / (2 * q);
        set ((1 - c) * 0.5, 1 - c, (1 - c) * 0.5, 1 + al, -2 * c, 1 - al);
    }

    void makeHighpass (double sr, double f, double q)
    {
        f = clampF (sr, f);
        const double w0 = 6.283185307179586 * f / sr, c = std::cos (w0), al = std::sin (w0) / (2 * q);
        set ((1 + c) * 0.5, -(1 + c), (1 + c) * 0.5, 1 + al, -2 * c, 1 - al);
    }

    // Respuesta en frecuencia (para dibujar la curva del EQ)
    double magnitudeDb (double f, double sr) const
    {
        const double w = 6.283185307179586 * f / sr;
        const std::complex<double> e1 (std::cos (w), -std::sin (w)), e2 = e1 * e1;
        const std::complex<double> num = (double) b0 + (double) b1 * e1 + (double) b2 * e2;
        const std::complex<double> den = 1.0 + (double) a1 * e1 + (double) a2 * e2;
        return 20.0 * std::log10 (std::max (1.0e-9, std::abs (num) / std::max (1.0e-9, std::abs (den))));
    }
};

inline float readLerp (const std::vector<float>& buf, int size, float pos)
{
    while (pos < 0.0f) pos += (float) size;
    int i0 = (int) pos;
    const float fr = pos - (float) i0;
    if (i0 >= size) i0 -= size;
    int i1 = i0 + 1; if (i1 >= size) i1 = 0;
    return buf[(size_t) i0] + fr * (buf[(size_t) i1] - buf[(size_t) i0]);
}

// ------------------------------- COMPRESOR ---------------------------------
struct Compressor
{
    double sr = 44100.0;
    float grDb = 0.0f;

    void prepare (double s) { sr = s; grDb = 0.0f; }

    void process (float* L, float* R, int n, float thr, float ratio, float attMs, float relMs, float makeupDb)
    {
        const float attC = std::exp (-1.0f / (0.001f * attMs * (float) sr));
        const float relC = std::exp (-1.0f / (0.001f * relMs * (float) sr));
        const float slope = 1.0f - 1.0f / std::max (1.0f, ratio);
        const float makeup = juce::Decibels::decibelsToGain (makeupDb);

        for (int i = 0; i < n; ++i)
        {
            const float a = std::max (std::abs (L[i]), std::abs (R[i]));
            const float lvDb = juce::Decibels::gainToDecibels (a, -100.0f);
            const float over = lvDb - thr;
            const float target = over > 0.0f ? over * slope : 0.0f;
            grDb = target > grDb ? attC * grDb + (1.0f - attC) * target
                                 : relC * grDb + (1.0f - relC) * target;
            const float g = juce::Decibels::decibelsToGain (-grDb) * makeup;
            L[i] *= g; R[i] *= g;
        }
    }
};

// ---------------------------------- EQ --------------------------------------
struct EQ
{
    double sr = 44100.0;
    Biquad lo[2], mid[2], hi[2], air[2];

    void prepare (double s) { sr = s; reset(); }
    void reset() { for (int c = 0; c < 2; ++c) { lo[c].reset(); mid[c].reset(); hi[c].reset(); air[c].reset(); } }

    void updateCoeffs (Biquad& l, Biquad& m, Biquad& h, Biquad& a, float gLo, float gMid, float gHi, float gAir,
                       float fLo, float fMid, float fHi) const
    {
        l.makeLowShelf (sr, fLo, gLo);
        m.makePeak (sr, fMid, 0.9, gMid);
        h.makeHighShelf (sr, fHi, gHi);
        a.makeHighShelf (sr, 12000.0, gAir);
    }

    void process (float* L, float* R, int n, float gLo, float gMid, float gHi, float gAir,
                  float fLo, float fMid, float fHi)
    {
        updateCoeffs (lo[0], mid[0], hi[0], air[0], gLo, gMid, gHi, gAir, fLo, fMid, fHi);
        lo[1].copyCoeffs (lo[0]); mid[1].copyCoeffs (mid[0]); hi[1].copyCoeffs (hi[0]); air[1].copyCoeffs (air[0]);

        for (int i = 0; i < n; ++i)
        {
            L[i] = air[0].process (hi[0].process (mid[0].process (lo[0].process (L[i]))));
            R[i] = air[1].process (hi[1].process (mid[1].process (lo[1].process (R[i]))));
        }
    }
};

// ------------------------------ BRILLO / AIRE -------------------------------
struct Bright
{
    double sr = 44100.0;
    Biquad shelf[2], airS[2], hp[2];

    void prepare (double s) { sr = s; for (int c = 0; c < 2; ++c) { shelf[c].reset(); airS[c].reset(); hp[c].reset(); } }

    void process (float* L, float* R, int n, float br, float air)
    {
        shelf[0].makeHighShelf (sr, 3500.0, 6.0 * br);
        airS[0].makeHighShelf (sr, 11000.0, 9.0 * air);
        hp[0].makeHighpass (sr, 4500.0, 0.707);
        shelf[1].copyCoeffs (shelf[0]); airS[1].copyCoeffs (airS[0]); hp[1].copyCoeffs (hp[0]);
        const float ex = 0.16f * br + 0.06f * air; // exciter armonico sutil

        for (int i = 0; i < n; ++i)
        {
            const float xl = L[i], xr = R[i];
            const float el = std::tanh (hp[0].process (xl) * 4.0f) * ex;
            const float er = std::tanh (hp[1].process (xr) * 4.0f) * ex;
            L[i] = airS[0].process (shelf[0].process (xl)) + el;
            R[i] = airS[1].process (shelf[1].process (xr)) + er;
        }
    }
};

// -------------------------- DISTORSION / SATURACION -------------------------
struct Distortion
{
    void process (float* L, float* R, int n, float drive, float sat, float mix)
    {
        if (drive <= 0.0001f && sat <= 0.0001f) return;
        const float gain = std::pow (10.0f, drive * 30.0f / 20.0f);
        const float amt = juce::jlimit (0.0f, 1.0f, drive * 6.0f + sat * 2.0f);
        const float bias = sat * 0.25f;
        const float hard = sat * 0.6f;
        const float comp = 0.9f / (1.0f + drive * 1.5f);
        const float tb = std::tanh (bias);

        for (int i = 0; i < n; ++i)
        {
            for (int c = 0; c < 2; ++c)
            {
                float* p = (c == 0 ? L : R) + i;
                const float x = *p;
                const float t = std::tanh (x * gain + bias) - tb;
                const float cl = juce::jlimit (-1.0f, 1.0f, x * gain);
                const float shaped = (t * (1.0f - hard) + cl * hard) * comp;
                const float y = x + amt * (shaped - x);
                *p = x + mix * (y - x);
            }
        }
    }
};

// --------------------------------- CHORUS -----------------------------------
struct Chorus
{
    double sr = 44100.0;
    std::vector<float> bufL, bufR;
    int size = 0, w = 0;
    float phase = 0.0f;

    void prepare (double s)
    {
        sr = s;
        size = (int) (sr * 0.05) + 8;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        w = 0; phase = 0.0f;
    }

    void process (float* L, float* R, int n, float depth, float rate, float mix)
    {
        const float inc = rate / (float) sr;
        const float base = 0.012f * (float) sr;
        const float mod = (0.002f + 0.006f * depth) * (float) sr;
        const float norm = 1.0f / (1.0f + mix * 0.5f);

        for (int i = 0; i < n; ++i)
        {
            bufL[(size_t) w] = L[i];
            bufR[(size_t) w] = R[i];
            float pR = phase + 0.25f; if (pR >= 1.0f) pR -= 1.0f;
            const float dL = base + mod * std::sin (6.2831853f * phase);
            const float dR = base + mod * std::sin (6.2831853f * pR);
            const float wl = readLerp (bufL, size, (float) w - dL);
            const float wr = readLerp (bufR, size, (float) w - dR);
            L[i] = (L[i] + wl * mix) * norm;
            R[i] = (R[i] + wr * mix) * norm;
            phase += inc; if (phase >= 1.0f) phase -= 1.0f;
            if (++w >= size) w = 0;
        }
    }
};

// --------------------------------- REVERB -----------------------------------
struct ReverbFx
{
    juce::Reverb rev;

    void prepare (double s) { rev.setSampleRate (s); rev.reset(); }

    void process (float* L, float* R, int n, float size, float decay, float mix)
    {
        juce::Reverb::Parameters p;
        p.roomSize = juce::jlimit (0.0f, 1.0f, size * 0.5f + decay * 0.5f);
        p.damping  = juce::jlimit (0.0f, 1.0f, 0.65f - decay * 0.35f);
        p.wetLevel = mix * 0.35f;
        p.dryLevel = 1.0f - mix * 0.3f;
        p.width    = 1.0f;
        p.freezeMode = 0.0f;
        rev.setParameters (p);
        rev.processStereo (L, R, n);
    }
};

// ---------------------------------- DELAY -----------------------------------
struct DelayFx
{
    double sr = 44100.0;
    std::vector<float> bufL, bufR;
    int size = 0, w = 0;
    float lpL = 0.0f, lpR = 0.0f;
    juce::SmoothedValue<float> timeSm;

    void prepare (double s)
    {
        sr = s;
        size = (int) (sr * 1.7) + 8;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        w = 0; lpL = lpR = 0.0f;
        timeSm.reset (sr, 0.08);
        timeSm.setCurrentAndTargetValue ((float) (0.35 * sr));
    }

    void process (float* L, float* R, int n, float timeMs, float fb, float mix)
    {
        timeSm.setTargetValue ((float) (timeMs * 0.001 * sr));
        const float a = 1.0f - std::exp (-6.2831853f * 5000.0f / (float) sr);
        const float maxT = (float) (size - 4);
        fb = std::min (fb, 0.95f);

        for (int i = 0; i < n; ++i)
        {
            const float t = timeSm.getNextValue();
            const float tL = std::min (t, maxT), tR = std::min (t * 1.02f, maxT);
            const float dl = readLerp (bufL, size, (float) w - tL);
            const float dr = readLerp (bufR, size, (float) w - tR);
            lpL += a * (dl - lpL);
            lpR += a * (dr - lpR);
            const float inL = L[i], inR = R[i];
            bufL[(size_t) w] = inL + lpL * fb;
            bufR[(size_t) w] = inR + lpR * fb;
            L[i] = inL + dl * mix;
            R[i] = inR + dr * mix;
            if (++w >= size) w = 0;
        }
    }
};

// ------------------------------ STEREO WIDENER ------------------------------
struct Widener
{
    double sr = 44100.0;
    std::vector<float> dly;
    int size = 0, w = 0, dSamples = 0;
    Biquad lowS;

    void prepare (double s)
    {
        sr = s;
        size = (int) (sr * 0.03) + 8;
        dly.assign ((size_t) size, 0.0f);
        w = 0;
        dSamples = (int) (0.011 * sr);
        lowS.reset();
        lowS.makeLowpass (sr, 150.0, 0.707);
    }

    // width: 0..2 (1 = sin cambio) | amount: decorrelacion mono-segura | mono: bajos en mono
    void process (float* L, float* R, int n, float width, float amount, float mono)
    {
        for (int i = 0; i < n; ++i)
        {
            const float M = 0.5f * (L[i] + R[i]);
            float S = 0.5f * (L[i] - R[i]);
            dly[(size_t) w] = M;
            int rp = w - dSamples; if (rp < 0) rp += size;
            S += amount * 0.5f * dly[(size_t) rp];   // M retrasado en S: se cancela al sumar a mono
            S *= width;
            const float low = lowS.process (S);
            S -= mono * low;
            L[i] = M + S;
            R[i] = M - S;
            if (++w >= size) w = 0;
        }
    }
};
