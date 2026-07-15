const express = require('express');
const https = require('https');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const app = express();
const PORT = process.env.PORT || 3000;
const DATA_DIR = process.env.DATA_DIR || path.join(__dirname, 'data');
const OPTIONS_FILE = path.join(DATA_DIR, 'options.json');
const AUTH_API = 'https://satella-auth-bot.onrender.com';

// ─── helpers ──────────────────────────────────────────────────

app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

function ensureDataDir() {
    if (!fs.existsSync(DATA_DIR)) fs.mkdirSync(DATA_DIR, { recursive: true });
}

function httpsPost(url, data) {
    return new Promise((resolve, reject) => {
        const urlObj = new URL(url);
        const body = JSON.stringify(data);
        const options = {
            hostname: urlObj.hostname,
            port: 443,
            path: urlObj.pathname,
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Content-Length': Buffer.byteLength(body)
            },
            rejectUnauthorized: false
        };
        const req = https.request(options, (res) => {
            let respData = '';
            res.on('data', chunk => respData += chunk);
            res.on('end', () => {
                try { resolve(JSON.parse(respData)); }
                catch (e) { resolve({ success: false, error: 'parse error' }); }
            });
        });
        req.on('error', (e) => resolve({ success: false, error: e.message }));
        req.write(body);
        req.end();
    });
}

// ─── default options ──────────────────────────────────────────

function getDefaultOptions() {
    return {
        sections: [
            {
                name: 'AimBot',
                options: [
                    { k: 'Enabled', t: 'bool', v: false },
                    { k: 'Precision', t: 'bool', v: false },
                    { k: 'ScopeTracking', t: 'bool', v: false },
                    { k: 'Pixel', t: 'bool', v: false },
                    { k: 'IgnoreKnocked', t: 'bool', v: false },
                    { k: 'TargetNPC', t: 'bool', v: false },
                    { k: 'PullCamAfterKill', t: 'bool', v: false },
                    { k: 'MaxDistance', t: 'int', v: 300, min: 0, max: 600 },
                    { k: 'DelayMode', t: 'combo', v: 0, opts: ['0 Peito', '1 Peito', '2 Peitos', '3 Peitos', '4 Peitos', '5 Peitos', 'Random'] },
                    { k: 'AimType', t: 'combo', v: 0, opts: ['Aim Rage', 'Aim Neck', 'Aim Legit'] },
                    { k: 'FOV', t: 'float', v: 180, min: 0, max: 360 },
                    { k: 'SmoothH', t: 'float', v: 10, min: 0, max: 200, l: 'Smooth Horizontal' },
                    { k: 'SmoothV', t: 'float', v: 10, min: 0, max: 200, l: 'Smooth Vertical' },
                    { k: 'ColliderFOV', t: 'float', v: 180, min: 0, max: 360 },
                    { k: 'ColliderVisibleOnly', t: 'bool', v: false },
                    { k: 'PullCamHeight', t: 'float', v: 0.5, min: 0.1, max: 1.0 }
                ]
            },
            {
                name: 'ESP Players',
                options: [
                    { k: 'Enabled', t: 'bool', v: false },
                    { k: 'ShowLocalPlayer', t: 'bool', v: false },
                    { k: 'ShowNPCs', t: 'bool', v: false },
                    { k: 'Box', t: 'bool', v: false },
                    { k: 'BoxStyle', t: 'combo', v: 0, opts: ['Full', 'Cornered'] },
                    { k: 'Skeleton', t: 'bool', v: false },
                    { k: 'Name', t: 'bool', v: false },
                    { k: 'HealthBar', t: 'bool', v: false },
                    { k: 'HealthBarPos', t: 'combo', v: 0, opts: ['Left', 'Right', 'Top', 'Bottom'] },
                    { k: 'ArmorBar', t: 'bool', v: false },
                    { k: 'WeaponName', t: 'bool', v: false },
                    { k: 'Distance', t: 'bool', v: false },
                    { k: 'SnapLines', t: 'bool', v: false },
                    { k: 'SnapLinesPos', t: 'combo', v: 0, opts: ['Top', 'Bottom'] },
                    { k: 'RenderDistance', t: 'int', v: 300, min: 0, max: 600 }
                ]
            },
            {
                name: 'Exploits',
                options: [
                    { k: 'NoRecoil', t: 'bool', v: false },
                    { k: 'NoRecoilStrength', t: 'float', v: 100, min: 0, max: 100, l: 'Recoil Reduction %' },
                    { k: 'WeaponAttributes', t: 'bool', v: false },
                    { k: 'WeaponAttributesLevel', t: 'combo', v: 0, opts: ['LV 1', 'LV 2', 'LV 3', 'LV 4'] },
                    { k: 'SpinBot', t: 'bool', v: false },
                    { k: 'SpinBotSpeed', t: 'float', v: 10, min: 0, max: 50 },
                    { k: 'SpeedHack', t: 'bool', v: false },
                    { k: 'SpeedMultiplier', t: 'float', v: 2, min: 1, max: 10 },
                    { k: 'FastMedkit', t: 'bool', v: false },
                    { k: 'FastReload', t: 'bool', v: false },
                    { k: 'SilentAimEnabled', t: 'bool', v: false },
                    { k: 'SilentAimUseFOV', t: 'bool', v: false },
                    { k: 'SilentAimFOV', t: 'float', v: 180, min: 0, max: 360 },
                    { k: 'SilentAimDistance', t: 'int', v: 300, min: 0, max: 600 },
                    { k: 'SilentAimHitBox', t: 'combo', v: 0, opts: ['Head', 'Neck', 'Chest'] },
                    { k: 'SilentAimTargetBots', t: 'bool', v: false }
                ]
            },
            {
                name: 'General',
                options: [
                    { k: 'CaptureBypass', t: 'bool', v: false },
                    { k: 'StreamMode', t: 'bool', v: false },
                    { k: 'BypassEnabled', t: 'bool', v: false },
                    { k: 'MenuToggleKey', t: 'int', v: 45, min: 0, max: 255 }
                ]
            }
        ]
    };
}

