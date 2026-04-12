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
// Utilities
// ---------------------------------------------------------------------------

/**
 * Format a Unix timestamp (ms) as a readable time string.
 * If the event was today, shows only HH:MM:SS. Otherwise shows the date too.
 */
function formatTime(timestampMs) {
  const date = new Date(timestampMs);
  const now  = new Date();
  const isToday = date.toDateString() === now.toDateString();

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
  if (level >= 4) return '';
  if (level === 3) return '';
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
// Stats Bar
// ---------------------------------------------------------------------------

function updateStats() {
  const allSensors = [...sensors.values()];
  const deviceCount = devices.size;
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

/**
 * Build the HTML for a single sensor row inside a device card.
 * @param {object} sensor
 */
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

/**
 * Build the full HTML for a device card.
 * @param {string}   deviceId
 * @param {object[]} deviceSensors  — all sensors belonging to this device
 */
function deviceCardHtml(deviceId, deviceSensors) {
  const hasAlert  = deviceSensors.some(s => s.status === 'EMPTY');
  const latestTs  = Math.max(...deviceSensors.map(s => s.last_seen));
  const rssi      = deviceSensors[0]?.rssi;

  return `
    <div class="device-card ${hasAlert ? 'has-alert' : ''}" id="device-${deviceId.replace(/[^a-z0-9]/gi, '-')}">
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
        </div>
      </div>
      <div class="device-sensors">
        ${deviceSensors.map(sensorRowHtml).join('')}
      </div>
    </div>
  `;
}

/** Re-render the entire device grid from the current state maps. */
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

/**
 * Apply a partial update to a device card that already exists in the DOM,
 * avoiding a full grid re-render (reduces flicker on rapid updates).
 * @param {string}   deviceId
 * @param {object[]} updatedSensors  — sensors that changed in this report
 * @param {number}   rssi
 * @param {number}   timestamp
 */
function patchDeviceCard(deviceId, updatedSensors, rssi, timestamp) {
  const cardId = `device-${deviceId.replace(/[^a-z0-9]/gi, '-')}`;
  const card   = document.getElementById(cardId);
  if (!card) { renderDeviceGrid(); return; }

  // Update each changed sensor row
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

  // Update card-level alert highlight
  const hasAlert = [...devices.get(deviceId) || []].some(s => s.status === 'EMPTY');
  card.classList.toggle('has-alert', hasAlert);

  // Update RSSI and last-seen
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

/** Prepend a new alert item to the top of the feed. */
function prependAlert(alert, animate = false) {
  const feed    = document.getElementById('alert-feed');
  const noAlerts = feed.querySelector('.no-alerts');
  if (noAlerts) noAlerts.remove();

  // Remove oldest if we're over the cap
  while (feed.children.length >= MAX_FEED_ITEMS) {
    feed.removeChild(feed.lastChild);
  }

  const item = document.createElement('div');
  item.className = `alert-item ${alert.event}${animate ? ' new-alert' : ''}`;
  item.innerHTML = `
    <div class="alert-body">
      <div class="alert-event">${eventLabel(alert.event)} — Sensor ${alert.sensor_num}</div>
      <div class="alert-detail">${alert.device_id}</div>
    </div>
    <div class="alert-time">${formatTime(alert.timestamp)}</div>
  `;

  feed.insertBefore(item, feed.firstChild);
}

/** Populate the alert feed from the initial API response (no animation). */
function populateAlertFeed(alerts) {
  const feed = document.getElementById('alert-feed');
  if (!alerts.length) return;

  feed.innerHTML = '';
  // API returns newest-first; render them in that order
  alerts.forEach(a => prependAlert(a, false));

  // After initial load, flip so newest is on top and re-insert in order
  feed.innerHTML = '';
  [...alerts].reverse().forEach(a => prependAlert(a, false));
}

// ---------------------------------------------------------------------------
// State Synchronisation
// Apply a sensor update from either the initial API call or a socket event.
// ---------------------------------------------------------------------------

/**
 * Merge incoming sensor data into our state maps.
 * @param {object[]} sensorList  — array of sensor objects (from API or socket event)
 */
function mergeSensors(sensorList) {
  for (const sensor of sensorList) {
    sensors.set(sensor.id, sensor);

    // Keep the device → sensors[] index up to date
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
  const socket = io();
  const badge  = document.getElementById('connection-badge');
  const label  = document.getElementById('connection-label');

  socket.on('connect', () => {
    badge.className = 'connection-badge connected';
    label.textContent = 'Live';
  });

  socket.on('disconnect', () => {
    badge.className = 'connection-badge disconnected';
    label.textContent = 'Reconnecting...';
  });

  // Fired by sensorService whenever an ESP32 sends a report
  socket.on('sensor:update', (event) => {
    // Rebuild full sensor objects from partial update + existing state
    for (const updated of event.sensors) {
      const existing = sensors.get(updated.sensorId) || {};
      sensors.set(updated.sensorId, {
        ...existing,
        id:        updated.sensorId,
        device_id: event.deviceId,
        sensor_num: updated.sensorNum,
        status:    updated.status,
        rssi:      event.rssi,
        last_seen: event.timestamp,
      });

      // Update device index
      const deviceSensors = devices.get(event.deviceId) || [];
      const idx = deviceSensors.findIndex(s => s.id === updated.sensorId);
      const full = sensors.get(updated.sensorId);
      if (idx >= 0) deviceSensors[idx] = full;
      else          deviceSensors.push(full);
      deviceSensors.sort((a, b) => a.sensor_num - b.sensor_num);
      devices.set(event.deviceId, deviceSensors);
    }

    patchDeviceCard(event.deviceId, event.sensors, event.rssi, event.timestamp);
    updateStats();

    // Add any new alerts to the top of the feed
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
    const [sensorData, alertData] = await Promise.all([
      fetch('/api/sensors').then(r => r.json()),
      fetch('/api/alerts?limit=50').then(r => r.json()),
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

    // Refresh "X ago" labels without touching the rest of the DOM
    document.querySelectorAll('[data-ts]').forEach(el => {
      el.textContent = timeAgo(parseInt(el.dataset.ts, 10));
    });
  }
  tick();
  setInterval(tick, 10_000);
}

// ---------------------------------------------------------------------------
// Boot
// ---------------------------------------------------------------------------

loadInitialState();
connectSocket();
startClock();
