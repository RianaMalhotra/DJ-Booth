#found online
with open("output_8khz.wav", "rb") as f:
    data = f.read()[44:]  # skip WAV header

samples = []
for i in range(0, len(data)-1, 2):
    val = int.from_bytes(data[i:i+2], 'little', signed=True)
    samples.append(val << 8)  # scale to 24-bit

with open("audio_data.h", "w") as f:
    f.write("static const int audio_samples[] = {\n")
    for s in samples[:8000]:  # 1 second = 8000 samples
        f.write(f"    {s},\n")
    f.write("};\n")
    f.write(f"#define AUDIO_LEN {min(len(samples), 8000)}\n")

print("Done! audio_data.h created.")