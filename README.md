# Train Commander

A Guild Wars 2 Nexus addon for managing and running farm trains.

> Note: This addon was developed with the help of artificial intelligence. If you disagree with that, please refrain from using it.

## Features

- 📋 **Train Templates** — Create, save, and share ordered sequences of farm events
- ⏱️ **Live Event Catalog** — Horizontal timeline view of all GW2 meta events with accurate live timers
- 📢 **One-click Broadcasting** — Copy waypoints, squad messages and mechanics to clipboard instantly
- 🔁 **Live Countdowns** — Each train step shows a rolling daily countdown to its next scheduled spawn
- 📤 **Import/Export** — Share train templates via Base64 clipboard strings
 - 💬 **Custom Messages & Mechanics** — Add per-train custom squad messages and mechanic notes; save and copy them quickly from the overlay

## Installation (via Nexus)

1. Open **Nexus** in Guild Wars 2
2. Search for **Train Commander**
3. Click **Install**

## Manual Installation

1. Download `TrainCommander.dll` from the [latest release](../../releases/latest)
2. Place it in `Guild Wars 2/addons/`
3. Restart the game

## Usage

- Press **Shift+Alt+T** to open the Train Editor
- Use the **Event Catalog** button to browse the live meta event schedule
- Click an event block to add it to your active train
- Use the in-game overlay to navigate steps and broadcast messages to your squad

## Building from Source

**Requirements:**
- Visual Studio 2022 or newer
- Windows SDK

```
msbuild TrainCommander.sln /p:Configuration=Release /p:Platform=x64
```

The compiled DLL will be in `x64/TrainCommander.dll`.
