// =============================================================================
// Dashboard — Real-time Client Logic
//
// Responsibilities:
//   1. Fetch initial sensor + alert state from the REST API
//   2. Connect to Socket.io and apply live updates as they arrive
//   3. Render/update the DOM (device cards, alert feed, stats bar)
//
// This file is intentionally framework-free (vanilla JS) so any developer
// can read, modify, or replace it without build tooling.
// =============================================================================

// ---------------------------------------------------------------------------
// State
// In-memory maps so we can update individual cards without full re-renders.
// ---------------------------------------------------------------------------

/** @type {Map<string, object>}  sensorId → sensor row object */
const sensors = new Map();

/** @type {Map<string, object[]>} deviceId → array of sensor objects for that device */
const devices = new Map();

// ---------------------------------------------------------------------------
// Auth Utilities
// ---------------------------------------------------------------------------

/**
 * Wrapper around fetch() that redirects to /login on a 401 response.
 * Use this for all API calls in this file.
 */
async function apiFetch(url, options = {}) {
  const res = await fetch(url, options);
  if (res.status === 401) {
    window.location.href = '/login';
    return null;
  }
  return res;
}

/** Sign out the current user and return to the login page. */
async function logout() {
  await fetch('/auth/logout', { method: 'POST' });
  window.location.href = '/login';
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

/**
 * Format a Unix timestamp (ms) as a readable time string.
 * If the event was today, shows only HH:MM:SS. Otherwise shows the date too.
 */
function formatTime(timestampMs) {
  const date    = new Date(timestampMs);
  const isToday = date.toDateString() === new Date().toDateString();

  if (isToday) {
    return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', second: '2-digit' });
  }
  return date.toLocaleDateString([], { month: 'short', day: 'numeric' })
    + ' ' + date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

/**
 * Returns a human-readable "time ago" string (e.g. "just now", "2 min ago").
 */
function timeAgo(timestampMs) {
  const seconds = Math.floor((Date.now() - timestampMs) / 1000);
  if (seconds < 10)  return 'just now';
  if (seconds < 60)  return `${seconds}s ago`;
  const minutes = Math.floor(seconds / 60);
  if (minutes < 60)  return `${minutes}m ago`;
  const hours = Math.floor(minutes / 60);
  return `${hours}h ago`;
}

/**
 * Convert RSSI (dBm) to a signal strength level 0–4.
 *   4 = excellent (> -50), 3 = good, 2 = fair, 1 = poor, 0 = no signal / null
 */
function rssiToLevel(rssi) {
  if (rssi == null) return 0;
  if (rssi > -50)  return 4;
  if (rssi > -60)  return 3;
  if (rssi > -70)  return 2;
  return 1;
}

function rssiQuality(rssi) {
  const level = rssiToLevel(rssi);
  if (level >= 3) return '';
  if (level === 2) return 'fair';
  return 'poor';
}

/** Build the 4-bar WiFi strength HTML snippet. */
function rssiHtml(rssi) {
  const level   = rssiToLevel(rssi);
  const quality = rssiQuality(rssi);
  const bars    = [1, 2, 3, 4].map(n => {
    const active = n <= level ? `active ${quality}` : '';
    return `<div class="rssi-bar ${active}"></div>`;
  }).join('');
  const label = rssi != null ? `${rssi} dBm` : 'no signal';
  return `<div class="rssi-bars" title="${label}">${bars}</div>`;
}

/** Map an event type to a friendly display string. */
function eventLabel(event) {
  return event === 'TANK_EMPTY' ? 'Tank Empty' : 'Tank Restored';
}

// ---------------------------------------------------------------------------
// Header: show current user + logout button
// ---------------------------------------------------------------------------

async function loadCurrentUser() {
  const res = await apiFetch('/auth/me');
  if (!res) return;

  const data = await res.json();
  if (data.username) {
    const headerRight = document.getElementById('header-right');
    const userEl = document.createElement('span');
    userEl.style.cssText = 'font-size:12px;color:var(--text-secondary)';
    userEl.textContent = data.username;

    const logoutBtn = document.createElement('button');
    logoutBtn.textContent = 'Sign out';
    logoutBtn.style.cssText = `
      font-size: 12px; padding: 4px 10px; border-radius: 6px; cursor: pointer;
      background: transparent; color: var(--text-secondary);
      border: 1px solid var(--border); transition: color 0.2s, border-color 0.2s;
    `;
    logoutBtn.onmouseover = () => { logoutBtn.style.color = 'var(--text-primary)'; logoutBtn.style.borderColor = 'var(--text-secondary)'; };
    logoutBtn.onmouseout  = () => { logoutBtn.style.color = 'var(--text-secondary)'; logoutBtn.style.borderColor = 'var(--border)'; };
    logoutBtn.addEventListener('click', logout);

    headerRight.prepend(logoutBtn);
    headerRight.prepend(userEl);
  }
}

// ---------------------------------------------------------------------------
// Stats Bar
// ---------------------------------------------------------------------------

function updateStats() {
  const allSensors   = [...sensors.values()];
  const deviceCount  = devices.size;
  const okCount      = allSensors.filter(s => s.status === 'OK').length;
  const emptyCount   = allSensors.filter(s => s.status === 'EMPTY').length;
  const offlineCount = allSensors.filter(s => s.status === 'OFFLINE').length;

  document.getElementById('stat-devices').textContent = deviceCount;
  document.getElementById('stat-ok').textContent      = okCount;
  document.getElementById('stat-empty').textContent   = emptyCount;
  document.getElementById('stat-offline').textContent = offlineCount;
}

// ---------------------------------------------------------------------------
// Device Card Rendering
// ---------------------------------------------------------------------------

function sensorRowHtml(sensor) {
  return `
    <div class="sensor-row status-${sensor.status}" id="sensor-${sensor.id.replace(/[^a-z0-9]/gi, '-')}">
      <div class="sensor-label">
        <strong>Sensor ${sensor.sensor_num}</strong>
        Gas Tank ${sensor.sensor_num}
      </div>
      <div class="sensor-status-badge ${sensor.status}">
        <div class="status-dot"></div>
        ${sensor.status}
      </div>
    </div>
  `;
}

function deviceCardHtml(deviceId, deviceSensors) {
  const hasAlert = deviceSensors.some(s => s.status === 'EMPTY');
  const latestTs = Math.max(...deviceSensors.map(s => s.last_seen));
  const rssi     = deviceSensors[0]?.rssi;
  const safeId   = deviceId.replace(/[^a-z0-9]/gi, '-');

  return `
    <div class="device-card ${hasAlert ? 'has-alert' : ''}" id="device-${safeId}">
      <div class="device-header">
        <div>
          <div class="device-name">
            ${deviceId.replace(/-/g, ' ').replace(/\b\w/g, c => c.toUpperCase())}
          </div>
          <div class="device-id">${deviceId}</div>
        </div>
        <div class="device-meta">
          ${rssiHtml(rssi)}
          <div class="last-seen" data-ts="${latestTs}">${timeAgo(latestTs)}</div>
          <button
            class="delete-device-btn"
            title="Delete device"
            onclick="deleteDevice('${deviceId.replace(/'/g, "\\'")}')"
            style="
              background:none;border:1px solid var(--border);color:var(--text-secondary);
              border-radius:6px;padding:3px 8px;font-size:11px;cursor:pointer;
              transition:color .2s,border-color .2s;margin-left:4px;
            "
            onmouseover="this.style.color='#c0392b';this.style.borderColor='#c0392b'"
            onmouseout="this.style.color='var(--text-secondary)';this.style.borderColor='var(--border)'"
          >&#x1F5D1;</button>
        </div>
      </div>
      <div class="device-sensors">
        ${deviceSensors.map(sensorRowHtml).join('')}
      </div>
    </div>
  `;
}

function renderDeviceGrid() {
  const grid = document.getElementById('device-grid');

  if (devices.size === 0) {
    grid.innerHTML = `
      <div class="empty-state">
        <div class="empty-icon">&#x1F4E1;</div>
        <p>Waiting for devices to connect...</p>
      </div>`;
    return;
  }

  grid.innerHTML = [...devices.entries()]
    .map(([deviceId, deviceSensors]) => deviceCardHtml(deviceId, deviceSensors))
    .join('');
}

function patchDeviceCard(deviceId, updatedSensors, rssi, timestamp) {
  const cardId = `device-${deviceId.replace(/[^a-z0-9]/gi, '-')}`;
  const card   = document.getElementById(cardId);
  if (!card) { renderDeviceGrid(); return; }

  for (const updated of updatedSensors) {
    const rowId  = `sensor-${updated.sensorId.replace(/[^a-z0-9]/gi, '-')}`;
    const row    = document.getElementById(rowId);
    const stored = sensors.get(updated.sensorId);
    if (!row || !stored) continue;

    row.className = `sensor-row status-${stored.status}`;
    const badge = row.querySelector('.sensor-status-badge');
    if (badge) {
      badge.className = `sensor-status-badge ${stored.status}`;
      badge.innerHTML = `<div class="status-dot"></div>${stored.status}`;
    }
  }

  const hasAlert = [...(devices.get(deviceId) || [])].some(s => s.status === 'EMPTY');
  card.classList.toggle('has-alert', hasAlert);

  const metaEl = card.querySelector('.device-meta');
  if (metaEl) {
    metaEl.innerHTML = `
      ${rssiHtml(rssi)}
      <div class="last-seen" data-ts="${timestamp}">${timeAgo(timestamp)}</div>
    `;
  }
}

// ---------------------------------------------------------------------------
// Alert Feed
// ---------------------------------------------------------------------------

const MAX_FEED_ITEMS = 100;

function prependAlert(alert, animate = false) {
  const feed     = document.getElementById('alert-feed');
  const noAlerts = feed.querySelector('.no-alerts');
  if (noAlerts) noAlerts.remove();

  while (feed.children.length >= MAX_FEED_ITEMS) {
    feed.removeChild(feed.lastChild);
  }

  const item = document.createElement('div');
  item.className = `alert-item ${alert.event}${animate ? ' new-alert' : ''}`;
  const sensorNum  = alert.sensor_num  ?? alert.sensorNum;
  const deviceId   = alert.device_id   ?? alert.deviceId;
  const deviceName = (deviceId || '').replace(/[_-]/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
  item.innerHTML = `
    <div class="alert-body">
      <div class="alert-event">${eventLabel(alert.event)} — Sensor ${sensorNum}</div>
      <div class="alert-detail">${deviceName}</div>
    </div>
    <div class="alert-time">${formatTime(alert.timestamp)}</div>
  `;

  feed.insertBefore(item, feed.firstChild);
}

function populateAlertFeed(alerts) {
  if (!alerts.length) return;
  document.getElementById('alert-feed').innerHTML = '';
  [...alerts].reverse().forEach(a => prependAlert(a, false));
}

// ---------------------------------------------------------------------------
// State Synchronisation
// ---------------------------------------------------------------------------

function mergeSensors(sensorList) {
  for (const sensor of sensorList) {
    sensors.set(sensor.id, sensor);

    const deviceSensors = devices.get(sensor.device_id) || [];
    const existing = deviceSensors.findIndex(s => s.id === sensor.id);
    if (existing >= 0) {
      deviceSensors[existing] = sensor;
    } else {
      deviceSensors.push(sensor);
      deviceSensors.sort((a, b) => a.sensor_num - b.sensor_num);
    }
    devices.set(sensor.device_id, deviceSensors);
  }
}

// ---------------------------------------------------------------------------
// Socket.io Connection
// ---------------------------------------------------------------------------

function connectSocket() {
  // The JWT cookie is sent automatically with the WebSocket upgrade request —
  // no need to pass it manually here.
  const socket = io();
  const badge  = document.getElementById('connection-badge');
  const label  = document.getElementById('connection-label');

  socket.on('connect', () => {
    badge.className   = 'connection-badge connected';
    label.textContent = 'Live';
  });

  socket.on('disconnect', () => {
    badge.className   = 'connection-badge disconnected';
    label.textContent = 'Reconnecting...';
  });

  // Redirect to login if the server rejects the socket (expired/invalid JWT)
  socket.on('connect_error', (err) => {
    if (err.message.includes('Authentication') || err.message.includes('Session')) {
      window.location.href = '/login';
    }
  });

  socket.on('sensor:update', (event) => {
    for (const updated of event.sensors) {
      const existing = sensors.get(updated.sensorId) || {};
      sensors.set(updated.sensorId, {
        ...existing,
        id:         updated.sensorId,
        device_id:  event.deviceId,
        sensor_num: updated.sensorNum,
        status:     updated.status,
        rssi:       event.rssi,
        last_seen:  event.timestamp,
      });

      const deviceSensors = devices.get(event.deviceId) || [];
      const idx  = deviceSensors.findIndex(s => s.id === updated.sensorId);
      const full = sensors.get(updated.sensorId);
      if (idx >= 0) deviceSensors[idx] = full;
      else          deviceSensors.push(full);
      deviceSensors.sort((a, b) => a.sensor_num - b.sensor_num);
      devices.set(event.deviceId, deviceSensors);
    }

    patchDeviceCard(event.deviceId, event.sensors, event.rssi, event.timestamp);
    updateStats();

    for (const alert of event.alerts) {
      prependAlert(alert, true);
    }
  });
}

// ---------------------------------------------------------------------------
// Initial Data Load
// ---------------------------------------------------------------------------

async function loadInitialState() {
  try {
    const [sensorsRes, alertsRes] = await Promise.all([
      apiFetch('/api/sensors'),
      apiFetch('/api/alerts?limit=50'),
    ]);

    // apiFetch returns null and redirects on 401
    if (!sensorsRes || !alertsRes) return;

    const [sensorData, alertData] = await Promise.all([
      sensorsRes.json(),
      alertsRes.json(),
    ]);

    mergeSensors(sensorData);
    renderDeviceGrid();
    populateAlertFeed(alertData);
    updateStats();
  } catch (err) {
    console.error('Failed to load initial state:', err);
  }
}

// ---------------------------------------------------------------------------
// Clock & Relative Timestamps
// ---------------------------------------------------------------------------

function startClock() {
  function tick() {
    document.getElementById('clock').textContent = new Date().toLocaleTimeString();
    document.querySelectorAll('[data-ts]').forEach(el => {
      el.textContent = timeAgo(parseInt(el.dataset.ts, 10));
    });
  }
  tick();
  setInterval(tick, 10_000);
}

// ---------------------------------------------------------------------------
// Device Deletion
// ---------------------------------------------------------------------------

async function deleteDevice(deviceId) {
  const name = deviceId.replace(/[_-]/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
  if (!confirm(`Delete all data for "${name}"?\n\nThis removes the device card and its alert history.`)) return;

  const res = await apiFetch(`/api/sensors/${encodeURIComponent(deviceId)}`, { method: 'DELETE' });
  if (!res || !res.ok) return;

  for (const sensor of (devices.get(deviceId) || [])) {
    sensors.delete(sensor.id);
  }
  devices.delete(deviceId);
  renderDeviceGrid();
  updateStats();
}

// ---------------------------------------------------------------------------
// Boot
// ---------------------------------------------------------------------------

loadCurrentUser();
loadInitialState();
connectSocket();
startClock();
