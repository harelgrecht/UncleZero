// =============================================================================
// Route: /api/alerts
//
// GET /api/alerts?limit=50  — Returns recent alerts, newest first
// =============================================================================

const { Router }    = require('express');
const sensorService = require('../services/sensorService');

const router = Router();

// ---------------------------------------------------------------------------
// GET /api/alerts
// ---------------------------------------------------------------------------
router.get('/', (req, res) => {
  const limit = Math.min(parseInt(req.query.limit) || 50, 500);
  res.json(sensorService.getRecentAlerts(limit));
});

module.exports = router;
