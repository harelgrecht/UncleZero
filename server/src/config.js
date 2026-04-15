// =============================================================================
// Server Configuration
//
// All tunable values live here. Override any setting with environment variables
// so the same codebase runs in dev, staging, and production without code changes.
//
// Example:
//   PORT=8080 DB_PATH=/var/data/sensors.db node src/server.js
// =============================================================================

module.exports = {
  server: {
    port: process.env.PORT || 3000,
    host: process.env.HOST || '0.0.0.0',
  },

  database: {
    // Path is relative to the project root (server/)
    path: process.env.DB_PATH || 'data/sensors.db',
    // Prune alert records older than this many milliseconds (default: 30 days)
    maxAgeMs: parseInt(process.env.ALERT_MAX_AGE_MS) || 30 * 24 * 60 * 60 * 1000,
  },

  sensors: {
    // Mark a sensor OFFLINE if it has not reported within this window.
    // Default is 25 hours — suitable for once-a-day reporting with some drift tolerance.
    // For more frequent reporting, lower this value (e.g. 60_000 for every-minute reports).
    offlineThresholdMs: parseInt(process.env.OFFLINE_THRESHOLD_MS) || 25 * 60 * 60 * 1000, // 25 hours
  },
};
