#ifndef WEBFILES_H
#define WEBFILES_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>BEAM BANDIT V2.0</title>
    <style>
        :root {
            --bg-color: #0f1115;
            --panel-color: #16181f;
            --accent-color: #ff5555;
            --success-color: #50fa7b;
            --text-color: #f8f8f2;
            --console-bg: #1e1f29;
            --border-color: #2a2d37;
        }
        * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-color);
            margin: 0; padding: 12px;
            font-size: 14px;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        header {
            display: flex; justify-content: space-between; align-items: flex-start;
            border-bottom: 2px dashed #444; padding-bottom: 12px; margin-bottom: 15px;
        }
        .brand-block h1 { margin: 0; color: #ffb86c; font-size: 24px; font-weight: 700; letter-spacing: 1.5px; }
        .brand-block .sub1 { margin: 4px 0 2px 0; font-size: 14px; text-transform: uppercase; color: #8be9fd; letter-spacing: 1px; font-weight: 600; }
        .brand-block .sub2 { margin: 0; font-size: 13px; color: var(--accent-color); font-family: monospace; font-weight: bold; }
        
        .grid-layout { display: grid; grid-template-columns: 1fr; gap: 15px; }
        @media(min-width: 900px) { .grid-layout { grid-template-columns: 1fr 1fr 1.2fr; } }
        
        .panel {
            background-color: var(--panel-color);
            border: 1px solid var(--border-color); padding: 15px; border-radius: 6px;
            display: flex; flex-direction: column;
        }
        .panel h2 { margin: 0 0 12px 0; font-size: 13px; text-transform: uppercase; color: #ffb86c; letter-spacing: 0.5px; }
        
        button {
            color: #000; border: none;
            padding: 14px; font-weight: bold; font-size: 14px; font-family: inherit;
            cursor: pointer; width: 100%; margin-bottom: 12px; border-radius: 4px;
            transition: transform 0.1s, opacity 0.2s; text-transform: uppercase;
        }
        button:last-child { margin-bottom: 0; }
        button:active { transform: scale(0.98); }
        button.danger { background-color: var(--accent-color); color: #fff; }
        button.success { background-color: var(--success-color); color: #000; }
        
        textarea {
            width: 100%; background-color: var(--console-bg); color: #f1fa8c;
            border: 1px solid var(--border-color); padding: 12px; border-radius: 4px;
            font-family: "Courier New", Courier, monospace; font-size: 13px; margin-bottom: 12px;
            outline: none; height: 110px; resize: none;
        }
        
        .history-list {
            background-color: var(--console-bg); border: 1px solid var(--border-color);
            border-radius: 4px; height: 235px; overflow-y: auto; padding: 5px;
        }
        .history-item {
            display: flex; justify-content: space-between; align-items: center;
            padding: 10px; margin-bottom: 5px; background: rgba(255,255,255,0.03);
            border-radius: 3px; cursor: pointer; border: 1px solid transparent;
            transition: background 0.2s;
        }
        .history-item:hover { background: rgba(255,255,255,0.08); border-color: #6272a4; }
        .history-item .info { font-size: 12px; font-family: monospace; }
        .history-item .action-text { font-size: 11px; color: #8be9fd; text-transform: uppercase; font-weight: bold; }

        .console {
            background-color: var(--console-bg); border-left: 3px solid #ffb86c;
            padding: 12px; font-family: "Courier New", Courier, monospace; font-size: 12px;
            height: 110px; overflow-y: auto; border-radius: 4px; margin-top: 15px;
            line-height: 1.5; color: #a9b2c3;
        }
        
        .status-badge { padding: 5px 10px; font-size: 11px; font-weight: bold; border-radius: 3px; background: #222; }
        .state-IDLE { color: var(--success-color); border: 1px solid var(--success-color); }
        .state-CAPTURING { color: var(--accent-color); border: 1px solid var(--accent-color); animation: flash 1s infinite; }
        .state-SENDING { color: #8be9fd; border: 1px solid #8be9fd; }
        @keyframes flash { 50% { opacity: 0.3; } }
    </style>
</head>
<body>

<div class="container">
    <header>
        <div class="brand-block">
            <h1>BEAM BANDIT V2.0</h1>
            <div class="sub1">operation: dark pulse</div>
            <div class="sub2">By Gh05tJr</div>
        </div>
        <div><span id="sysState" class="status-badge state-IDLE">IDLE</span></div>
    </header>

    <div class="grid-layout">
        <div class="panel">
            <h2>Control Deck</h2>
            <button class="danger" id="captureBtn" onclick="triggerCapture()">Capture</button>
            <h2 style="margin-top: 8px; margin-bottom: 8px;">Live Capture Register</h2>
            <textarea id="rawPayload" placeholder="Awaiting live signal sync... Arriving sequences populate here automatically."></textarea>
            <button class="success" onclick="sendLiveHardwareBuffer()">Send</button>
        </div>

        <div class="panel">
            <h2>Manual Injection Workspace</h2>
            <p style="margin: 0 0 10px 0; color: #6272a4; font-size: 12px;">Paste raw space-separated timing arrays below:</p>
            <textarea id="pastePayload" placeholder="Example format: [9000, 4500, 560, 560...]"></textarea>
            <button class="success" onclick="sendCustomPayload('pastePayload')">Send</button>
        </div>

        <div class="panel">
            <h2>Captured Data Records</h2>
            <div class="history-list" id="historyLog">
                <div style="color:#6272a4; text-align:center; padding:40px; font-size:12px;" id="emptyHint">No sequences saved in session memory</div>
            </div>
        </div>
    </div>

    <h2>Active System Logs</h2>
    <div class="console" id="terminal">--- UI Engine Ready ---<br></div>
</div>

<script>
    const terminal = document.getElementById('terminal');
    const stateBadge = document.getElementById('sysState');
    const payloadBox = document.getElementById('rawPayload');
    const pasteBox = document.getElementById('pastePayload');
    const historyLog = document.getElementById('historyLog');
    const captureBtn = document.getElementById('captureBtn');

    let sequenceCount = 0;

    function log(msg, type="SYS") {
        const time = new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
        terminal.innerHTML += `[${time}] [${type}] ${msg}<br>`;
        terminal.scrollTop = terminal.scrollHeight;
    }

    function appendToHistory(signalArray) {
        if (!signalArray || !signalArray.length) return;
        const hint = document.getElementById('emptyHint');
        if (hint) hint.remove();
        
        sequenceCount++;
        const currentNum = sequenceCount;
        
        const item = document.createElement('div');
        item.className = 'history-item';
        
        let standardStr = JSON.stringify(signalArray).replace(/[\r\n\s]+/g, '');
        item.setAttribute('data-raw', standardStr);
        
        item.innerHTML = `
            <div class="info">Record #${currentNum} [Size: ${signalArray.length}]</div>
            <div class="action-text">Load</div>
        `;
        
        item.onclick = function() {
            let directData = this.getAttribute('data-raw');
            pasteBox.value = directData;
            log(`Loaded Record #${currentNum} into Manual Workspace window.`, "UI");
        };
        
        historyLog.insertBefore(item, historyLog.firstChild);
    }

    async function checkSystemLoop() {
        try {
            let res = await fetch('/api/status');
            if (!res.ok) return;
            let data = await res.json();
            
            if (data && data.state) {
                stateBadge.innerText = data.state;
                stateBadge.className = `status-badge state-${data.state}`;

                if (data.state === "CAPTURING") {
                    captureBtn.innerText = "Cancel Capture";
                } else {
                    captureBtn.innerText = "Capture";
                }
            }

            if (data && data.new_data && data.signal && data.signal.length > 0) {
                let cleanString = JSON.stringify(data.signal).replace(/[\r\n\s]+/g, '');
                payloadBox.value = cleanString;
                log(`New timing metrics cached globally (${data.signal.length} values).`, "IR");
                appendToHistory(data.signal);
            }
        } catch (e) {}
    }

    async function triggerCapture() {
        try {
            log("Broadcasting capture initialization frame down to hardware target...", "WEB");
            let res = await fetch('/api/capture', { method: 'POST' });
            if (res.ok) {
                let responseText = await res.text();
                log(responseText, "SYS");
            } else {
                log("Hardware refused request: System busy.", "ERR");
            }
        } catch(e) {
            log("Network Error: Failed to contact D1 Mini.", "ERR");
        }
    }

    async function sendLiveHardwareBuffer() {
        try {
            log("Firing active hardware data register directly...", "TX");
            let res = await fetch('/api/fire_live', { method: 'POST' });
            if(res.ok) {
                log("Transmission complete.", "TX");
            } else {
                log("Transmission failed: Active register memory empty.", "ERR");
            }
        } catch(e) {
            log("Network communication breakdown.", "ERR");
        }
    }

    async function sendCustomPayload(sourceId) {
        try {
            const targetBox = document.getElementById(sourceId);
            if (!targetBox) return;
            
            let val = targetBox.value.trim().replace(/[\r\n\s]+/g, '');
            if(!val || val === "[]") return log(`Error: Target workspace area is empty.`, "ERR");
            
            if (!val.startsWith('[')) val = '[' + val;
            if (!val.endsWith(']')) val = val + ']';

            log("Streaming manual text instructions to emitter register...", "TX");
            
            let formData = new URLSearchParams();
            formData.append('data', val);

            let res = await fetch('/api/send', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: formData.toString()
            });
            
            if(res.ok) {
                log("Transmission complete.", "TX");
            } else {
                log("Transmission failed: Device rejected array metrics.", "ERR");
            }
        } catch(e) {
            log("Validation failure: Verify data follows format [x, y, z].", "ERR");
        }
    }

    setInterval(checkSystemLoop, 1000);
</script>
</body>
</html>
)rawhtml";

#endif