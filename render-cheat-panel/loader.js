const fs = require('fs');
const path = require('path');
const { execFile } = require('child_process');
const https = require('https');
const http = require('http');

// ─── CONFIG ──────────────────────────────────────────────────────
const DATA_DIR = process.env.DATA_DIR || path.join(__dirname, 'data');
const LOADER_CONFIG_FILE = path.join(DATA_DIR, 'loader.json');

// caminhos relativos ao diretório do cheat
const CHEAT_DIR = path.join(__dirname, '..', 'cheat');
const INJECTOR_PATH = path.join(CHEAT_DIR, 'injector_cli.exe');
const DEFAULT_DLL_PATH = path.join(CHEAT_DIR, 'Client.dll');

// ─── CONFIG DEFAULT ──────────────────────────────────────────────
function defaultConfig() {
    return {
        target_process: 'HD-Player.exe',
        dll_url: '',
        dll_path: DEFAULT_DLL_PATH,
        auto_inject: false,
        inject_on_start: false,
        poll_interval: 2000
    };
}

// ─── LOAD / SAVE CONFIG ──────────────────────────────────────────
function ensureDataDir() {
    if (!fs.existsSync(DATA_DIR)) {
        fs.mkdirSync(DATA_DIR, { recursive: true });
    }
}

function loadConfig() {
    ensureDataDir();
    try {
        if (fs.existsSync(LOADER_CONFIG_FILE)) {
            const data = fs.readFileSync(LOADER_CONFIG_FILE, 'utf8');
            return JSON.parse(data);
        }
    } catch (e) { /* fallback */ }
    const cfg = defaultConfig();
    saveConfig(cfg);
    return cfg;
}

function saveConfig(cfg) {
    ensureDataDir();
    fs.writeFileSync(LOADER_CONFIG_FILE, JSON.stringify(cfg, null, 2), 'utf8');
}

// ─── STATUS ────────────────────────────────────────────────────────
function getStatus() {
    const cfg = loadConfig();
    const result = {
        target: cfg.target_process,
        pid: 0,
        dll_exists: false,
        dll_path: cfg.dll_path,
        injector_exists: false,
        config: cfg
    };

    // verifica se o processo existe
    try {
        // usa tasklist pra verificar processo (funciona sem dependências)
        const { execSync } = require('child_process');
        const output = execSync(
            `tasklist /FI "IMAGENAME eq ${cfg.target_process}" /NH /FO CSV`,
            { encoding: 'utf8', timeout: 3000 }
        );
        if (output.includes(cfg.target_process)) {
            // extrai o PID
            const match = output.match(/"([^"]+)","(\d+)"/);
            if (match) result.pid = parseInt(match[2]);
        }
    } catch (e) { /* processo nao encontrado */ }

    // verifica se a dll existe
    result.dll_exists = fs.existsSync(cfg.dll_path);

    // verifica se o injetor existe
    result.injector_exists = fs.existsSync(INJECTOR_PATH);

    return result;
}

// ─── DOWNLOAD DLL ────────────────────────────────────────────────
function downloadDll(url) {
    return new Promise((resolve, reject) => {
        if (!url) {
            return reject(new Error('URL da DLL não configurada'));
        }

        const cfg = loadConfig();
        const outputPath = cfg.dll_path;

        console.log(`[loader] baixando dll: ${url} -> ${outputPath}`);

        const parsedUrl = new URL(url);
        const mod = parsedUrl.protocol === 'https:' ? https : http;

        const file = fs.createWriteStream(outputPath);
        let totalBytes = 0;

        const req = mod.get(url, (res) => {
            // redirecionamento
            if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
                file.close();
                fs.unlinkSync(outputPath);
                return downloadDll(res.headers.location).then(resolve).catch(reject);
            }

            if (res.statusCode !== 200) {
                file.close();
                fs.unlinkSync(outputPath);
                return reject(new Error(`HTTP ${res.statusCode}`));
            }

            res.pipe(file);

            res.on('data', (chunk) => {
                totalBytes += chunk.length;
            });

            file.on('finish', () => {
                file.close();
                console.log(`[loader] download completo: ${totalBytes} bytes`);
                resolve({ bytes: totalBytes, path: outputPath });
            });
        });

        req.on('error', (err) => {
            file.close();
            if (fs.existsSync(outputPath)) fs.unlinkSync(outputPath);
            reject(err);
        });

        req.setTimeout(30000, () => {
            req.destroy();
            file.close();
            if (fs.existsSync(outputPath)) fs.unlinkSync(outputPath);
            reject(new Error('Timeout no download'));
        });
    });
}

