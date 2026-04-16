// =============================================================================
// UncleZero — Gas Pressure Monitor Server
//
// Entry point. Wires together:
//   • Helmet       — Security headers (CSP, HSTS, X-Frame-Options, etc.)
//   • Express      — REST API for ESP32 sensor reports and dashboard data
//   • Auth         — JWT cookie sessions + API key auth for ESP32
//   • Socket.io    — Real-time push to dashboard clients (JWT-verified)
//   • Static       — Serves the dashboard and login page from /public
// =============================================================================

// Load .env before anything else so every module sees the environment variables
require('dotenv').config();

const http         = require('http');
const path         = require('path');
const express      = require('express');
const helmet       = require('helmet');
const cors         = require('cors');
const cookieParser = require('cookie-parser');
const { Server: SocketServer } = require('socket.io');

const config        = require('./config');
const authService   = require('./services/authService');
const sensorService = require('./services/sensorService');
const requireAuth   = require('./middleware/requireAuth');
const authRouter    = require('./routes/auth');
const sensorsRouter = require('./routes/sensors');
const alertsRouter  = require('./routes/alerts');

// ---------------------------------------------------------------------------
// First-run setup
// Creates a default admin user and ESP32 API key if none exist yet.
// Credentials are printed to the console once and never shown again.
// ---------------------------------------------------------------------------
authService.seedDefaultAdmin();
authService.seedDefaultApiKey();

// ---------------------------------------------------------------------------
// Express App
// ---------------------------------------------------------------------------

const app = express();

app.use(helmet({
  // Allow Socket.io to work within the CSP
  contentSecurityPolicy: {
    directives: {
      defaultSrc:  ["'self'"],
      scriptSrc:   ["'self'"],
      connectSrc:  ["'self'", 'ws:', 'wss:'], // WebSocket connections
      styleSrc:    ["'self'", "'unsafe-inline'"], // inline styles in HTML
      imgSrc:      ["'self'", 'data:'],
    },
  },
}));

app.use(cors());
app.use(express.json());
app.use(cookieParser());

// ---------------------------------------------------------------------------
// Public Routes (no authentication required)
// ---------------------------------------------------------------------------

// Login page — must come before the authenticated static file handler
app.get('/login', (_req, res) => {
  res.sendFile(path.join(__dirname, '../public/login.html'));
});

// Auth endpoints (login, logout, session check)
app.use('/auth', authRouter);

// Health check — used by load balancers, Docker health checks, uptime monitors
app.get('/health', (_req, res) => {
  res.json({ status: 'ok', uptime: process.uptime() });
});

// ---------------------------------------------------------------------------
// Protected Routes (authentication required)
// ---------------------------------------------------------------------------

// Dashboard — redirect to /login if not authenticated
app.get('/', requireAuth, (_req, res) => {
  res.sendFile(path.join(__dirname, '../public/index.html'));
});

// Sensor + alert API routes (auth enforced per-route in each router)
app.use('/api/sensors', sensorsRouter);
app.use('/api/alerts',  alertsRouter);

// Static assets (dashboard.js, icons, etc.) — served without auth since
// they contain no sensitive data. index.html is excluded via { index: false }.
app.use(express.static(path.join(__dirname, '../public'), { index: false }));

// ---------------------------------------------------------------------------
// HTTP Server + Socket.io
// ---------------------------------------------------------------------------

const httpServer = http.createServer(app);

const io = new SocketServer(httpServer, {
  cors: { origin: '*' },
});

// Verify the JWT cookie on every Socket.io connection.
// Browsers send cookies automatically with the WebSocket upgrade request.
io.use((socket, next) => {
  const cookieHeader = socket.handshake.headers.cookie || '';
  const token = parseCookieHeader(cookieHeader)['token'];

  if (!token) return next(new Error('Authentication required.'));

  try {
    socket.user = authService.verifyJwt(token);
    next();
  } catch {
    next(new Error('Session expired. Please log in again.'));
  }
});

io.on('connection', (socket) => {
  console.log(`[WS] Connected    — ${socket.id} (${socket.user?.username})`);
  socket.on('disconnect', () => {
    console.log(`[WS] Disconnected — ${socket.id}`);
  });
});

// Forward every sensor event to all authenticated dashboard clients.
// Add other integrations (SMS, PagerDuty, dispatch) the same way:
//   sensorService.onSensorEvent(sendSmsAlert);
sensorService.onSensorEvent((event) => {
  io.emit('sensor:update', event);
});

// ---------------------------------------------------------------------------
// Daily Maintenance: prune old alert records
// ---------------------------------------------------------------------------

const ONE_DAY_MS = 24 * 60 * 60 * 1000;
setInterval(() => {
  const deleted = sensorService.pruneOldAlerts();
  if (deleted > 0) console.log(`[DB] Pruned ${deleted} old alert records.`);
}, ONE_DAY_MS);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Parse a raw Cookie header string into a key→value map. */
function parseCookieHeader(cookieHeader) {
  return Object.fromEntries(
    cookieHeader
      .split(';')
      .map(part => part.trim().split('='))
      .filter(parts => parts.length === 2)
      .map(([k, v]) => [k.trim(), decodeURIComponent(v.trim())]),
  );
}

// ---------------------------------------------------------------------------
// Start
// ---------------------------------------------------------------------------

httpServer.listen(config.server.port, config.server.host, () => {
  console.log('  ─────────────────────────────────────────────────');
  console.log('  UncleZero Gas Pressure Monitor');
  console.log(`  Server    http://${config.server.host}:${config.server.port}`);
  console.log(`  Dashboard http://localhost:${config.server.port}`);
  console.log(`  API       http://localhost:${config.server.port}/api/sensors`);
  console.log('  ─────────────────────────────────────────────────');
  console.log('');
});
