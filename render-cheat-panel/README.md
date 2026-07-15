# Satella Cheat Panel — Render.com

Web panel + auth + options sync para o cheat. Roda 100% na nuvem, sem expor teu PC.

## Deploy no Render

### 1. Cria um repositório no GitHub

```bash
# Na tua máquina:
cd render-cheat-panel
git init
git add .
git commit -m "initial"
# cria um repositorio no GitHub e:
git remote add origin https://github.com/TEU_USUARIO/render-cheat-panel.git
git push -u origin main
```

### 2. Faz deploy no Render

1. Vai em **[render.com](https://render.com)** → Dashboard → **New +** → **Web Service**
2. Conecta teu GitHub e seleciona o repo
3. Configura:
   - **Name:** `satella-panel` (ou o que quiser)
   - **Runtime:** `Node`
   - **Build Command:** `npm install`
   - **Start Command:** `node server.js`
   - **Plan:** `Free`
4. Em **Advanced** → **Add Environment Variable**:
   - `DATA_DIR = /data` (ou deixa vazio, ele usa a pasta `data/` padrão)
5. Clica em **Create Web Service**

Pronto. Depois de alguns minutos, Render vai te dar uma URL tipo:
```
https://satella-panel.onrender.com
```

### 3. Usando

- Abre a URL no navegador
- Faz login com tua key (válida na API da satella)
- O painel funciona igual ao localhost

## Como o cheat se conecta

O cheat DLL precisa ser adaptado pra:
1. **NÃO** iniciar o servidor web local
2. Ao invés disso, **pollar** o Render a cada 1-2 segundos:
   ```
   GET https://satella-panel.onrender.com/api/poll
   ```
3. Aplicar as opções recebidas no jogo

Essa adaptação é feita no código C++ do cheat (web_server.cpp). Se quiser, posso preparar a versão modificada do cheat que faz polling ao invés de servir o painel.

## Estrutura do projeto

```
render-cheat-panel/
├── package.json        # dependências (só express)
├── server.js           # servidor Node.js (auth + options + frontend)
├── public/
│   └── index.html      # frontend do painel (Lunar/Satella UI)
├── data/               # onde as opções são salvas (JSON)
└── README.md
```

## Endpoints

| Rota | Método | Descrição |
|------|--------|-----------|
| `/` | GET | Frontend do painel |
| `/api/auth/key` | POST | Valida key (proxy pra satella API) |
| `/api/auth/verify` | POST | Verifica se token ainda é válido |
| `/api/options` | GET | Retorna as opções atuais |
| `/api/set?path=...&value=...` | GET | Atualiza uma opção |
| `/api/poll` | GET | Usado pelo cheat pra pegar opções |
| `/api/bypass` | POST | Ativa bypass |