// ─── LIST AVAILABLE DLLS ─────────────────────────────────────────
function getAvailableDlls() {
    const results = [];
    const cfg = loadConfig();
    try {
        const files = fs.readdirSync(CHEAT_DIR);
        for (const f of files) {
            if (f.toLowerCase().endsWith('.dll')) {
                const fp = path.join(CHEAT_DIR, f);
                const stat = fs.statSync(fp);
                results.push({ name: f, path: fp, size: stat.size });
            }
        }
    } catch (e) { /* dir not found */ }
    // also check configured dll path
    if (cfg.dll_path && fs.existsSync(cfg.dll_path) && !results.find(r => r.path === cfg.dll_path)) {
        const stat = fs.statSync(cfg.dll_path);
        results.push({ name: path.basename(cfg.dll_path), path: cfg.dll_path, size: stat.size });
    }
    return results;
}

// ─── INJECT ────────────────────────────────────────────────────────
function inject(optDllPath) {
    return new Promise((resolve, reject) => {
        const cfg = loadConfig();
        const targetDll = optDllPath || cfg.dll_path;

        if (!fs.existsSync(INJECTOR_PATH)) {
            return reject(new Error(`injetor não encontrado: ${INJECTOR_PATH}`));
        }
        if (!fs.existsSync(targetDll)) {
            return reject(new Error(`dll não encontrada: ${targetDll}`));
        }

        const args = [
            '--process', cfg.target_process,
            '--dll', targetDll
        ];

        console.log(`[loader] injetando: ${INJECTOR_PATH} ${args.join(' ')}`);

        execFile(INJECTOR_PATH, args, {
            timeout: 10000,
            cwd: CHEAT_DIR
        }, (err, stdout, stderr) => {
            if (err) {
                // erro de execução
                return reject(new Error(`falha ao executar injetor: ${err.message}`));
            }

            try {
                const result = JSON.parse(stdout.trim());
                resolve(result);
            } catch (e) {
                reject(new Error(`resposta inválida do injetor: ${stdout}`));
            }
        });
    });
}

// ─── INJECT VIA NATIVE (fallback sem injector_cli.exe) ──────────
// usa o hd_injector.exe se existir
function injectLegacy() {
    return new Promise((resolve, reject) => {
        const legacyPath = path.join(CHEAT_DIR, 'hd_injector.exe');
        if (!fs.existsSync(legacyPath)) {
            return reject(new Error('nem injector_cli.exe nem hd_injector.exe encontrados'));
        }

        execFile(legacyPath, [], {
            timeout: 15000,
            cwd: CHEAT_DIR
        }, (err, stdout, stderr) => {
            if (err) return reject(new Error(`hd_injector falhou: ${err.message}`));
            resolve({ success: true, output: stdout });
        });
    });
}

// ─── WATCHDOG ──────────────────────────────────────────────────────
// monitora o processo alvo e injeta automaticamente se configurado
let watchdogInterval = null;

function startWatchdog(onStatus) {
    if (watchdogInterval) return;

    const cfg = loadConfig();
    const interval = cfg.poll_interval || 2000;

    console.log(`[loader] watchdog iniciado (intervalo: ${interval}ms)`);

    watchdogInterval = setInterval(() => {
        try {
            const status = getStatus();
            if (onStatus) onStatus(status);

            if (cfg.auto_inject && status.dll_exists && status.pid > 0) {
                inject().then((res) => {
                    console.log('[loader] watchdog: injeção automática:', res);
                }).catch((err) => {
                    console.log('[loader] watchdog: erro na injeção:', err.message);
                });
            }
        } catch (e) {
            console.log('[loader] watchdog error:', e.message);
        }
    }, interval);
}

function stopWatchdog() {
    if (watchdogInterval) {
        clearInterval(watchdogInterval);
        watchdogInterval = null;
        console.log('[loader] watchdog parado');
    }
}

// ─── EXPORTS ──────────────────────────────────────────────────────
module.exports = {
    loadConfig,
    saveConfig,
    defaultConfig,
    getStatus,
    downloadDll,
    inject,
    injectLegacy,
    startWatchdog,
    stopWatchdog,
    getAvailableDlls,
    CHEAT_DIR,
    INJECTOR_PATH
};
