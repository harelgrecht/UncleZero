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
// Security note:
//   For production deployments add an API key check before processReport().
//   Example: require('../../middleware/apiKeyAuth')(req, res, next)
// =============================================================================

const { Router }     = require('express');
const sensorService  = require('../services/sensorService');

const router = Router();

// ---------------------------------------------------------------------------
// POST /api/sensors/report
// ---------------------------------------------------------------------------
router.post('/report', (req, res) => {
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
// GET /api/sensors
// ---------------------------------------------------------------------------
router.get('/', (_req, res) => {
  res.json(sensorService.getAllSensors());
});

module.exports = router;
