# 🎧 AirTux Daemon

### Open-source auto-pairing for Bluetooth headphones on Linux

AirTux Daemon is a lightweight background service for Linux that makes Bluetooth headphones behave more like AirPods on Apple devices - but without locking you into one ecosystem.

It automatically detects when your headphones connect, routes audio intelligently, reads notifications aloud, and pauses playback when your headset disconnects unexpectedly. Best of all, it works with virtually any Bluetooth headset: AirPods, Sony, Bose, and more.

Built in modern C++ with a focus on performance and low resource usage, AirTux runs quietly in the background without unnecessary CPU overhead.

---

## ✨ Features

### Universal Bluetooth Support

Works with almost any Bluetooth audio device instead of being tied to a single brand or platform.

### Privacy-First Disconnect Protection

If your headphones disconnect unexpectedly, AirTux can instantly pause all media playback to prevent audio from blasting through your speakers.

### Smart Notification Voice Readouts

Listens for desktop notifications over D-Bus (Discord, Slack, system alerts, etc.) and reads them directly into your headphones using speech synthesis.

### Intelligent Media Control

Detects active media sessions through MPRIS and automatically pauses or resumes the correct audio source.

### Auto-Standby Mode

Disconnects idle devices when you're no longer listening to help preserve battery life.

---

# 🛠️ Configuration

AirTux uses a simple `devices.yaml` configuration file:

```yaml
devices:
  airpods:
    mac: 70:8C:F2:70:87:D3

auto-standby: true

# Modes:
# strict -> pause all audio on disconnect
# tab    -> pause only active media source
# open   -> do nothing on disconnect
audio_mode: strict
```

---

# 🚀 Getting Started

## 1. Install Dependencies

### Ubuntu / Debian

```bash
sudo apt install g++ yaml-cpp libspeechd-dev playerctl bluetoothctl speech-dispatcher
```

### Arch Linux

```bash
sudo pacman -S gcc yaml-cpp speech-dispatcher playerctl bluez-utils
```

---

## 2. Compile

```bash
./compile.sh airpods_daemon
```

---

## 3. Run

```bash
./airpods_daemon
```

---

# 🧪 Testing Notifications

While the daemon is running, send a test desktop notification:

```bash
notify-send Discord "Yo wsp"
```

If configured correctly, the notification should be spoken directly through your connected headphones.

---

# 📜 Architecture Overview

### `main.cpp`

Core daemon loop responsible for Bluetooth monitoring and device state management.

### `tts.cpp / tts.h`

Handles asynchronous text-to-speech processing and D-Bus notification integration.

### `compile.sh`

Simple helper script for fast local builds and deployment.
