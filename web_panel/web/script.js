const BASE = '';
let state = {};
let startTime = Date.now();

// ─── API ──────────────────────────────────────────────────────────
async function api(endpoint, method = 'GET') {
    try {
        const r = await fetch(`${BASE}${endpoint}`, { method });
        return await r.json();
    } catch (e) {
        return null;
    }
}

// ─── UI ────────────────────────────────────────────────────────────
function updateBadge(s) {
    const badge = document.getElementById('statusBadge');
    if (s.injected) {
        badge.textContent = 'injetado';
        badge.className = 'badge online';
    } else if (s.dll_ready) {
        badge.textContent = 'pronto';
        badge.className = 'badge online';
    } else if (s.downloading) {
        badge.textContent = 'baixando...';
        badge.className = 'badge downloading';
    } else {
        badge.textContent = 'offline';
        badge.className = 'badge offline';
    }
}

function updateInfo(s) {
    document.getElementById('targetField').textContent = s.target || 'HD-Player.exe';
    document.getElementById('pidField').textContent = s.pid && s.pid > 0 ? s.pid : '—';

    const dllEl = document.getElementById('dllField');
    if (s.dll_ready) {
        dllEl.textContent = '✅ pronta';
        dllEl.style.color = '#4ade80';
    } else if (s.downloading) {
        dllEl.textContent = '⏳ baixando...';
        dllEl.style.color = '#facc15';
    } else {
        dllEl.textContent = '❌ ausente';
        dllEl.style.color = '#f87171';
    }

    const injEl = document.getElementById('injectField');
    if (s.injected) {
        injEl.textContent = '✅ injetado';
        injEl.style.color = '#4ade80';
    } else if (s.pid && s.pid > 0) {
        injEl.textContent = '⏳ pendente';
        injEl.style.color = '#facc15';
    } else {
        injEl.textContent = '—';
        injEl.style.color = '#777';
    }
}

function updateLog(s) {
    const el = document.getElementById('logOutput');
    if (s.log) el.textContent = s.log;
}

function updateButtons(s) {
    const btnDl = document.getElementById('btnDownload');
    const btnIj = document.getElementById('btnInject');

    btnDl.disabled = s.downloading || s.dll_ready;
    btnIj.disabled = !s.dll_ready || s.injected || !(s.pid && s.pid > 0);
}

function updateUptime() {
    const elapsed = Math.floor((Date.now() - startTime) / 1000);
    const mins = Math.floor(elapsed / 60);
    const secs = elapsed % 60;
    document.getElementById('uptimeField').textContent =
        `online — ${mins}m ${secs.toString().padStart(2, '0')}s`;
}

function updateUI(s) {
    if (!s) {
        document.getElementById('logOutput').textContent = '[system] servidor offline';
        document.getElementById('statusBadge').textContent = 'offline';
        document.getElementById('statusBadge').className = 'badge offline';
        return;
    }
    state = s;
    updateBadge(s);
    updateInfo(s);
    updateLog(s);
    updateButtons(s);
}

// ─── POLLING ───────────────────────────────────────────────────────
async function poll() {
    const s = await api('/api/status');
    updateUI(s);
    updateUptime();
}

// ─── EVENTOS ───────────────────────────────────────────────────────
document.getElementById('btnDownload').addEventListener('click', async () => {
    document.getElementById('logOutput').textContent = '[system] iniciando download...';
    await api('/api/download', 'POST');
    setTimeout(poll, 500);
});

document.getElementById('btnInject').addEventListener('click', async () => {
    document.getElementById('logOutput').textContent = '[system] injetando...';
    await api('/api/inject', 'POST');
    setTimeout(poll, 500);
});

// ─── START ─────────────────────────────────────────────────────────
poll();
setInterval(poll, 2000);
