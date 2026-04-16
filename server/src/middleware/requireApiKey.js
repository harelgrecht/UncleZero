// =============================================================================
// Middleware: requireApiKey
//
// Protects ESP32-facing routes (POST /api/sensors/report).
// Accepts two forms of authentication so developers can test easily:
//
//   1. X-Api-Key header  — used by ESP32 firmware (primary)
//   2. JWT cookie        — used by developers testing from a browser
//
// To add the header in your ESP32 firmware:
//   client.addHeader("X-Api-Key", "your-key-here");
// =============================================================================

const authService = require('../services/authService');

function requireApiKey(req, res, next) {
  // ── Path 1: API key (ESP32 devices) ──────────────────────────────────────
  const apiKey = req.headers['x-api-key'];
  if (apiKey) {
    if (authService.verifyApiKey(apiKey)) return next();
    return res.status(401).json({ error: 'Invalid API key.' });
  }

  // ── Path 2: JWT cookie (browser / developer testing) ─────────────────────
  const token = req.cookies?.token;
  if (token) {
    try {
      req.user = authService.verifyJwt(token);
      return next();
    } catch {
      // Invalid JWT — fall through to the 401 below
    }
  }

  res.status(401).json({
    error: 'Unauthorized. Provide an X-Api-Key header (for devices) or log in first (for browsers).',
  });
}

module.exports = requireApiKey;