function loadOptions() {
    ensureDataDir();
    try {
        if (fs.existsSync(OPTIONS_FILE)) {
            const data = fs.readFileSync(OPTIONS_FILE, 'utf8');
            return JSON.parse(data);
        }
    } catch (e) { /* fallback */ }
    const defaults = getDefaultOptions();
    saveOptions(defaults);
    return defaults;
}

function saveOptions(opts) {
    ensureDataDir();
    fs.writeFileSync(OPTIONS_FILE, JSON.stringify(opts, null, 2), 'utf8');
}

// ─── auth store (memory, resets on restart) ───────────────────
// sessionId -> { token, username, expires }
const sessions = new Map();

// ─── loader ──────────────────────────────────────────────────
const loader = require('./loader');

// Loader config
app.get('/api/loader/config', (req, res) => {
    const cfg = loader.loadConfig();
    res.json(cfg);
});

app.post('/api/loader/config', (req, res) => {
    const cfg = req.body;
    if (!cfg) return res.json({ success: false, error: 'body required' });
    loader.saveConfig(cfg);
    res.json({ success: true });
});

app.post('/api/loader/config/default', (req, res) => {
    const cfg = loader.defaultConfig();
    loader.saveConfig(cfg);
    res.json(cfg);
});

// Loader status
app.get('/api/loader/status', (req, res) => {
    const status = loader.getStatus();
    res.json(status);
});

// List available DLLs
app.get('/api/loader/dlls', (req, res) => {
    const dlls = loader.getAvailableDlls();
    res.json(dlls);
});

// Serve DLL file
app.get('/api/loader/dll/:name', (req, res) => {
    const name = req.params.name;
    const dlls = loader.getAvailableDlls();
    const found = dlls.find(d => d.name === name);
    if (!found) return res.status(404).json({ error: 'dll not found' });
    res.download(found.path, name);
});

// Special case: serve cheat.dll from public/ (used by ghost)  
app.get('/api/loader/dll/cheat.dll', (req, res) => {
    const dllPath = path.join(__dirname, 'public', 'cheat.dll');
    if (fs.existsSync(dllPath)) {
        res.download(dllPath, 'cheat.dll');
    } else {
        res.status(404).json({ error: 'cheat.dll not found' });
    }
});

// Download DLL
app.post('/api/loader/download', async (req, res) => {
    const { url } = req.body;
    try {
        const result = await loader.downloadDll(url || '');
        res.json({ success: true, bytes: result.bytes, path: result.path });
    } catch (e) {
        res.json({ success: false, error: e.message });
    }
});

// Inject
app.post('/api/loader/inject', async (req, res) => {
    const { dll_path } = req.body || {};
    try {
        const result = await loader.inject(dll_path);
        res.json(result);
    } catch (e) {
        res.json({ success: false, error: e.message });
    }
});

app.post('/api/loader/inject/legacy', async (req, res) => {
    try {
        const result = await loader.injectLegacy();
        res.json(result);
    } catch (e) {
        res.json({ success: false, error: e.message });
    }
});

// Get local IP for mobile access
app.get('/api/loader/ip', (req, res) => {
    const os = require('os');
    const interfaces = os.networkInterfaces();
    let ips = [];
    for (const name of Object.keys(interfaces)) {
        for (const iface of interfaces[name]) {
            if (iface.family === 'IPv4' && !iface.internal) {
                ips.push(iface.address);
            }
        }
    }
    res.json({ ips, hostname: os.hostname() });
});

// Watchdog
app.post('/api/loader/watchdog/start', (req, res) => {
    loader.startWatchdog();
    res.json({ success: true, message: 'watchdog iniciado' });
});

app.post('/api/loader/watchdog/stop', (req, res) => {
    loader.stopWatchdog();
    res.json({ success: true, message: 'watchdog parado' });
});

// ─── ghost agent ───────────────────────────────────────────────
// Command queue for ghost app polling
let ghostCommands = {};
let ghostStatuses = {};

