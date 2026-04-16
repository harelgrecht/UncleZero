// =============================================================================
// Route: /auth
//
// POST /auth/login   — Verify credentials, set JWT cookie
// POST /auth/logout  — Clear JWT cookie
// GET  /auth/me      — Return the current user (dashboard session check)
// =============================================================================

const { Router }  = require('express');
const rateLimit   = require('express-rate-limit');
const authService = require('../services/authService');
const config      = require('../config');

const router = Router();

// ---------------------------------------------------------------------------
// Brute-force protection on the login endpoint
// Max 10 attempts per 15 minutes per IP address.
// ---------------------------------------------------------------------------
const loginLimiter = rateLimit({
  windowMs:       15 * 60 * 1000, // 15 minutes
  max:            10,
  standardHeaders: true,
  legacyHeaders:  false,
  message:        { error: 'Too many login attempts. Please try again in 15 minutes.' },
});

// ---------------------------------------------------------------------------
// POST /auth/login
// ---------------------------------------------------------------------------
router.post('/login', loginLimiter, (req, res) => {
  const { username, password } = req.body;

  if (!username || !password) {
    return res.status(400).json({ error: 'Username and password are required.' });
  }

  const user = authService.verifyCredentials(username, password);
  if (!user) {
    // Return the same message for wrong username or wrong password
    // (prevents user enumeration attacks)
    return res.status(401).json({ error: 'Invalid username or password.' });
  }

  const token = authService.generateJwt(user);

  // httpOnly  — JS cannot read this cookie (blocks XSS token theft)
  // SameSite  — cookie is not sent on cross-site requests (blocks CSRF)
  // secure    — HTTPS only in production
  res.cookie('token', token, {
    httpOnly: true,
    sameSite: 'Strict',
    secure:   process.env.NODE_ENV === 'production',
    maxAge:   config.auth.jwtExpirySeconds * 1000,
  });

  res.json({ ok: true, username: user.username });
});

// ---------------------------------------------------------------------------
// POST /auth/logout
// ---------------------------------------------------------------------------
router.post('/logout', (_req, res) => {
  res.clearCookie('token');
  res.json({ ok: true });
});

// ---------------------------------------------------------------------------
// GET /auth/me  —  Let the dashboard confirm its session is still valid
// ---------------------------------------------------------------------------
router.get('/me', (req, res) => {
  const token = req.cookies?.token;
  if (!token) return res.status(401).json({ error: 'Not authenticated.' });

  try {
    const user = authService.verifyJwt(token);
    res.json({ username: user.username });
  } catch {
    res.status(401).json({ error: 'Session expired.' });
  }
});

module.exports = router;
