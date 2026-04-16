// =============================================================================
// Route: /api/admin
//
// Admin-only API endpoints. Every route here requires a valid JWT session —
// unauthenticated requests are rejected with 401.
//
// POST /api/admin/users  — Create a new dashboard user
// GET  /api/admin/users  — List all users (username + created date, no hashes)
// =============================================================================

const { Router }    = require('express');
const authService   = require('../services/authService');
const requireAuth   = require('../middleware/requireAuth');
const db            = require('../database');

const router = Router();

// All admin routes require a valid login session
router.use(requireAuth);

// ---------------------------------------------------------------------------
// POST /api/admin/users  —  Create a new user
// ---------------------------------------------------------------------------
router.post('/users', (req, res) => {
  const { username, password } = req.body;

  if (!username || typeof username !== 'string' || username.trim().length < 3) {
    return res.status(400).json({ error: 'Username must be at least 3 characters.' });
  }
  if (!password || typeof password !== 'string' || password.length < 8) {
    return res.status(400).json({ error: 'Password must be at least 8 characters.' });
  }

  // Check for duplicate username
  const existing = db.prepare('SELECT id FROM users WHERE username = ?').get(username.trim());
  if (existing) {
    return res.status(409).json({ error: `Username "${username.trim()}" is already taken.` });
  }

  authService.createUser(username.trim(), password);
  console.log(`[ADMIN] User "${username.trim()}" created by "${req.user.username}"`);

  res.status(201).json({ ok: true, username: username.trim() });
});

// ---------------------------------------------------------------------------
// GET /api/admin/users  —  List all users (safe fields only)
// ---------------------------------------------------------------------------
router.get('/users', (_req, res) => {
  const users = db
    .prepare('SELECT id, username, created_at FROM users ORDER BY created_at ASC')
    .all();
  res.json(users);
});

module.exports = router;
