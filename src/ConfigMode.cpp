#include "../include/ConfigMode.h"
#include "../include/Config.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

// ==========================================
// Module state
// ==========================================
static Preferences   prefs;
static WebServer     server(80);
static DNSServer     dnsServer;
static unsigned long lastActivityMs = 0;

// ==========================================
// Embedded HTML config page
// ==========================================
static const char CONFIG_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>DodZero Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;background:#ECEFF1;color:#263238;padding:16px}
.wrap{max-width:480px;margin:0 auto}
header{background:linear-gradient(135deg,#1565C0,#0D47A1);color:#fff;padding:18px 20px;border-radius:14px;margin-bottom:16px}
header h1{font-size:21px;font-weight:700;margin-bottom:2px}
header p{font-size:12px;opacity:.75}
.card{background:#fff;border-radius:14px;padding:18px 20px;margin-bottom:14px;box-shadow:0 1px 6px rgba(0,0,0,.08)}
.card-title{font-size:11px;font-weight:700;color:#90A4AE;text-transform:uppercase;letter-spacing:.8px;margin-bottom:14px}
.row{display:flex;align-items:center;gap:10px;margin-bottom:6px}
.dot{width:11px;height:11px;border-radius:50%;flex-shrink:0}
.dot-on{background:#2E7D32} .dot-off{background:#C62828}
.st{font-size:17px;font-weight:700}
.sub{font-size:13px;color:#78909C;margin-top:3px}
.bars{display:flex;align-items:flex-end;gap:3px;height:26px;margin-top:12px}
.bar{width:13px;border-radius:2px 2px 0 0;background:#E0E0E0}
.net-list{margin-top:10px;border:1px solid #ECEFF1;border-radius:10px;max-height:200px;overflow-y:auto}
.net{display:flex;justify-content:space-between;align-items:center;padding:11px 14px;cursor:pointer;border-bottom:1px solid #F5F5F5;transition:background .15s}
.net:last-child{border-bottom:none}
.net:hover,.net.sel{background:#E3F2FD}
.net-name{font-size:14px;color:#263238}
.net-rssi{font-size:12px;color:#90A4AE}
label{display:block;font-size:13px;font-weight:600;color:#546E7A;margin:13px 0 5px}
input[type=text],input[type=password]{width:100%;padding:12px 14px;border:1.5px solid #CFD8DC;border-radius:10px;font-size:15px;color:#263238;background:#FAFAFA;outline:none;transition:border .2s}
input[type=text]:focus,input[type=password]:focus{border-color:#1565C0;background:#fff}
.pw{position:relative}
.pw input{padding-right:64px}
.eye{position:absolute;right:12px;top:50%;transform:translateY(-50%);background:none;border:none;color:#1565C0;font-size:13px;font-weight:600;cursor:pointer;padding:4px}
.btn{width:100%;padding:14px;border:none;border-radius:11px;font-size:15px;font-weight:700;cursor:pointer;transition:opacity .2s}
.btn:disabled{opacity:.5;cursor:not-allowed}
.primary{background:#1565C0;color:#fff;margin-top:14px}
.primary:hover:not(:disabled){background:#0D47A1}
.outline{background:#fff;color:#1565C0;border:2px solid #1565C0;margin-top:8px}
.outline:hover:not(:disabled){background:#E3F2FD}
.danger{background:#fff;color:#C62828;border:2px solid #C62828;margin-top:8px}
.danger:hover:not(:disabled){background:#FFEBEE}
.alert{border-radius:10px;padding:13px 14px;margin-top:12px;font-size:13px;line-height:1.5}
.ok{background:#E8F5E9;color:#2E7D32}
.err{background:#FFEBEE;color:#C62828}
.info{background:#E3F2FD;color:#1565C0}
.spin{display:inline-block;width:14px;height:14px;border:2px solid currentColor;border-top-color:transparent;border-radius:50%;animation:sp .6s linear infinite;vertical-align:middle;margin-right:6px}
@keyframes sp{to{transform:rotate(360deg)}}
.hidden{display:none}
.chip{display:inline-block;background:#E3F2FD;color:#1565C0;font-size:11px;font-weight:700;padding:3px 8px;border-radius:20px;margin-left:8px}
.toggle-row{display:flex;justify-content:space-between;align-items:flex-start;gap:16px;padding:6px 0}
.tl{font-size:14px;font-weight:600;color:#263238}
.td{font-size:12px;color:#78909C;margin-top:4px;line-height:1.5}
.switch{position:relative;display:inline-block;width:50px;height:28px;flex-shrink:0;margin-top:2px}
.switch input{opacity:0;width:0;height:0}
.sl{position:absolute;cursor:pointer;inset:0;background:#CFD8DC;border-radius:28px;transition:.3s}
.sl:before{position:absolute;content:'';height:22px;width:22px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.3s;box-shadow:0 1px 3px rgba(0,0,0,.2)}
input:checked+.sl{background:#1565C0}
input:checked+.sl:before{transform:translateX(22px)}
</style>
</head>
<body>
<div class="wrap">
<header><h1>DodZero Setup</h1><p>WiFi Configuration Portal</p></header>

<div class="card">
  <div class="card-title">Connection Status</div>
  <div class="row">
    <div class="dot dot-off" id="dot"></div>
    <span class="st" id="st">Loading...</span>
    <span class="chip hidden" id="chip"></span>
  </div>
  <div class="sub" id="ssid-sub"></div>
  <div class="sub" id="ip-sub"></div>
  <div class="bars">
    <div class="bar" id="b1" style="height:6px"></div>
    <div class="bar" id="b2" style="height:12px"></div>
    <div class="bar" id="b3" style="height:19px"></div>
    <div class="bar" id="b4" style="height:26px"></div>
  </div>
  <div class="sub" id="rssi-sub" style="margin-top:5px"></div>
</div>

<div class="card">
  <div class="card-title">WiFi Configuration</div>
  <button class="btn outline" id="scan-btn" onclick="doScan()">Scan Nearby Networks</button>
  <div class="net-list hidden" id="net-list"></div>
  <label for="ssid">Network Name (SSID)</label>
  <input type="text" id="ssid" placeholder="e.g. MyHomeNetwork" autocomplete="off" autocapitalize="none" spellcheck="false">
  <label for="pw">Password</label>
  <div class="pw">
    <input type="password" id="pw" placeholder="Enter password" autocomplete="off">
    <button class="eye" type="button" onclick="togglePw()">Show</button>
  </div>
  <button class="btn primary" id="save-btn" onclick="doSave()">Save &amp; Connect</button>
  <div id="save-result"></div>
</div>

<div class="card">
  <div class="card-title">Device Settings</div>
  <div class="toggle-row">
    <div>
      <div class="tl">Always-on Web Portal</div>
      <div class="td">Keep this page accessible at <strong>http://dodzero.local</strong> without pressing the config button. The <strong>DodZero-Setup</strong> AP will stay active so you can always reconfigure WiFi. Takes effect after next restart.</div>
    </div>
    <label class="switch">
      <input type="checkbox" id="web-always-on" onchange="saveSetting()">
      <span class="sl"></span>
    </label>
  </div>
  <div id="setting-result"></div>
</div>

<div class="card">
  <div class="card-title">Device</div>
  <div class="sub">From your home network: <strong style="color:#1565C0">http://dodzero.local</strong></div>
  <div class="sub" style="margin-top:6px">AP always available: <strong style="color:#1565C0">http://192.168.4.1</strong> &nbsp;(DodZero-Setup)</div>
  <button class="btn danger" id="restart-btn" onclick="doRestart()">Restart Device</button>
</div>
</div>

<script>
function qi(id){return document.getElementById(id);}
function rssiInfo(r){
  if(r==null)return{bars:0,lbl:'',col:'#E0E0E0'};
  if(r>=-50)return{bars:4,lbl:'Excellent',col:'#2E7D32'};
  if(r>=-60)return{bars:3,lbl:'Good',col:'#558B2F'};
  if(r>=-75)return{bars:2,lbl:'Fair',col:'#F57F17'};
  if(r>=-85)return{bars:1,lbl:'Poor',col:'#E65100'};
  return{bars:0,lbl:'Very Poor',col:'#B71C1C'};
}
function setBars(rssi){
  var i=rssiInfo(rssi);
  ['b1','b2','b3','b4'].forEach(function(id,idx){qi(id).style.background=idx<i.bars?i.col:'#E0E0E0';});
  qi('rssi-sub').textContent=rssi!=null?rssi+' dBm - '+i.lbl:'';
}
function loadStatus(){
  fetch('/api/status').then(function(r){return r.json();}).then(function(d){
    var on=d.wifi_connected;
    qi('dot').className='dot '+(on?'dot-on':'dot-off');
    qi('st').textContent=on?'Connected':'Not Connected';
    var chip=qi('chip');
    if(on){chip.textContent=d.ssid;chip.classList.remove('hidden');}else chip.classList.add('hidden');
    qi('ssid-sub').textContent=on?'Network: '+d.ssid:'No WiFi connection';
    qi('ip-sub').textContent=on?'IP: '+d.ip:'AP: '+d.ap_ip;
    setBars(on?d.rssi:null);
  }).catch(function(){});
}
function loadSettings(){
  fetch('/api/settings').then(function(r){return r.json();}).then(function(d){
    qi('web-always-on').checked=!!d.web_always_on;
  }).catch(function(){});
}
function saveSetting(){
  var val=qi('web-always-on').checked;
  fetch('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'web_always_on='+(val?'1':'0')})
  .then(function(r){return r.json();}).then(function(){
    var res=qi('setting-result');
    res.innerHTML='<div class="alert ok" style="margin-top:10px">'
      +(val?'Always-on portal <strong>enabled</strong>. Restart to activate.'
           :'Always-on portal <strong>disabled</strong>. Device will deep-sleep after restart.')
      +'</div>';
    setTimeout(function(){res.innerHTML='';},5000);
  });
}
function doScan(){
  var btn=qi('scan-btn');
  btn.disabled=true;btn.innerHTML='<span class="spin"></span>Scanning...';
  fetch('/api/scan').then(function(r){return r.json();}).then(function(d){
    var list=qi('net-list');list.classList.remove('hidden');
    list.innerHTML=d.networks.map(function(n){
      return '<div class="net" onclick="pickNet(this,\''+esc(n.ssid)+'\')">'
        +'<span class="net-name">'+(n.secure?'&#128274; ':'')+esc(n.ssid)+'</span>'
        +'<span class="net-rssi">'+n.rssi+' dBm</span></div>';
    }).join('')||'<div class="net"><span class="net-name" style="color:#90A4AE">No networks found</span></div>';
  }).catch(function(){}).finally(function(){btn.disabled=false;btn.textContent='Scan Nearby Networks';});
}
function pickNet(el,ssid){
  document.querySelectorAll('.net').forEach(function(e){e.classList.remove('sel');});
  el.classList.add('sel');qi('ssid').value=ssid;qi('pw').focus();
}
function doSave(){
  var ssid=qi('ssid').value.trim(),pw=qi('pw').value;
  if(!ssid){alert('Please enter or select a network name.');return;}
  var btn=qi('save-btn'),res=qi('save-result');
  btn.disabled=true;btn.innerHTML='<span class="spin"></span>Connecting...';
  res.innerHTML='<div class="alert info">Connecting - this may take up to 15 seconds...</div>';
  fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(ssid)+'&password='+encodeURIComponent(pw)})
  .then(function(r){return r.json();}).then(function(d){
    if(d.status==='ok'){
      res.innerHTML='<div class="alert ok">Connected! IP: <strong>'+d.ip+'</strong><br>Reach this page at <a href="http://dodzero.local" style="color:#2E7D32">http://dodzero.local</a></div>';
      loadStatus();
    }else{
      res.innerHTML='<div class="alert err">Failed: '+(d.msg||'Check your password and try again.')+'</div>';
    }
  }).catch(function(){
    res.innerHTML='<div class="alert info">Device may be restarting. Try <strong>http://dodzero.local</strong> from your home network.</div>';
  }).finally(function(){btn.disabled=false;btn.textContent='Save & Connect';});
}
function togglePw(){
  var inp=qi('pw');inp.type=inp.type==='password'?'text':'password';
  event.target.textContent=inp.type==='password'?'Show':'Hide';
}
function doRestart(){
  if(!confirm('Restart the DodZero device?'))return;
  var btn=qi('restart-btn');btn.disabled=true;btn.innerHTML='<span class="spin"></span>Restarting...';
  fetch('/api/restart',{method:'POST'}).finally(function(){
    setTimeout(function(){btn.disabled=false;btn.textContent='Restart Device';},6000);
  });
}
function esc(s){return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;');}
loadStatus();loadSettings();setInterval(loadStatus,5000);
</script>
</body>
</html>
)rawhtml";

// ==========================================
// NVS helpers
// ==========================================

bool loadWifiCredentials(String& ssid, String& password) {
    // Open read-write (false) so NVS namespace is created on first boot
    prefs.begin(NVS_NAMESPACE, false);
    ssid     = prefs.getString(NVS_KEY_SSID, "");
    password = prefs.getString(NVS_KEY_PASS, "");
    prefs.end();
    return ssid.length() > 0;
}

static void saveWifiCredentials(const String& ssid, const String& password) {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(NVS_KEY_SSID, ssid.c_str());
    prefs.putString(NVS_KEY_PASS, password.c_str());
    prefs.end();
}

bool isWebAlwaysOn() {
    // Open read-write (false) so NVS namespace is created on first boot
    prefs.begin(NVS_NAMESPACE, false);
    bool val = prefs.getBool(NVS_KEY_WEB_ALWAYS_ON, true);  // default ON
    prefs.end();
    return val;
}

// ==========================================
// Web API handlers
// ==========================================

static void handleRoot() {
    lastActivityMs = millis();
    server.send_P(200, "text/html", CONFIG_HTML);
}

static void handleStatus() {
    lastActivityMs = millis();
    bool connected = (WiFi.status() == WL_CONNECTED);
    JsonDocument doc;
    doc["wifi_connected"] = connected;
    doc["ap_ip"]          = WiFi.softAPIP().toString();
    if (connected) {
        doc["ssid"] = WiFi.SSID();
        doc["rssi"] = WiFi.RSSI();
        doc["ip"]   = WiFi.localIP().toString();
    }
    String out; serializeJson(doc, out);
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "application/json", out);
}

static void handleScan() {
    lastActivityMs = millis();
    int n = WiFi.scanNetworks(false, true);
    JsonDocument doc;
    JsonArray nets = doc["networks"].to<JsonArray>();
    if (n > 0) {
        for (int i = 0; i < min(n, 20); i++) {
            if (WiFi.SSID(i).length() == 0) continue;
            JsonObject net = nets.add<JsonObject>();
            net["ssid"]   = WiFi.SSID(i);
            net["rssi"]   = WiFi.RSSI(i);
            net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        }
    }
    WiFi.scanDelete();
    String out; serializeJson(doc, out);
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "application/json", out);
}

static void handleConfig() {
    lastActivityMs = millis();
    String ssid = server.arg("ssid");
    String pass = server.arg("password");
    if (ssid.isEmpty()) {
        server.send(400, "application/json", "{\"status\":\"error\",\"msg\":\"SSID required\"}");
        return;
    }
    saveWifiCredentials(ssid, pass);
    WiFi.disconnect(false);
    WiFi.begin(ssid.c_str(), pass.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 24) { delay(500); attempts++; }
    JsonDocument doc;
    if (WiFi.status() == WL_CONNECTED) {
        doc["status"] = "ok";
        doc["ssid"]   = WiFi.SSID();
        doc["rssi"]   = WiFi.RSSI();
        doc["ip"]     = WiFi.localIP().toString();
        MDNS.begin(CONFIG_MDNS_HOST);
    } else {
        doc["status"] = "failed";
        doc["msg"]    = "Could not connect - check the password";
    }
    String out; serializeJson(doc, out);
    server.send(200, "application/json", out);
}

static void handleGetSettings() {
    lastActivityMs = millis();
    JsonDocument doc;
    doc["web_always_on"] = isWebAlwaysOn();
    String out; serializeJson(doc, out);
    server.sendHeader("Cache-Control", "no-cache");
    server.send(200, "application/json", out);
}

static void handlePostSettings() {
    lastActivityMs = millis();
    bool val = (server.arg("web_always_on") == "1");
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putBool(NVS_KEY_WEB_ALWAYS_ON, val);
    prefs.end();
    Serial.printf("[CONFIG] always-on portal: %s\r\n", val ? "ON" : "OFF");
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

static void handleRestart() {
    lastActivityMs = millis();
    server.send(200, "application/json", "{\"status\":\"restarting\"}");
    delay(300);
    ESP.restart();
}

static void handleCaptive() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/");
    server.send(302, "text/plain", "Redirecting...");
}

// ==========================================
// Shared helpers
// ==========================================

static void startAP() {
    WiFi.persistent(false);   // don't write credentials to flash
    WiFi.setSleep(false);     // keep radio awake — improves AP+STA stability
    WiFi.mode(WIFI_AP_STA);
    delay(200);               // let mode switch settle
    bool ok = WiFi.softAP(CONFIG_AP_SSID, nullptr, 1, false, 4);
    delay(500);               // give AP time to start beaconing before STA scan
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", WiFi.softAPIP());
    Serial.printf("[AP] '%s'  IP: %s  (%s)\r\n", CONFIG_AP_SSID,
                  WiFi.softAPIP().toString().c_str(),
                  ok ? "OK" : "FAILED");
}

static void setupWebServer() {
    server.on("/",                           HTTP_GET,  handleRoot);
    server.on("/api/status",                 HTTP_GET,  handleStatus);
    server.on("/api/scan",                   HTTP_GET,  handleScan);
    server.on("/api/config",                 HTTP_POST, handleConfig);
    server.on("/api/settings",               HTTP_GET,  handleGetSettings);
    server.on("/api/settings",               HTTP_POST, handlePostSettings);
    server.on("/api/restart",                HTTP_POST, handleRestart);
    server.on("/hotspot-detect.html",        HTTP_GET,  handleRoot);
    server.on("/library/test/success.html",  HTTP_GET,  handleRoot);
    server.on("/success.txt",                HTTP_GET,  handleRoot);
    server.on("/generate_204",               HTTP_GET,  handleRoot);
    server.on("/connectcheck.html",          HTTP_GET,  handleRoot);
    server.on("/redirect",                   HTTP_GET,  handleCaptive);
    server.on("/ncsi.txt",                   HTTP_GET,  handleRoot);
    server.onNotFound(handleCaptive);
    server.begin();
}

// ==========================================
// enterConfigMode — triggered by dev pin
// ==========================================

void enterConfigMode() {
    Serial.println("\r\n[CONFIG] *** WiFi Configuration Mode ***");

    for (int i = 0; i < 6; i++) {
        digitalWrite(statusLedPin, ledOn);  delay(100);
        digitalWrite(statusLedPin, ledOff); delay(100);
    }
    digitalWrite(statusLedPin, ledOn);

    startAP();

    String storedSsid, storedPass;
    if (loadWifiCredentials(storedSsid, storedPass)) {
        Serial.printf("[CONFIG] Connecting to stored WiFi: %s\r\n", storedSsid.c_str());
        WiFi.begin(storedSsid.c_str(), storedPass.c_str());
    }

    setupWebServer();

    if (MDNS.begin(CONFIG_MDNS_HOST)) {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("[CONFIG] mDNS: http://%s.local\r\n", CONFIG_MDNS_HOST);
    }

    Serial.printf("[CONFIG] Connect to '%s' then open http://192.168.4.1\r\n", CONFIG_AP_SSID);

    lastActivityMs = millis();
    bool wifiPrinted = false;

    while (true) {
        dnsServer.processNextRequest();
        server.handleClient();

        if (!wifiPrinted && WiFi.status() == WL_CONNECTED) {
            wifiPrinted = true;
            Serial.printf("[CONFIG] Also at http://%s\r\n",
                          WiFi.localIP().toString().c_str());
        }

        static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 800) {
            lastBlink = millis();
            digitalWrite(statusLedPin, !digitalRead(statusLedPin));
        }

        if (millis() - lastActivityMs > CONFIG_MODE_TIMEOUT_MS) {
            Serial.println("[CONFIG] Timeout - rebooting...");
            ESP.restart();
        }
        delay(5);
    }
}

// ==========================================
// startAlwaysOnServer — non-blocking
// ==========================================

void startAlwaysOnServer() {
    Serial.println("[ALWAYS-ON] Starting persistent web portal...");

    // AP stays running so the device is always reachable for
    // reconfiguration even if the home WiFi SSID/password changes.
    startAP();
    setupWebServer();

    if (MDNS.begin(CONFIG_MDNS_HOST)) {
        MDNS.addService("http", "tcp", 80);
    }

    Serial.printf("[ALWAYS-ON] AP:  http://192.168.4.1  (connect to '%s')\r\n", CONFIG_AP_SSID);
    Serial.printf("[ALWAYS-ON] LAN: http://%s.local\r\n", CONFIG_MDNS_HOST);
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[ALWAYS-ON] IP:  http://%s\r\n",
                      WiFi.localIP().toString().c_str());
    }
}

// ==========================================
// handleAlwaysOnClients — call every loop tick
// ==========================================

void handleAlwaysOnClients() {
    dnsServer.processNextRequest();
    server.handleClient();
}
