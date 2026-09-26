import numpy as np, subprocess, sys, os
SR = int(os.environ.get('SR', 48000))
rng = np.random.default_rng(2)

def run(x, **params):
    x = np.asarray(x, dtype=np.float32)
    x.tofile("/tmp/vin.f32")
    args = ["./runcore", "/tmp/vin.f32", "/tmp/vout.f32", str(SR)] + [f"{k}={v}" for k, v in params.items()]
    out = subprocess.run(args, capture_output=True, text=True)
    lat = int(out.stdout.strip()); print("   ", out.stderr.strip())
    return np.fromfile("/tmp/vout.f32", dtype=np.float32), lat

def tone(f, dur, amp=0.3, sr=None):
    sr = sr or SR
    t = np.arange(int(dur*sr))/sr
    return (amp*np.sin(2*np.pi*f*t)).astype(np.float32)

def db(x): return 20*np.log10(np.maximum(np.abs(x), 1e-12))
def rms_db(x): return 20*np.log10(np.sqrt(np.mean(x.astype(np.float64)**2)) + 1e-12)
def band_energy(x, lo, hi, sr=None):
    sr = sr or SR
    X = np.abs(np.fft.rfft(x*np.hanning(len(x)))); fr = np.fft.rfftfreq(len(x), 1/sr)
    m = (fr>=lo)&(fr<=hi); return np.sqrt(np.mean(X[m]**2)) if m.any() else 0.0

ok = True
def check(name, cond, info=""):
    global ok; ok &= bool(cond); print(("PASS" if cond else "FAIL"), name, info)

allOff = dict(eqOn=0, compOn=0, deessOn=0, satOn=0, limOn=0)

# ---- 1. everything off/neutral -> near-identity (modulo the limiter's lookahead delay when on) ----
print("1) all stages off -> bit-exact passthrough")
x = tone(440, 1.0)
y, lat = run(x, **allOff)
check("bypass is exact", np.max(np.abs(y-x)) < 1e-6, f"(latency {lat})")

# ---- 2. EQ: peak boost/cut measured at the target frequency ----
print("2) EQ mid1 peak: +9 dB boost at 1 kHz")
x = (rng.standard_normal(SR*2)*0.05).astype(np.float32)   # white noise probe
p = dict(allOff); p.update(eqOn=1, hpFreq=20, lowOn=0, mid1On=1, mid1Freq=1000, mid1Gain=9, mid1Q=2.0, mid2On=0, highOn=0)
y, _ = run(x, **p)
def spectrum_ratio(x, y, f, sr=SR, bw=0.15):
    lo, hi = f*(1-bw), f*(1+bw)
    return 20*np.log10(band_energy(y,lo,hi,sr)/max(band_energy(x,lo,hi,sr),1e-9))
boost = spectrum_ratio(x, y, 1000)
print(f"    measured boost at 1kHz: {boost:.1f} dB (expected ~9 dB)")
check("EQ peak boost ~9dB", abs(boost-9) < 1.5)
far = spectrum_ratio(x, y, 100)
print(f"    measured change at 100Hz (should be ~0): {far:.1f} dB")
check("EQ peak doesn't affect 100Hz", abs(far) < 1.0)

