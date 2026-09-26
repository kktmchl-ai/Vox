// usage: runcore in.f32 out.f32 sr  [param=value ...]
// params: inGain outGain eqOn hpFreq lowOn lowFreq lowGain mid1On mid1Freq mid1Gain mid1Q
//         mid2On mid2Freq mid2Gain mid2Q highOn highFreq highGain
//         compOn thr ratio knee atk rel makeup
//         deessOn deessFreq deessThr deessRange deessListen
//         satOn drive warmth mix
//         limOn ceiling limRelease
#include "../Source/ChannelCore.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <chrono>

int main (int argc, char** argv)
{
    if (argc < 4) { fprintf (stderr, "args\n"); return 1; }
    double sr = atof (argv[3]);

    std::map<std::string, std::string> p;
    for (int i = 4; i < argc; ++i)
    {
        std::string s (argv[i]);
        auto eq = s.find ('=');
        if (eq != std::string::npos) p[s.substr (0, eq)] = s.substr (eq + 1);
    }
    auto f = [&] (const char* k, float def) { auto it = p.find (k); return it == p.end() ? def : (float) atof (it->second.c_str()); };
    auto b = [&] (const char* k, bool def)  { auto it = p.find (k); return it == p.end() ? def : (atoi (it->second.c_str()) != 0); };

    FILE* fin = fopen (argv[1], "rb"); fseek (fin, 0, SEEK_END); long bytes = ftell (fin); fseek (fin, 0, SEEK_SET);
    long n = bytes / 4;
    std::vector<float> x ((size_t) n); fread (x.data(), 4, x.size(), fin); fclose (fin);

    voxchain::ChannelCore core;
    core.prepare (sr);
    core.setInputGainDb (f ("inGain", 0.f));
    core.setOutputGainDb (f ("outGain", 0.f));

    core.setEqOn (b ("eqOn", true));
    core.eqRef().setHighpass (true, f ("hpFreq", 80.f));
    core.eqRef().setLowShelf  (b ("lowOn", true),  f ("lowFreq", 120.f),  f ("lowGain", 0.f));
    core.eqRef().setMid1      (b ("mid1On", true),  f ("mid1Freq", 800.f), f ("mid1Gain", 0.f), f ("mid1Q", 1.0f));
    core.eqRef().setMid2      (b ("mid2On", true),  f ("mid2Freq", 3000.f),f ("mid2Gain", 0.f), f ("mid2Q", 1.0f));
    core.eqRef().setHighShelf (b ("highOn", true), f ("highFreq", 8000.f),f ("highGain", 0.f));

    core.setCompOn (b ("compOn", true));
    core.compRef().setThresholdDb (f ("thr", -18.f));
    core.compRef().setRatio (f ("ratio", 3.f));
    core.compRef().setKneeDb (f ("knee", 6.f));
    core.compRef().setAttackMs (f ("atk", 12.f));
    core.compRef().setReleaseMs (f ("rel", 120.f));
    core.compRef().setMakeupDb (f ("makeup", 0.f));

    core.setDeessOn (b ("deessOn", true));
    core.deessRef().setFrequency (f ("deessFreq", 6500.f));
    core.deessRef().setThresholdDb (f ("deessThr", -24.f));
    core.deessRef().setRangeDb (f ("deessRange", 12.f));
    core.deessRef().setListen (b ("deessListen", false));

    core.setSatOn (b ("satOn", true));
    core.satRef().setDrive (f ("drive", 0.3f));
    core.satRef().setWarmth (f ("warmth", 0.2f));
    core.satRef().setMix (f ("mix", 0.5f));

    core.setLimiterOn (b ("limOn", true));
    core.limRef().setCeilingDb (f ("ceiling", -0.3f));
    core.limRef().setReleaseMs (f ("limRelease", 60.f));

    std::vector<float> y ((size_t) n);
    auto t0 = std::chrono::steady_clock::now();
    for (long i = 0; i < n; ++i) y[(size_t) i] = core.process (x[(size_t) i]);
    double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    fprintf (stderr, "latency=%d samples, cpu=%.2f%% of realtime\n", core.latencySamples(), 100.0 * secs / (n / sr));

    FILE* fout = fopen (argv[2], "wb"); fwrite (y.data(), 4, y.size(), fout); fclose (fout);
    printf ("%d\n", core.latencySamples());
}
