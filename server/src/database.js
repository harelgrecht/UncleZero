// =============================================================================
// Database Setup
//
// Uses SQLite via better-sqlite3 (synchronous, zero-config, file-based).
// The schema is created on first run; no migrations needed for new installs.
//
// Tables:
//   sensors  — Current state snapshot of every known sensor
//   alerts   — Immutable log of every status-change event
// =============================================================================

const path     = require('path');
const fs       = require('fs');
const Database = require('better-sqlite3');
const config   = require('./config');

// Resolve DB path relative to project root (the server/ directory)
const dbPath = path.resolve(__dirname, '..', config.database.path);

// Ensure the data directory exists before opening the database
fs.mkdirSync(path.dirname(dbPath), { recursive: true });

const db = new Database(dbPath);

// WAL mode gives better read concurrency (multiple dashboard clients + 1 writer)
db.pragma('journal_mode = WAL');

// ---------------------------------------------------------------------------
// Schema
// ---------------------------------------------------------------------------

db.exec(`
  -- Current state of each individual sensor (one row per sensor, upserted on every report)
  CREATE TABLE IF NOT EXISTS sensors (
    id          TEXT    PRIMARY KEY,  -- "<deviceId>:sensor-<num>", e.g. "kitchen-001:sensor-1"
    device_id   TEXT    NOT NULL,     -- The ESP32 that owns this sensor
    sensor_num  INTEGER NOT NULL,     -- 1 or 2
    status      TEXT    NOT NULL,     -- "OK" | "EMPTY" | "OFFLINE"
    rssi        INTEGER,              -- WiFi signal strength in dBm (from the parent device)
    last_seen   INTEGER NOT NULL      -- Unix timestamp in milliseconds
  );

  -- Append-only log of status changes (never updated, only inserted)
  CREATE TABLE IF NOT EXISTS alerts (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    sensor_id   TEXT    NOT NULL,
    device_id   TEXT    NOT NULL,
    sensor_num  INTEGER NOT NULL,
    event       TEXT    NOT NULL,     -- "TANK_EMPTY" | "TANK_RESTORED"
    timestamp   INTEGER NOT NULL      -- Unix timestamp in milliseconds
  );

  -- Speed up the dashboard's "latest alerts" query
  CREATE INDEX IF NOT EXISTS idx_alerts_timestamp ON alerts (timestamp DESC);
`);

module.exports = db;