# ---- 3. High-pass: attenuates well below cutoff, passes well above ----
print("3) high-pass at 200 Hz")
p = dict(allOff); p.update(eqOn=1, hpFreq=200)
lowTone = tone(50, 1.0); hiTone = tone(1000, 1.0)
yl, _ = run(lowTone, **p); yh, _ = run(hiTone, **p)
attenLow = rms_db(yl[SR//4:]) - rms_db(lowTone[SR//4:])
attenHi  = rms_db(yh[SR//4:]) - rms_db(hiTone[SR//4:])
print(f"    50 Hz attenuation: {attenLow:.1f} dB   1kHz attenuation: {attenHi:.1f} dB")
check("50Hz strongly attenuated (<-15dB)", attenLow < -15)
check("1kHz passes (> -1dB)", attenHi > -1)

# ---- 4. Compressor: gain reduction scales with how far above threshold ----
# Use a very fast attack/release so the envelope tracks the instantaneous peak
# closely; then compare PEAK-to-PEAK (both in the same "peak" domain) against
# the static gain-computer curve, isolating the curve from envelope dynamics.
print("4) compressor: threshold -20dB, ratio 4:1 (static curve, peak-to-peak)")
p = dict(allOff); p.update(compOn=1, thr=-20, ratio=4, knee=0, atk=0.05, rel=2, makeup=0)
for inDb in (-30, -20, -10, 0):
    x = tone(300, 0.6, amp=10**(inDb/20))
    y, _ = run(x, **p)
    outDb = 20*np.log10(np.max(np.abs(y[SR//4:])) + 1e-9)
    expected = inDb if inDb <= -20 else -20 + (inDb-(-20))/4
    print(f"    in {inDb:+4d} dB -> out {outDb:6.2f} dB  (expected ~{expected:6.2f} dB)")
    check(f"compressor gain at {inDb}dB in", abs(outDb-expected) < 1.0)

# ---- 4b. Makeup gain and knee sanity ----
print("4b) makeup gain shifts output up; softer knee rounds the corner")
p2 = dict(p); p2["makeup"] = 6
x = tone(300, 0.6, amp=10**(-10/20))
y, _ = run(x, **p2)
outDb = 20*np.log10(np.max(np.abs(y[SR//4:])) + 1e-9)
print(f"    with +6dB makeup: {outDb:.2f} dB (was {-20 + (-10-(-20))/4:.2f} dB without)")
check("makeup gain adds ~6dB", abs(outDb - (-20 + (-10-(-20))/4 + 6)) < 1.0)

# ---- 5. De-esser: attenuates a sibilant tone, leaves a low tone alone ----
print("5) de-esser: freq 6500Hz, threshold -30dB, range 15dB")
p = dict(allOff); p.update(deessOn=1, deessFreq=6500, deessThr=-30, deessRange=15)
sTone = tone(6500, 1.0, amp=0.5); lowT = tone(300, 1.0, amp=0.5)
ys, _ = run(sTone, **p); yl2, _ = run(lowT, **p)
redS = rms_db(ys[SR//4:]) - rms_db(sTone[SR//4:])
redL = rms_db(yl2[SR//4:]) - rms_db(lowT[SR//4:])
print(f"    sibilant (6.5kHz) reduction: {redS:.1f} dB    low tone (300Hz) change: {redL:.1f} dB")
check("sibilance reduced by 8-16dB", -16 < redS < -8)
check("low tone untouched (<1dB)", abs(redL) < 1.0)

# ---- 6. Saturator: more drive -> more harmonic energy; mix=0 -> untouched ----
print("6) saturator harmonics")
x = tone(300, 1.0, amp=0.7)
p0 = dict(allOff); p0.update(satOn=1, drive=0.0, warmth=0.0, mix=1.0)
p1 = dict(allOff); p1.update(satOn=1, drive=0.9, warmth=0.5, mix=1.0)
y0, _ = run(x, **p0); y1, _ = run(x, **p1)
h0 = band_energy(y0[SR//4:], 550, 1200); h1 = band_energy(y1[SR//4:], 550, 1200)   # 2nd/3rd harmonic region of 300Hz
print(f"    harmonic energy: drive=0 -> {20*np.log10(h0+1e-9):.1f} dB, drive=0.9 -> {20*np.log10(h1+1e-9):.1f} dB")
check("more drive makes more harmonics", h1 > h0 * 3)
p2 = dict(allOff); p2.update(satOn=1, drive=0.9, warmth=0.5, mix=0.0)
y2, _ = run(x, **p2)
check("mix=0 leaves signal unchanged", np.max(np.abs(y2-x)) < 1e-5)

# ---- 7. Limiter: never exceeds ceiling, reports correct lookahead latency ----
print("7) limiter ceiling -1dB on a signal peaking at +6dB")
p = dict(allOff); p.update(limOn=1, ceiling=-1.0, limRelease=50)
x = tone(300, 1.0, amp=2.0)  # ~+6dBFS
y, lat = run(x, **p)
peak = np.max(np.abs(y[lat+200:]))
print(f"    output peak: {20*np.log10(peak):.2f} dBFS (ceiling -1.0 dB), reported latency {lat} samples")
check("output never exceeds ceiling (+0.1dB tolerance)", 20*np.log10(peak) < -0.9)
check("limiter reports nonzero lookahead latency", lat > 0)
p_off = dict(allOff)
_, lat_off = run(x, **p_off)
check("latency is 0 when limiter disabled", lat_off == 0)

# ---- 8. Full chain: a vocal-like multi-tone signal comes out louder & bounded ----
print("8) full chain on a vocal-like test signal (fundamental + harmonics + sibilant burst)")
t = np.arange(int(1.5*SR))/SR
voice = 0.25*np.sin(2*np.pi*180*t) + 0.12*np.sin(2*np.pi*360*t) + 0.06*np.sin(2*np.pi*720*t)
sib = np.zeros_like(voice); sib[int(0.5*SR):int(0.6*SR)] = 0.4*rng.standard_normal(int(0.1*SR))
sibF = np.abs(np.fft.rfft(sib*np.hanning(len(sib))))
full = (voice + sib).astype(np.float32)
y, lat = run(full)     # defaults: everything on
print(f"    input RMS {rms_db(full):.1f} dB -> output RMS {rms_db(y):.1f} dB, peak {20*np.log10(np.max(np.abs(y))+1e-9):.2f} dBFS, latency {lat}")
check("output peak stays under -0.1 dBFS (default ceiling -0.3dB)", np.max(np.abs(y)) < 10**(-0.1/20))
check("chain doesn't produce NaN/Inf", np.all(np.isfinite(y)))

print("\nALL PASS" if ok else "\nSOME FAILED")
sys.exit(0 if ok else 1)
