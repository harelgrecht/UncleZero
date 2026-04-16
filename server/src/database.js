// =============================================================================
// Database Setup
//
// Uses SQLite via better-sqlite3 (synchronous, zero-config, file-based).
// The schema is created on first run; no migrations needed for new installs.
//
// Tables:
//   sensors   — Current state snapshot of every known sensor
//   alerts    — Immutable log of every status-change event
//   users     — Dashboard user accounts (hashed passwords)
//   api_keys  — ESP32 device API keys (SHA-256 hashed)
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

  -- Dashboard user accounts
  CREATE TABLE IF NOT EXISTS users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT    NOT NULL UNIQUE,
    password_hash TEXT    NOT NULL,   -- bcrypt hash
    created_at    INTEGER NOT NULL    -- Unix timestamp in milliseconds
  );

  -- ESP32 device API keys (one row per registered device)
  CREATE TABLE IF NOT EXISTS api_keys (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    label      TEXT    NOT NULL,      -- human-readable name, e.g. "kitchen-esp32"
    key_hash   TEXT    NOT NULL,      -- SHA-256 hex digest of the raw key
    created_at INTEGER NOT NULL       -- Unix timestamp in milliseconds
  );

  -- Speed up the dashboard's "latest alerts" query
  CREATE INDEX IF NOT EXISTS idx_alerts_timestamp ON alerts (timestamp DESC);
`);

module.exports = db;
