# Nyx Project — Complete Setup Guide

> Multi-component cheat suite for HD-Player.exe (BlueStacks)
> Components: cheat DLL, ghost agent, web panel, auth bot

---

## Table of Contents

1. [Prerequisites](#1-prerequisites)
2. [Building Everything](#2-building-everything)
3. [Auth Bot (Discord + Flask)](#3-auth-bot-discord--flask)
4. [Render Cheat Panel (Node.js)](#4-render-cheat-panel-nodejs)
5. [Ghost Agent](#5-ghost-agent)
6. [Cheat DLL + Injector](#6-cheat-dll--injector)
7. [Web Panel (Standalone C++)](#7-web-panel-standalone-c)
8. [Architecture & Data Flow](#8-architecture--data-flow)

---

## 1. Prerequisites

| Tool | Required For | Get It |
|------|-------------|--------|
| Visual Studio Build Tools | C++ projects (cheat, ghost, web_panel, injector) | [VS Build Tools](https://visualstudio.microsoft.com/visual-cpp-build-tools/) — include "Desktop development with C++" and "MSVC v143" |
| Python 3.10+ | Auth Bot | [python.org](https://python.org) |
| Node.js 18+ | Render Cheat Panel | [nodejs.org](https://nodejs.org) |
| Git (optional) | Version control | [git-scm.com](https://git-scm.com) |

**Visual Studio Note:** You need the *Desktop development with C++* workload. During install, also select:
- MSVC v143 - VS 2022 C++ x64/x86 build tools
- Windows 10/11 SDK
- C++ ATL for v143 (optional)
- **MASM (ml64.exe)** — this is under "Individual components" → search for "MASM"

---

## 2. Building Everything

### Quick Start (if VS tools are in PATH)
```batch
build_all.bat
```

### Build Individual Components

**Injector CLI:**
```batch
cd cheat
build_injector.bat
```

**Cheat DLL:**
```batch
cd cheat
cl /EHsc /std:c++17 /O2 /W3 /LD /Fe:cheat.dll cheat.cpp /link kernel32.lib user32.lib gdi32.lib winmm.lib ws2_32.lib /DEF:cheat.def /DLL
```

**Ghost Agent:**
```batch
cd ghost
compile_link.bat
```

**Web Panel:**
```batch
cd web_panel
build.bat
```

**Node.js Panel:**
```batch
cd render-cheat-panel
npm install
```

---

## 3. Auth Bot (Discord + Flask)

### Local Setup

1. **Create a Discord Application:**
   - Go to https://discord.com/developers/applications
   - Create a new application → Bot → Copy token
   - Enable all Privileged Gateway Intents (Presence, Server Members, Message Content)

2. **Configure:**
   ```batch
   cd auth_bot-20260715T072529Z-1-001\auth_bot
   python setup.py
   ```
   Or manually edit `config.json`:
   ```json
   {
     "token": "your_discord_bot_token",
     "admin_ids": [1512518620373324047],
     "api_port": 5000,
     "admins": { "s": "2200922", "mutano": "902209" },
     "webhook_url": "your_webhook_url"
   }
   ```

3. **Install dependencies:**
   ```batch
   pip install -r requirements.txt
   ```

4. **Run:**
   ```batch
   python auth_bot.py
   ```
   Or use the launcher:
   ```batch
   run.bat
   ```

### Deploy on Railway

1. Push to GitHub
2. Connect repo on [Railway](https://railway.app)
3. Set environment variables:
   - `DISCORD_TOKEN`
   - `API_PORT` (optional)
   - `DATABASE_URL` (optional, defaults to SQLite)
   - `WEBHOOK_URL` (optional)
   - `ADMIN_IDS` (optional)
4. Railway auto-detects `railway.json` and `Procfile`

### API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/api/register` | POST | Register with key |
| `/api/login` | POST | Login with key |
| `/api/verify` | POST | Verify token |
| `/api/status` | GET | Server status |

---

## 4. Render Cheat Panel (Node.js)

### Local Development

```batch
cd render-cheat-panel
npm install
node server.js
```

Opens on `http://localhost:3000`

### Deploy on Render

1. Push to GitHub
2. Create a new **Web Service** on [Render](https://render.com)
3. Connect repo → Select `render-cheat-panel` as root directory
4. Build command: `npm install`
5. Start command: `node server.js`
6. Add environment variables:
   - `PORT` — Render sets this automatically
   - `AUTH_API` — URL of your auth bot (default: `https://satella-auth-bot.onrender.com`)

### Frontend Features

The panel at `public/index.html` includes:
- Login with key authentication
- Options panel (AimBot, ESP, Exploits, General)
- Loader controls (inject, watchdog, DLL management)
- Ghost agent status monitor

---

## 5. Ghost Agent

The ghost agent runs on the target machine, polls the panel server for commands, and injects the cheat DLL.

### Build

```batch
cd ghost
compile_link.bat
```

### Usage

```batch
ghost.exe --server=https://your-panel.onrender.com
```

### Options

| Flag | Description |
|------|-------------|
| `--server` | Panel server URL (required) |
| `--id` | Custom agent ID (optional, auto-generated) |
| `--interval` | Poll interval in ms (default: 5000) |

### Embedding the DLL

To make the ghost agent fully self-contained (no cheat.dll on disk):
1. Build `cheat.dll`
2. Run the Python script to generate `loader.h`:
   ```batch
   cd cheat
   python dll_to_header.py cheat.dll ../ghost/loader.h
   ```
3. Rebuild ghost:
   ```batch
   cd ghost
   compile_link.bat
   ```

---

## 6. Cheat DLL + Injector

### Building the DLL

```batch
cd cheat
cl /EHsc /std:c++17 /O2 /W3 /LD /Fe:cheat.dll cheat.cpp /link kernel32.lib user32.lib gdi32.lib winmm.lib ws2_32.lib /DEF:cheat.def /DLL
```

### Building the Injector

```batch
cd cheat
build_injector.bat
```

### Injecting Manually

```batch
injector_cli.exe --process HD-Player.exe --dll cheat.dll
```

### Web Server Mode

The cheat DLL includes an embedded HTTP server for real-time config changes:
- Listens on a configurable port
- Polls the panel server for options
- Updates features in real-time

---

## 7. Web Panel (Standalone C++)

A lightweight standalone HTTP server for download + inject functionality.

### Build

```batch
cd web_panel
build.bat
```

### Run

```batch
web_panel.exe
```

Opens on `http://localhost:8080`

---

## 8. Architecture & Data Flow

```
┌─────────────────────────────────────────────────────────┐
│                     Internet                             │
│                                                          │
│  ┌──────────────┐    ┌──────────────┐    ┌───────────┐  │
│  │  Auth Bot     │◄───│  Cheat Panel │    │  Render   │  │
│  │  (Railway)    │    │  (Node.js)   │    │  Hosting  │  │
│  │  port 5000    │    │  port 3000   │    │           │  │
│  └──────┬───────┘    └──────┬───────┘    └───────────┘  │
│         │                   │                            │
│         ▼                   ▼                            │
│  ┌──────────────────────────────────────────┐            │
│  │           Ghost Agent (target PC)         │            │
│  │  polls panel → gets commands → injects   │            │
│  └──────────────────┬───────────────────────┘            │
│                     │                                    │
│                     ▼                                    │
│  ┌──────────────────────────────────────────┐            │
│  │           HD-Player.exe (BlueStacks)      │            │
│  │           ← cheat.dll injected            │            │
│  └──────────────────────────────────────────┘            │
│                                                          │
│  Local Access:                                           │
│  ┌──────────────┐                                        │
│  │  Web Panel    │  (standalone C++, port 8080)          │
│  │  or browse to│  panel server IP for web UI           │
│  └──────────────┘                                        │
└─────────────────────────────────────────────────────────┘
```

### Key Communication Paths

1. **User → Panel:** Opens web UI → enters key → gets session
2. **Panel → Auth Bot:** Proxies key validation to auth API
3. **Ghost → Panel:** Polls `/api/ghost/poll` every N seconds for commands
4. **Panel → Ghost:** Commands queued via `/api/ghost/command`
5. **Cheat DLL → Panel:** Polls `/api/poll` for option changes
6. **Panel → Cheat DLL:** Serves options via `/api/options`

---

## Troubleshooting

### "cl.exe not found"
Open **"Developer Command Prompt for VS 2022"** from Start Menu, then run build scripts.

### "ml64.exe not found"
Re-run VS Installer → Modify → Individual Components → Search "MASM" → Install.

### "Discord token invalid"
Regenerate token in Discord Developer Portal. Ensure bot is invited to a guild with proper intents.

### Ghost won't connect
- Verify `--server` URL is correct and panel is running
- Check firewall isn't blocking outbound HTTPS
- Use `--verbose` flag for debug output

### Auth bot won't start
- Run `python setup.py` to verify configuration
- Ensure all dependencies installed: `pip install -r requirements.txt`
- Check token permissions in Discord Developer Portal

### npm install fails
- Update Node.js to v18+
- Delete `node_modules` and `package-lock.json`, retry
- Check internet connection
