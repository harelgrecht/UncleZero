// =============================================================================
// Server Configuration
//
// All tunable values live here. Override any setting with environment variables
// so the same codebase runs in dev, staging, and production without code changes.
//
// Recommended .env for production:
//   PORT=3000
//   JWT_SECRET=<long random string>
//   ADMIN_USERNAME=admin
//   ADMIN_PASSWORD=<strong password>
//   ESP32_API_KEY=<long random string>
//   NODE_ENV=production
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

  auth: {
    // JWT_SECRET must be a long, random, secret string in production.
    // If not set, the server warns loudly and uses an insecure default.
    jwtSecret: process.env.JWT_SECRET || (() => {
      if (process.env.NODE_ENV === 'production') {
        console.error('[AUTH] FATAL: JWT_SECRET environment variable is not set. Refusing to start.');
        process.exit(1);
      }
      console.warn('[AUTH] WARNING: JWT_SECRET is not set. Using an insecure default — do not use in production!');
      return 'uncle-zero-insecure-dev-secret';
    })(),

    // How long a login session lasts (default: 8 hours)
    jwtExpirySeconds: parseInt(process.env.JWT_EXPIRY_SECONDS) || 8 * 60 * 60,

    // Default admin account created on first run if no users exist.
    // If ADMIN_PASSWORD is not set, a random password is generated and shown in the console.
    adminUsername: process.env.ADMIN_USERNAME || 'admin',
    adminPassword: process.env.ADMIN_PASSWORD || null,

    // API key for ESP32 devices. If not set, a random key is generated and shown in the console.
    esp32ApiKey: process.env.ESP32_API_KEY || null,
  },
};
