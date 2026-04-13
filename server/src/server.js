// =============================================================================
// UncleZero — Gas Pressure Monitor Server
//
// Entry point. Wires together:
//   • Express    — REST API for ESP32 sensor reports and dashboard data
//   • Socket.io  — Real-time push to dashboard clients
//   • Static     — Serves the dashboard from /public
// =============================================================================

const http    = require('http');
const path    = require('path');
const express = require('express');
const cors    = require('cors');
const { Server: SocketServer } = require('socket.io');

const config        = require('./config');
const sensorService = require('./services/sensorService');
const sensorsRouter = require('./routes/sensors');
const alertsRouter  = require('./routes/alerts');

// ---------------------------------------------------------------------------
// Express App
// ---------------------------------------------------------------------------

const app = express();

app.use(cors());
app.use(express.json());

// Serve the dashboard (index.html + assets) from the public/ directory
app.use(express.static(path.join(__dirname, '../public')));

// ---------------------------------------------------------------------------
// REST API Routes
// ---------------------------------------------------------------------------

app.use('/api/sensors', sensorsRouter);
app.use('/api/alerts',  alertsRouter);

// Health check — used by load balancers, Docker health checks, uptime monitors
app.get('/health', (_req, res) => {
  res.json({ status: 'ok', uptime: process.uptime() });
});

// ---------------------------------------------------------------------------
// HTTP Server + Socket.io
// ---------------------------------------------------------------------------

const httpServer = http.createServer(app);

const io = new SocketServer(httpServer, {
  cors: { origin: '*' },
});

// Forward every sensor event to all connected dashboard clients.
// Other integrations (SMS, PagerDuty, etc.) plug in here the same way:
//   sensorService.onSensorEvent(sendSmsAlert);
sensorService.onSensorEvent((event) => {
  io.emit('sensor:update', event);
});

io.on('connection', (socket) => {
  console.log(`[WS] Client connected    — ${socket.id}`);
  socket.on('disconnect', () => {
    console.log(`[WS] Client disconnected — ${socket.id}`);
  });
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
// Start
// ---------------------------------------------------------------------------

httpServer.listen(config.server.port, config.server.host, () => {
  console.log('');
  console.log('  UncleZero Gas Pressure Monitor');
  console.log('  ─────────────────────────────────────────────────');
  console.log(`  Server    http://${config.server.host}:${config.server.port}`);
  console.log(`  Dashboard http://localhost:${config.server.port}`);
  console.log(`  API       http://localhost:${config.server.port}/api/sensors`);
  console.log('  ─────────────────────────────────────────────────');
  console.log('');
});
