// =============================================================================
// Auth Service  —  Users, Sessions & API Keys
//
// Handles all authentication logic in one place:
//   • User accounts  — bcrypt-hashed passwords, JWT session tokens
//   • API keys       — SHA-256-hashed keys for ESP32 devices
//   • First-run seed — creates the default admin and ESP32 key on startup
//
// Why SHA-256 for API keys instead of bcrypt?
//   API keys are already long, high-entropy random strings — they don't need
//   bcrypt's key-stretching. SHA-256 is the industry standard for API tokens
//   (used by GitHub, npm, Stripe, etc.) and is constant-time comparable.
// =============================================================================

const bcrypt = require('bcrypt');
const jwt    = require('jsonwebtoken');
const crypto = require('crypto');

const db     = require('../database');
const config = require('../config');

const BCRYPT_ROUNDS = 12;

// ---------------------------------------------------------------------------
// Prepared Statements
// ---------------------------------------------------------------------------

const stmts = {
  getUserByUsername: db.prepare('SELECT * FROM users WHERE username = ?'),
  getUserCount:      db.prepare('SELECT COUNT(*) as count FROM users'),
  insertUser:        db.prepare(`
    INSERT INTO users (username, password_hash, created_at)
    VALUES (@username, @passwordHash, @createdAt)
  `),

  getApiKeyByHash: db.prepare('SELECT id FROM api_keys WHERE key_hash = ?'),
  getApiKeyCount:  db.prepare('SELECT COUNT(*) as count FROM api_keys'),
  insertApiKey:    db.prepare(`
    INSERT INTO api_keys (label, key_hash, created_at)
    VALUES (@label, @keyHash, @createdAt)
  `),
};

// ---------------------------------------------------------------------------
// User Management
// ---------------------------------------------------------------------------

/**
 * Create a new user with a securely hashed password.
 * @param {string} username
 * @param {string} password  — plain text (will be hashed)
 */
function createUser(username, password) {
  const passwordHash = bcrypt.hashSync(password, BCRYPT_ROUNDS);
  stmts.insertUser.run({ username, passwordHash, createdAt: Date.now() });
}

/**
 * Verify a username + password pair.
 * Returns the user object on success, or null on failure.
 * Timing-safe: always runs bcrypt even if the user doesn't exist.
 *
 * @param {string} username
 * @param {string} password  — plain text
 * @returns {{ id: number, username: string } | null}
 */
function verifyCredentials(username, password) {
  const user = stmts.getUserByUsername.get(username);

  // Always run bcrypt to prevent user-enumeration via timing difference
  const hashToCheck = user?.password_hash ?? '$2b$12$invalidhashfortimingprotection000000000000000000000000';
  const valid = bcrypt.compareSync(password, hashToCheck);

  return valid && user ? { id: user.id, username: user.username } : null;
}

// ---------------------------------------------------------------------------
// JWT Session Tokens
// ---------------------------------------------------------------------------

/**
 * Generate a signed JWT for a verified user.
 * @param {{ id: number, username: string }} user
 * @returns {string}
 */
function generateJwt(user) {
  return jwt.sign(
    { sub: user.id, username: user.username },
    config.auth.jwtSecret,
    { expiresIn: config.auth.jwtExpirySeconds },
  );
}

/**
 * Verify a JWT and return its payload.
 * Throws if the token is invalid or expired.
 * @param {string} token
 * @returns {object}
 */
function verifyJwt(token) {
  return jwt.verify(token, config.auth.jwtSecret);
}

// ---------------------------------------------------------------------------
// API Keys (for ESP32 devices)
// ---------------------------------------------------------------------------

function hashApiKey(rawKey) {
  return crypto.createHash('sha256').update(rawKey).digest('hex');
}

/**
 * Register a new API key.
 * @param {string} label   — human-readable name (e.g. "kitchen-esp32")
 * @param {string} rawKey  — the plain-text key (store this in your ESP32 firmware)
 */
function createApiKey(label, rawKey) {
  stmts.insertApiKey.run({ label, keyHash: hashApiKey(rawKey), createdAt: Date.now() });
}

/**
 * Verify an incoming API key from the X-Api-Key header.
 * @param {string} rawKey
 * @returns {boolean}
 */
function verifyApiKey(rawKey) {
  const keyHash = hashApiKey(rawKey);
  return !!stmts.getApiKeyByHash.get(keyHash);
}

// ---------------------------------------------------------------------------
// First-run Seed
// Called on server startup to ensure there is always at least one admin user
// and one API key so the system is usable out of the box.
// ---------------------------------------------------------------------------

function seedDefaultAdmin() {
  const { count } = stmts.getUserCount.get();
  if (count > 0) return; // Already has users — nothing to do

  const username  = config.auth.adminUsername;
  let   password  = config.auth.adminPassword;
  let   generated = false;

  if (!password) {
    password  = crypto.randomBytes(12).toString('base64url');
    generated = true;
  }

  createUser(username, password);

  console.log('');
  console.log('  [AUTH] No users found — default admin account created:');
  console.log(`         Username : ${username}`);
  if (generated) {
    console.log(`         Password : ${password}`);
    console.log('                    ^ Save this now. It will not be shown again.');
  }
  console.log('');
}

function seedDefaultApiKey() {
  const { count } = stmts.getApiKeyCount.get();
  if (count > 0) return; // Already has keys — nothing to do

  let rawKey   = config.auth.esp32ApiKey;
  let generated = false;

  if (!rawKey) {
    rawKey    = crypto.randomBytes(24).toString('base64url');
    generated = true;
  }

  createApiKey('esp32-default', rawKey);

  if (generated) {
    console.log('  [AUTH] No API keys found — default ESP32 key created:');
    console.log(`         X-Api-Key: ${rawKey}`);
    console.log('                    ^ Add this header to your ESP32 HTTP requests.');
    console.log('');
  }
}

module.exports = {
  createUser,
  verifyCredentials,
  generateJwt,
  verifyJwt,
  createApiKey,
  verifyApiKey,
  seedDefaultAdmin,
  seedDefaultApiKey,
};
