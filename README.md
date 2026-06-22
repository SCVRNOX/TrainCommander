# Train Commander

A Guild Wars 2 Nexus addon for managing and running farm trains.

> Note: This addon was developed with the help of KI. If you disagree with that, please refrain from using it.

## Features

- 📋 **Train Templates** — Create, save, and share ordered sequences of farm events
- ⏱️ **Live Event Catalog** — Horizontal timeline view of all GW2 meta events with accurate live timers
- 📢 **One-click Broadcasting** — Copy waypoints, squad messages and mechanics to clipboard instantly
- 🔁 **Live Countdowns** — Each train step shows a rolling daily countdown to its next scheduled spawn
- 📤 **Import/Export** — Share train templates via Base64 clipboard strings
 - 💬 **Custom Messages & Mechanics** — Add per-train custom squad messages and mechanic notes; save and copy them quickly from the overlay

## Installation (via Nexus) (Coming soon hopefully)

1. Open **Nexus** in Guild Wars 2
2. Search for **Train Commander**
3. Click **Install**

## Manual Installation

1. Download `TrainCommander.dll` from the [latest release](../../releases/latest)
2. Place it in `Guild Wars 2/addons/`
3. Restart the game

## Usage

- Use the **Event Catalog** button to browse the live meta event schedule
- Click an event block to add it to your active train
- Use the in-game overlay to navigate steps and broadcast messages to your squad

##Images
<img width="1287" height="809" alt="image" src="https://github.com/user-attachments/assets/9740b024-c0ad-4e28-8c43-8c7b715f5075" />
<img width="1783" height="721" alt="image" src="https://github.com/user-attachments/assets/04b33d89-ddf0-43c5-8630-02371256921f" />
<img width="373" height="606" alt="image" src="https://github.com/user-attachments/assets/1483b3de-4135-4217-8be8-4e75ed2b1654" />




## Building from Source

**Requirements:**
- Visual Studio 2022 or newer
- Windows SDK

```
msbuild TrainCommander.sln /p:Configuration=Release /p:Platform=x64
```

The compiled DLL will be in `x64/TrainCommander.dll`.
