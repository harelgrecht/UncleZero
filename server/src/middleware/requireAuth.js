// =============================================================================
// Middleware: requireAuth
//
// Protects dashboard routes (browser clients).
// Reads the JWT from the httpOnly cookie set at login.
//
// On failure:
//   • JSON clients  (Accept: application/json) → 401 JSON error
//   • Browser clients                          → redirect to /login
// =============================================================================

const authService = require('../services/authService');

function requireAuth(req, res, next) {
  const token = req.cookies?.token;

  if (!token) {
    return respondUnauthorized(req, res, 'Authentication required. Please log in.');
  }

  try {
    req.user = authService.verifyJwt(token);
    next();
  } catch {
    // Token is expired or tampered with — clear it and force re-login
    res.clearCookie('token');
    respondUnauthorized(req, res, 'Session expired. Please log in again.');
  }
}

// Send either a JSON 401 or a browser redirect depending on the client type
function respondUnauthorized(req, res, message) {
  const prefersJson = req.accepts(['html', 'json']) === 'json';
  if (prefersJson) {
    return res.status(401).json({ error: message });
  }
  res.redirect('/login');
}

module.exports = requireAuth;
