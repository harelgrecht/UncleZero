// =============================================================================
// Sensor Service  —  Core Business Logic
//
// This is the single place where sensor data is processed, persisted, and
// broadcast. Keep integrations with external systems here:
//
//   • WhatsApp / SMS alerts  → call your notifier inside processReport()
//                              after inserting into the alerts table
//   • Technician dispatch    → register a listener with onSensorEvent()
//   • Push notifications     → same pattern as above
//
// The onSensorEvent() listener pattern keeps this service decoupled from
// Socket.io, Twilio, PagerDuty, etc. — each integration registers itself
// and reacts independently.
// =============================================================================

const db     = require('../database');
const config = require('../config');

// ---------------------------------------------------------------------------
// Event Listeners
// External systems (Socket.io, SMS, etc.) register here to receive events.
// ---------------------------------------------------------------------------

/** @type {Array<(event: SensorEvent) => void>} */
const listeners = [];

/**
 * Register a callback that fires on every sensor state change.
 * @param {(event: SensorEvent) => void} callback
 */
function onSensorEvent(callback) {
  listeners.push(callback);
}

function emitEvent(event) {
  listeners.forEach(fn => fn(event));
}

// ---------------------------------------------------------------------------
// Prepared Statements
// Defined once at startup for performance (SQLite compiles the query plan).
// ---------------------------------------------------------------------------

const stmts = {
  getSensor: db.prepare('SELECT status FROM sensors WHERE id = ?'),

  upsertSensor: db.prepare(`
    INSERT INTO sensors (id, device_id, sensor_num, status, rssi, last_seen)
    VALUES (@id, @deviceId, @sensorNum, @status, @rssi, @lastSeen)
    ON CONFLICT(id) DO UPDATE SET
      status    = excluded.status,
      rssi      = excluded.rssi,
      last_seen = excluded.last_seen
  `),

  insertAlert: db.prepare(`
    INSERT INTO alerts (sensor_id, device_id, sensor_num, event, timestamp)
    VALUES (@sensorId, @deviceId, @sensorNum, @event, @timestamp)
  `),

  getAllSensors: db.prepare(`
    SELECT * FROM sensors ORDER BY device_id, sensor_num
  `),

  getAlerts: db.prepare(`
    SELECT * FROM alerts ORDER BY timestamp DESC LIMIT ?
  `),

  pruneAlerts: db.prepare(`
    DELETE FROM alerts WHERE timestamp < ?
  `),
};

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/**
 * Process a sensor report from an ESP32 device.
 *
 * Persists the new sensor state, records an alert when the status changes,
 * then notifies all registered listeners with a single aggregated event.
 *
 * @param {object}   report
 * @param {string}   report.deviceId  - Unique device identifier (e.g. "kitchen-001")
 * @param {object[]} report.sensors   - Array of { id: number, status: "OK"|"EMPTY" }
 * @param {number}   [report.rssi]    - WiFi RSSI in dBm
 * @param {number}   [report.uptime]  - Device uptime in seconds
 *
 * @returns {{ updatedSensors: object[], newAlerts: object[] }}
 */
function processReport({ deviceId, sensors, rssi, uptime }) {
  const now           = Date.now();
  const updatedSensors = [];
  const newAlerts      = [];

  // Wrap all DB writes in a transaction so they succeed or fail together
  const transaction = db.transaction(() => {
    for (const sensor of sensors) {
      const sensorId      = `${deviceId}:sensor-${sensor.id}`;
      const previousRow   = stmts.getSensor.get(sensorId);
      const previousStatus = previousRow?.status;
      const newStatus      = sensor.status; // "OK" or "EMPTY"

      stmts.upsertSensor.run({
        id:        sensorId,
        deviceId,
        sensorNum: sensor.id,
        status:    newStatus,
        rssi:      rssi ?? null,
        lastSeen:  now,
      });

      updatedSensors.push({ sensorId, sensorNum: sensor.id, status: newStatus });

      // Only record an alert when the status actually changes (suppress duplicates)
      if (previousStatus !== newStatus) {
        const event = newStatus === 'EMPTY' ? 'TANK_EMPTY' : 'TANK_RESTORED';

        stmts.insertAlert.run({
          sensorId,
          deviceId,
          sensorNum: sensor.id,
          event,
          timestamp: now,
        });

        newAlerts.push({ sensorId, deviceId, sensorNum: sensor.id, event, timestamp: now });
      }
    }
  });

  transaction();

  // Broadcast one event per report (not one per sensor) to avoid UI flicker
  emitEvent({
    type:    'SENSOR_UPDATE',
    deviceId,
    rssi:    rssi ?? null,
    uptime:  uptime ?? null,
    sensors: updatedSensors,
    alerts:  newAlerts,
    timestamp: now,
  });

  return { updatedSensors, newAlerts };
}

/**
 * Return the current state of all known sensors.
 * @returns {object[]}
 */
function getAllSensors() {
  return stmts.getAllSensors.all();
}

/**
 * Return recent alerts, newest first.
 * @param {number} [limit=50]
 * @returns {object[]}
 */
function getRecentAlerts(limit = 50) {
  return stmts.getAlerts.all(limit);
}

/**
 * Remove alerts older than the configured max age.
 * Call this on a schedule (e.g. daily) to keep the DB small.
 */
function pruneOldAlerts() {
  const cutoff = Date.now() - config.database.maxAgeMs;
  const result = stmts.pruneAlerts.run(cutoff);
  return result.changes; // number of rows deleted
}

module.exports = { processReport, getAllSensors, getRecentAlerts, pruneOldAlerts, onSensorEvent };
