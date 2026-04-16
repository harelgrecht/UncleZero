// =============================================================================
// Route: /api/alerts
//
// GET /api/alerts?limit=50  — Returns recent alerts, newest first
//
// Authentication: requires JWT cookie (dashboard users only)
// =============================================================================

const { Router }    = require('express');
const sensorService = require('../services/sensorService');
const requireAuth   = require('../middleware/requireAuth');

const router = Router();

// ---------------------------------------------------------------------------
// GET /api/alerts
// ---------------------------------------------------------------------------
router.get('/', requireAuth, (req, res) => {
  const limit = Math.min(parseInt(req.query.limit) || 50, 500);
  res.json(sensorService.getRecentAlerts(limit));
});

module.exports = router;