// Ghost polls for commands
app.get('/api/ghost/poll', (req, res) => {
    const id = req.query.id || 'unknown';
    const cmd = ghostCommands[id] || null;
    if (cmd) {
        delete ghostCommands[id];
        return res.json({ command: cmd });
    }
    res.json({ command: null });
});

// Ghost reports status
app.post('/api/ghost/status', (req, res) => {
    const { id, status, extra, target } = req.body;
    if (id) {
        ghostStatuses[id] = {
            status: status || 'unknown',
            extra: extra || '',
            target: target || '',
            lastSeen: Date.now()
        };
    }
    res.json({ success: true });
});

// Web panel sends command to ghost
app.post('/api/ghost/command', (req, res) => {
    const { id, command } = req.body;
    if (!id || !command) {
        return res.json({ success: false, error: 'id and command required' });
    }
    ghostCommands[id] = command;
    console.log(`[ghost] command ${command} queued for ${id}`);
    res.json({ success: true, command });
});

// Web panel checks ghost status
app.get('/api/ghost/status', (req, res) => {
    const ghosts = [];
    const now = Date.now();
    for (const [id, data] of Object.entries(ghostStatuses)) {
        ghosts.push({
            id,
            ...data,
            online: (now - data.lastSeen) < 30000 // 30s timeout
        });
    }
    res.json({ ghosts });
});

// ─── routes ──────────────────────────────────────────────────

// Auth proxy
app.post('/api/auth/key', async (req, res) => {
    const { key } = req.body;
    if (!key) return res.json({ success: false, error: 'Key required' });

    // try register
    let result = await httpsPost(AUTH_API + '/api/register', {
        username: key, password: key, key: key, hwid: ''
    });
    if (result.success) {
        const sessionId = crypto.randomBytes(16).toString('hex');
        sessions.set(sessionId, { token: result.token, username: key, expires: Date.now() + 3600000 });
        return res.json({ success: true, token: result.token, session: sessionId });
    }

    // try login
    result = await httpsPost(AUTH_API + '/api/login', {
        username: key, password: key, hwid: ''
    });
    if (result.success) {
        const sessionId = crypto.randomBytes(16).toString('hex');
        sessions.set(sessionId, { token: result.token, username: key, expires: Date.now() + 3600000 });
        return res.json({ success: true, token: result.token, session: sessionId });
    }

    res.json({ success: false, error: result.error || 'Invalid key' });
});

app.post('/api/auth/verify', async (req, res) => {
    const { token } = req.body;
    if (!token) return res.json({ success: false, error: 'Token required' });
    const result = await httpsPost(AUTH_API + '/api/verify', { token });
    res.json(result);
});

// Options API
app.get('/api/options', (req, res) => {
    const opts = loadOptions();
    res.json(opts);
});

app.get('/api/set', (req, res) => {
    const { path: optPath, value } = req.query;
    if (!optPath || value === undefined) {
        return res.json({ success: false, error: 'path and value required' });
    }
    updateOption(optPath, value);
    res.json({ success: true });
});

app.post('/api/set', (req, res) => {
    const { path: optPath, value } = req.body;
    if (!optPath || value === undefined) {
        return res.json({ success: false, error: 'path and value required' });
    }
    updateOption(optPath, value);
    res.json({ success: true });
});

app.post('/api/option', (req, res) => {
    const { path: optPath, value } = req.body;
    if (!optPath || value === undefined) {
        return res.json({ success: false, error: 'path and value required' });
    }
    updateOption(optPath, value);
    res.json({ success: true });
});

// Poll endpoint - used by the cheat DLL to get current options
app.get('/api/poll', (req, res) => {
    const opts = loadOptions();
    // add a version/changed flag so the cheat knows if options changed
    res.json({
        changed: true,
        options: opts
    });
});

// Bypass endpoint
app.post('/api/bypass', (req, res) => {
    const opts = loadOptions();
    const general = opts.sections[3].options;
    const bypass = general.find(o => o.k === 'BypassEnabled');
    if (bypass) bypass.v = true;
    saveOptions(opts);
    res.json({ success: true, message: 'Bypass activated' });
});

// ─── update option helper ────────────────────────────────────

function updateOption(optPath, value) {
    const opts = loadOptions();
    // path format: "SectionName.OptionKey" or "SectionName.Subsection.OptionKey"
    const parts = optPath.split('.');
    const optionKey = parts[parts.length - 1];

    for (const section of opts.sections) {
        for (const opt of section.options) {
            if (opt.k === optionKey) {
                const val = String(value);
                switch (opt.t) {
                    case 'bool': opt.v = (val === 'true' || val === '1'); break;
                    case 'int': opt.v = parseInt(val) || 0; break;
                    case 'float': opt.v = parseFloat(val) || 0; break;
                    case 'combo': opt.v = parseInt(val) || 0; break;
                    default: opt.v = val;
                }
                saveOptions(opts);
                return;
            }
        }
    }
}

// ─── serve frontend for all non-API routes ───────────────────
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// ─── start ────────────────────────────────────────────────────

app.listen(PORT, '0.0.0.0', () => {
    console.log(`[satella-panel] running on port ${PORT}`);
    console.log(`[satella-panel] data dir: ${DATA_DIR}`);
});
