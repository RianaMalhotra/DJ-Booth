# ****🎧 DJ Booth (DE1-SoC FPGA)****

## ****📌 Overview****
The DJ Booth is an interactive audio-visual application built on the ****DE1-SoC FPGA platform****. It supports playback of up to five pre-loaded audio tracks using the onboard audio codec, with real-time effects such as ****pitch shifting, reverse playback, echo/reverb, and crossfading****.

A VGA display provides a dynamic UI with animated vinyl discs, live switch/LED states, and visual indicators for active effects. All functionality is controlled through the board’s physical switches and push-buttons.

---

## ****🧠 Features****
- ****🎵 Multi-track audio playback**** (up to 5 tracks)

- ****🎚️ Real-time audio effects:****
  - Pitch shifting (0.8× – 1.2×)
  - Reverse playback
  - Echo / reverb
  - Crossfade between tracks

- ****🖥️ VGA-based animated interface:****
  - Spinning vinyl visualization  
  - Live control panel display  
  - Effect status indicators  

- ****🎛️ Fully hardware-controlled**** via switches and keys

---

## ****🏗️ System Architecture****
- ****Audio Engine****  
  Handles playback, buffering, and effects processing  

- ****Interrupt System****  
  Timer + audio interrupts for real-time performance  

- ****VGA Renderer****  
  Displays UI and animations  

- ****Input Controller****  
  Reads switch and key states for control logic  

---

## ****🧩 Block Diagram****

<img width="1215" height="981" alt="Screenshot 2026-04-02 134516" src="https://github.com/user-attachments/assets/7a035681-33e4-4380-aad7-dd98de1788c6" />
---



## ****🎮 Controls****

| ****Control**** | ****Function**** |
|--------------|----------------|
| SW[9:7] | Volume (0 = mute, 7 = full) |
| SW[6:4] | Pitch (0 = 0.8×, 7 = 1.2×) |
| SW[2]   | Crossfade enable |
| SW[1]   | Echo enable |
| SW[0]   | Reverse playback |
| KEY0    | Restart current track |
| KEY1    | Pause / Resume |
| KEY2    | Previous track |
| KEY3    | Next track |

---

## ****⚙️ Setup & Usage****
1. Load the project onto the ****DE1-SoC FPGA board****  
2. Ensure audio files are preloaded in memory  
3. Connect:
   - Audio output (headphones/speakers)  
   - VGA display  
4. Power on the board  
5. Use switches and keys to control playback and effects  

---

## ****🛠️ Tech Stack****
- ****Languages:**** C, Embedded C  
- ****Hardware:**** DE1-SoC FPGA  
- ****Peripherals:**** Audio Codec, VGA Controller, GPIO  

Concepts:
- Interrupt-driven programming  
- Real-time audio processing  
- Memory-mapped I/O  

---

## ****🚀 Future Improvements****
- Bluetooth / wireless control  
- Additional audio effects (filters, EQ)  
- Improved UI animations  
- Dynamic track loading  

---
