// =============================================================================
// Route: /api/sensors
//
// POST /api/sensors/report  — ESP32 sends its current sensor readings here
// GET  /api/sensors          — Dashboard fetches initial state on load
//
// ESP32 request body example:
// {
//   "deviceId": "kitchen-001",
//   "sensors": [
//     { "id": 1, "status": "OK"    },
//     { "id": 2, "status": "EMPTY" }
//   ],
//   "rssi":   -65,
//   "uptime": 3721
// }
//
// Authentication:
//   POST /report → requires X-Api-Key header (or JWT cookie for browser testing)
//   GET  /       → requires JWT cookie (dashboard users only)
// =============================================================================

const { Router }      = require('express');
const sensorService   = require('../services/sensorService');
const requireAuth     = require('../middleware/requireAuth');
const requireApiKey   = require('../middleware/requireApiKey');

const router = Router();

// ---------------------------------------------------------------------------
// POST /api/sensors/report  —  Protected by API key (ESP32 devices)
// ---------------------------------------------------------------------------
router.post('/report', requireApiKey, (req, res) => {
  const { deviceId, sensors, rssi, uptime } = req.body;

  if (!deviceId || typeof deviceId !== 'string') {
    return res.status(400).json({ error: '"deviceId" is required and must be a string.' });
  }
  if (!Array.isArray(sensors) || sensors.length === 0) {
    return res.status(400).json({ error: '"sensors" must be a non-empty array.' });
  }

  const result = sensorService.processReport({ deviceId, sensors, rssi, uptime });

  res.json({
    ok:         true,
    alertCount: result.newAlerts.length,
    alerts:     result.newAlerts,
  });
});

// ---------------------------------------------------------------------------
// GET /api/sensors  —  Protected by JWT (dashboard users)
// ---------------------------------------------------------------------------
router.get('/', requireAuth, (_req, res) => {
  res.json(sensorService.getAllSensors());
});

// ---------------------------------------------------------------------------
// DELETE /api/sensors/:deviceId  —  Protected by JWT (dashboard users)
// ---------------------------------------------------------------------------
router.delete('/:deviceId', requireAuth, (req, res) => {
  const { deviceId } = req.params;
  if (!deviceId) return res.status(400).json({ error: 'deviceId required' });
  sensorService.deleteDevice(deviceId);
  res.json({ ok: true });
});

module.exports = router;
