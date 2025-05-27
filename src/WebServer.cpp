#include "../include/WebServer.h"

#include "../include/Homing.h"
#include "../include/PickCycle.h"
#include "../include/TransferArm.h"

//* ************************************************************************
//* ************************ WEB SERVER IMPLEMENTATION *******************
//* ************************************************************************

// Global instance definition
TransferArmWebServer webServer;

// Static pointer for WebSocket callback
TransferArmWebServer *webServerInstance = nullptr;

// Constructor
TransferArmWebServer::TransferArmWebServer() : server(80), webSocket("/ws") {
  webServerInstance = this;
  motorsActive = false;  // Initialize motors as inactive
  resetToDefaults();
}

// Initialize web server and WiFi
void TransferArmWebServer::begin() {
  Serial.println("Initializing Web Server...");

  // Initialize SPIFFS for serving web files
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Load configuration from flash
  loadConfig();

  // Apply motor enable state from configuration
  digitalWrite(X_ENABLE_PIN, config.xMotorEnabled ? LOW : HIGH);

  // Setup WiFi - Try to connect to Everwood network first
  if (config.apMode || strlen(config.ssid) == 0) {
    // If no SSID configured, try Everwood network first
    WiFi.begin("Everwood", "Everwood-Staff");
    Serial.print("Connecting to Everwood WiFi");

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("Connected to Everwood! IP: ");
      Serial.println(WiFi.localIP());
      // Update config with successful connection
      strcpy(config.ssid, "Everwood");
      strcpy(config.password, "Everwood-Staff");
      config.apMode = false;
      saveConfig();
    } else {
      Serial.println("\nFailed to connect to Everwood, switching to AP mode");
      config.apMode = true;
      WiFi.softAP("TransferArm", "12345678");
      Serial.print("AP Mode - IP: ");
      Serial.println(WiFi.softAPIP());
    }
  } else {
    // Use configured WiFi credentials
    WiFi.begin(config.ssid, config.password);
    Serial.print("Connecting to configured WiFi: ");
    Serial.println(config.ssid);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("Connected! IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\nFailed to connect, trying Everwood network");
      WiFi.begin("Everwood", "Everwood-Staff");

      attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("Connected to Everwood! IP: ");
        Serial.println(WiFi.localIP());
        // Update config with successful connection
        strcpy(config.ssid, "Everwood");
        strcpy(config.password, "Everwood-Staff");
        config.apMode = false;
        saveConfig();
      } else {
        Serial.println("\nAll WiFi attempts failed, switching to AP mode");
        config.apMode = true;
        WiFi.softAP("TransferArm", "12345678");
        Serial.print("AP Mode - IP: ");
        Serial.println(WiFi.softAPIP());
      }
    }
  }

  // Setup WebSocket
  webSocket.onEvent(onWebSocketEventWrapper);
  server.addHandler(&webSocket);

  // Setup HTTP routes
  server.on("/", HTTP_GET,
            [this](AsyncWebServerRequest *request) { handleRoot(request); });

  server.on("/api", HTTP_POST,
            [this](AsyncWebServerRequest *request) { handleAPI(request); });

  // Serve static files from SPIFFS
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.onNotFound(
      [this](AsyncWebServerRequest *request) { handleNotFound(request); });

  // Start server
  server.begin();
  Serial.println("Web Server Started");
}

// Update method - call from main loop
void TransferArmWebServer::update() {
  // Only perform WebSocket operations when motors are NOT active
  if (!motorsActive) {
    webSocket.cleanupClients();
  }

  // No more time-based polling - all broadcasting is now event-driven
  // Status updates are triggered by:
  // 1. State changes in the pick cycle (only when motors inactive)
  // 2. Movement completion events
  // 3. Manual control actions (only when motors inactive)
  // 4. Configuration changes (only when motors inactive)
}

// Set motor activity state - disables WebSocket when motors are active
void TransferArmWebServer::setMotorsActive(bool active) {
  bool wasActive = motorsActive;
  motorsActive = active;

  if (active && !wasActive) {
    // Motors starting - disable WebSocket operations
    Serial.println("Motors active - WebSocket operations disabled");

    // Send final message before disabling WebSocket
    if (hasConnectedClients()) {
      JsonDocument doc;
      doc["type"] = "motorsActive";
      doc["active"] = true;
      doc["timestamp"] = millis();

      String message;
      serializeJson(doc, message);
      webSocket.textAll(message);
    }
  } else if (!active && wasActive) {
    // Motors stopped - re-enable WebSocket and send status update
    Serial.println("Motors inactive - WebSocket operations enabled");

    if (hasConnectedClients()) {
      // Send motor inactive message
      JsonDocument doc;
      doc["type"] = "motorsActive";
      doc["active"] = false;
      doc["timestamp"] = millis();

      String message;
      serializeJson(doc, message);
      webSocket.textAll(message);

      // Send updated status
      broadcastStatus();
    }
  }
}

// WebSocket event wrapper (static)
void TransferArmWebServer::onWebSocketEventWrapper(AsyncWebSocket *server,
                                                   AsyncWebSocketClient *client,
                                                   AwsEventType type, void *arg,
                                                   uint8_t *data, size_t len) {
  if (webServerInstance) {
    webServerInstance->onWebSocketEvent(server, client, type, arg, data, len);
  }
}

// WebSocket event handler
void TransferArmWebServer::onWebSocketEvent(AsyncWebSocket *server,
                                            AsyncWebSocketClient *client,
                                            AwsEventType type, void *arg,
                                            uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(),
                    client->remoteIP().toString().c_str());
      // Only send initial config and status if motors are not active
      if (!motorsActive) {
        sendConfigToClient(client->id());
        broadcastStatus();
      } else {
        // Send a message indicating motors are active
        JsonDocument doc;
        doc["type"] = "log";
        doc["message"] =
            "Motors are currently active - WebSocket operations disabled";
        String message;
        serializeJson(doc, message);
        webSocket.text(client->id(), message);
      }
      break;

    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;

    case WS_EVT_DATA: {
      // Reject all commands if motors are active
      if (motorsActive) {
        JsonDocument logDoc;
        logDoc["type"] = "log";
        logDoc["message"] = "Command rejected - motors are currently active";
        String logMessage;
        serializeJson(logDoc, logMessage);
        webSocket.text(client->id(), logMessage);
        return;
      }

      AwsFrameInfo *info = (AwsFrameInfo *)arg;
      if (info->final && info->index == 0 && info->len == len &&
          info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = String((char *)data);
        Serial.printf("WebSocket [%u] received: %s\n", client->id(),
                      message.c_str());

        // Parse JSON message
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);

        if (error) {
          Serial.println("JSON parse failed");
          return;
        }

        String command = doc["command"];

        if (command == "getStatus") {
          handleGetStatus();
        } else if (command == "getConfig") {
          handleGetConfig();
        } else if (command == "setConfig") {
          handleSetConfig(doc);
        } else if (command == "manualControl") {
          handleManualControl(doc);
        } else if (command == "emergencyStop") {
          handleEmergencyStop();
        }
      }
    } break;

    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

// HTTP Handlers
void TransferArmWebServer::handleRoot(AsyncWebServerRequest *request) {
  request->send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Transfer Arm Control Dashboard</title>
    <style>
        :root {
            --primary: #2563eb;
            --primary-dark: #1d4ed8;
            --secondary: #64748b;
            --success: #10b981;
            --warning: #f59e0b;
            --danger: #ef4444;
            --background: #f8fafc;
            --surface: #ffffff;
            --text: #1e293b;
            --text-light: #64748b;
        }
        
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: var(--background);
            color: var(--text);
            line-height: 1.6;
        }
        
        .container {
            max-width: 1400px;
            margin: 0 auto;
            padding: 2rem;
        }
        
        .header {
            text-align: center;
            margin-bottom: 3rem;
        }
        
        .header h1 {
            font-size: 2.5rem;
            font-weight: 700;
            margin-bottom: 0.5rem;
        }
        
        .status-badge {
            display: inline-block;
            padding: 0.5rem 1rem;
            border-radius: 9999px;
            font-size: 0.875rem;
            font-weight: 500;
            margin-top: 1rem;
        }
        
        .status-waiting { background: var(--secondary); color: white; }
        .status-running { background: var(--primary); color: white; }
        .status-error { background: var(--danger); color: white; }
        
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 2rem;
            margin-bottom: 2rem;
        }
        
        .card {
            background: var(--surface);
            border-radius: 1rem;
            box-shadow: 0 1px 3px rgba(0, 0, 0, 0.1);
            padding: 1.5rem;
            border: 1px solid #e2e8f0;
            overflow: hidden; /* Prevent content overflow */
            word-wrap: break-word;
        }
        
        .card-title {
            font-size: 1.25rem;
            font-weight: 600;
            margin-bottom: 1rem;
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }
        
        .btn {
            background: var(--primary);
            color: white;
            border: none;
            padding: 0.75rem 1.5rem;
            border-radius: 0.5rem;
            font-size: 0.875rem;
            font-weight: 500;
            cursor: pointer;
            transition: background 0.2s;
            margin: 0.25rem;
        }
        
        .btn:hover { background: var(--primary-dark); }
        .btn-danger { background: var(--danger); }
        .btn-danger:hover { background: #dc2626; }
        .btn-success { background: var(--success); }
        .btn-success:hover { background: #059669; }
        .btn-warning { background: var(--warning); }
        .btn-warning:hover { background: #d97706; }
        
        .input-group {
            margin-bottom: 1rem;
        }
        
        .input-group label {
            display: block;
            font-weight: 500;
            margin-bottom: 0.25rem;
            color: var(--text);
        }
        
        .input-group input, .input-group select {
            width: 100%;
            padding: 0.75rem;
            border: 1px solid #d1d5db;
            border-radius: 0.5rem;
            font-size: 0.875rem;
        }
        
        .status-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 1rem;
        }
        
        @media (max-width: 1200px) {
            .status-grid {
                grid-template-columns: 1fr;
            }
        }
        
        .status-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 0.75rem;
            background: #f8fafc;
            border-radius: 0.5rem;
            min-width: 0; /* Prevent flex items from overflowing */
        }
        
        .status-item span:first-child {
            font-weight: 500;
            margin-right: 1rem;
            flex-shrink: 0;
        }
        
        .status-item span:last-child {
            text-align: right;
            word-break: break-word;
        }
        
        .position-display {
            font-family: 'Courier New', monospace;
            font-weight: bold;
            color: var(--primary);
        }
        
        .system-status-card {
            min-width: 400px;
        }
        
        @media (max-width: 1400px) {
            .system-status-card {
                min-width: 350px;
            }
        }
        
        .emergency-stop {
            position: fixed;
            top: 2rem;
            right: 2rem;
            z-index: 1000;
        }
        
        .log-container {
            background: #1e293b;
            color: #e2e8f0;
            border-radius: 0.5rem;
            padding: 1rem;
            height: 200px;
            overflow-y: auto;
            font-family: 'Courier New', monospace;
            font-size: 0.875rem;
            margin-top: 1rem;
        }
        
        .connection-status {
            position: fixed;
            bottom: 2rem;
            right: 2rem;
            padding: 0.5rem 1rem;
            border-radius: 0.5rem;
            font-size: 0.875rem;
            font-weight: 500;
        }
        
        .connected { background: var(--success); color: white; }
        .disconnected { background: var(--danger); color: white; }
        
        @media (max-width: 768px) {
            .container { padding: 1rem; }
            .grid { 
                grid-template-columns: 1fr; 
                gap: 1rem;
            }
            .emergency-stop { position: static; margin-bottom: 2rem; }
            .status-grid {
                grid-template-columns: 1fr;
                gap: 0.5rem;
            }
        }
    </style>
</head>
<body>
    <div class="emergency-stop">
        <button class="btn btn-danger" onclick="emergencyStop()">🛑 EMERGENCY STOP</button>
    </div>
    
    <div class="container">
        <div class="header">
            <h1>Transfer Arm Control Dashboard</h1>
            <div class="status-badge" id="systemStatus">Connecting...</div>
        </div>
        
        <div class="grid">
            <!-- System Status -->
            <div class="card system-status-card">
                <div class="card-title">📊 System Status</div>
                <div class="status-grid">
                    <div class="status-item">
                        <span>State Machine:</span>
                        <span id="currentState">-</span>
                    </div>
                    <div class="status-item">
                        <span>X Position:</span>
                        <span class="position-display" id="xPosition">-</span>
                    </div>
                    <div class="status-item">
                        <span>Z Position:</span>
                        <span class="position-display" id="zPosition">-</span>
                    </div>
                    <div class="status-item">
                        <span>Servo Angle:</span>
                        <span class="position-display" id="servoPosition">-</span>
                    </div>
                    <div class="status-item">
                        <span>Vacuum:</span>
                        <span id="vacuumStatus">-</span>
                    </div>
                    <div class="status-item">
                        <span>Home Switches:</span>
                        <span id="homeSwitches">-</span>
                    </div>
                </div>
            </div>
            
            <!-- Manual Control -->
            <div class="card">
                <div class="card-title">🎮 Manual Control</div>
                <button class="btn" onclick="triggerHoming()">🏠 Home System</button>
                <button class="btn" onclick="triggerPickCycle()">🔄 Start Pick Cycle</button>
                <button class="btn btn-success" onclick="toggleVacuum()">💨 Toggle Vacuum</button>
                
                <div style="margin-top: 1rem;">
                    <div class="input-group">
                        <label>Move X Axis (inches):</label>
                        <input type="number" id="xTarget" step="0.1" placeholder="0.0">
                        <button class="btn" onclick="moveXAxis()" style="margin-top: 0.5rem;">Move X</button>
                    </div>
                    
                    <div class="input-group">
                        <label>Move Z Axis (inches):</label>
                        <input type="number" id="zTarget" step="0.1" placeholder="0.0">
                        <button class="btn" onclick="moveZAxis()" style="margin-top: 0.5rem;">Move Z</button>
                    </div>
                    
                    <div class="input-group">
                        <label>Servo Angle (degrees):</label>
                        <input type="number" id="servoTarget" min="0" max="180" placeholder="90">
                        <button class="btn" onclick="moveServo()" style="margin-top: 0.5rem;">Move Servo</button>
                    </div>
                </div>
            </div>
            
            <!-- Position Settings -->
            <div class="card">
                <div class="card-title">📍 Position Settings</div>
                <div class="input-group">
                    <label>X Pickup Position (inches):</label>
                    <input type="number" id="xPickupPos" step="0.1">
                </div>
                <div class="input-group">
                    <label>X Dropoff Position (inches):</label>
                    <input type="number" id="xDropoffPos" step="0.1">
                </div>
                <div class="input-group">
                    <label>Z Pickup Lower (inches):</label>
                    <input type="number" id="zPickupLower" step="0.1">
                </div>
                <div class="input-group">
                    <label>Z Dropoff Lower (inches):</label>
                    <input type="number" id="zDropoffLower" step="0.1">
                </div>
                <button class="btn" onclick="savePositions()">💾 Save Positions</button>
            </div>
            
            <!-- Speed Settings -->
            <div class="card">
                <div class="card-title">⚡ Speed Settings</div>
                <div class="input-group">
                    <label>X Max Speed (steps/sec):</label>
                    <input type="number" id="xMaxSpeed">
                </div>
                <div class="input-group">
                    <label>X Acceleration (steps/sec²):</label>
                    <input type="number" id="xAcceleration">
                </div>
                <div class="input-group">
                    <label>Z Max Speed (steps/sec):</label>
                    <input type="number" id="zMaxSpeed">
                </div>
                <div class="input-group">
                    <label>Z Acceleration (steps/sec²):</label>
                    <input type="number" id="zAcceleration">
                </div>
                <button class="btn" onclick="saveSpeeds()">💾 Save Speeds</button>
                <button class="btn btn-warning" onclick="resetAllSettings()" style="background: var(--warning); margin-top: 0.5rem;">🔄 Reset All to Defaults</button>
            </div>
            
            <!-- Motor Control -->
            <div class="card">
                <div class="card-title">🔧 Motor Control</div>
                <div class="status-item">
                    <span>X-Axis Motor:</span>
                    <span id="xMotorStatus">Enabled</span>
                </div>
                <button class="btn" id="xMotorToggle" onclick="toggleXMotor()">🔌 Toggle X Motor</button>
                <div style="margin-top: 1rem; padding: 1rem; background: #fef3c7; border-radius: 0.5rem; border-left: 4px solid var(--warning);">
                    <strong>⚠️ Warning:</strong> Disabling the X motor will prevent all X-axis movements. Only disable when maintenance is required.
                </div>
            </div>
            
            <!-- Servo Settings -->
            <div class="card">
                <div class="card-title">🔧 Servo Settings</div>
                <div class="input-group">
                    <label>Pickup Position (degrees):</label>
                    <input type="number" id="servoPickup" min="0" max="180">
                </div>
                <div class="input-group">
                    <label>Travel Position (degrees):</label>
                    <input type="number" id="servoTravel" min="0" max="180">
                </div>
                <div class="input-group">
                    <label>Dropoff Position (degrees):</label>
                    <input type="number" id="servoDropoff" min="0" max="180">
                </div>
                <button class="btn" onclick="saveServoSettings()">💾 Save Servo Settings</button>
            </div>
            
            <!-- Timing Settings -->
            <div class="card">
                <div class="card-title">⏱️ Timing Settings</div>
                <div class="input-group">
                    <label>Pickup Hold Time (ms):</label>
                    <input type="number" id="pickupHoldTime">
                </div>
                <div class="input-group">
                    <label>Dropoff Hold Time (ms):</label>
                    <input type="number" id="dropoffHoldTime">
                </div>
                <div class="input-group">
                    <label>Servo Rotation Wait (ms):</label>
                    <input type="number" id="servoRotationWait">
                </div>
                <button class="btn" onclick="saveTimingSettings()">💾 Save Timing</button>
            </div>
        </div>
        
        <!-- System Log -->
        <div class="card">
            <div class="card-title">📝 System Log</div>
            <div class="log-container" id="systemLog"></div>
        </div>
    </div>
    
    <div class="connection-status" id="connectionStatus">Connecting...</div>
    
    <script>
        let ws;
        let vacuumState = false;
        
        function connectWebSocket() {
            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
            ws = new WebSocket(`${protocol}//${window.location.hostname}/ws`);
            
            ws.onopen = function() {
                document.getElementById('connectionStatus').textContent = 'Connected';
                document.getElementById('connectionStatus').className = 'connection-status connected';
                log('WebSocket connected');
                requestStatus();
                requestConfig();
            };
            
            ws.onmessage = function(event) {
                const data = JSON.parse(event.data);
                handleMessage(data);
            };
            
            ws.onclose = function() {
                document.getElementById('connectionStatus').textContent = 'Disconnected';
                document.getElementById('connectionStatus').className = 'connection-status disconnected';
                log('WebSocket disconnected');
                setTimeout(connectWebSocket, 3000);
            };
            
            ws.onerror = function(error) {
                log('WebSocket error: ' + error);
            };
        }
        
        function handleMessage(data) {
            if (data.type === 'status') {
                updateStatus(data);
            } else if (data.type === 'config') {
                updateConfigUI(data.config);
            } else if (data.type === 'log') {
                log(data.message);
            } else if (data.type === 'stateChange') {
                log(`State changed to: ${data.state}`);
                // Request a full status update when state changes
                requestStatus();
            } else if (data.type === 'vacuumChange') {
                log(`Vacuum ${data.vacuum ? 'activated' : 'deactivated'}`);
                document.getElementById('vacuumStatus').textContent = data.vacuum ? 'ON' : 'OFF';
            } else if (data.type === 'servoChange') {
                log(`Servo moved to ${data.servoPos}°`);
                document.getElementById('servoPosition').textContent = data.servoPos + '°';
            } else if (data.type === 'motorsActive') {
                if (data.active) {
                    log('Motors active - WebSocket operations disabled');
                    showMotorActivityIndicator(true);
                } else {
                    log('Motors inactive - WebSocket operations enabled');
                    showMotorActivityIndicator(false);
                    // Request status update when motors become inactive
                    requestStatus();
                }
            }
        }
        
        function updateStatus(data) {
            document.getElementById('currentState').textContent = data.state || '-';
            
            // Update position display based on movement status
            if (data.motorsMoving) {
                document.getElementById('xPosition').textContent = `→ ${data.xTarget || 0} steps (moving)`;
                document.getElementById('zPosition').textContent = `→ ${data.zTarget || 0} steps (moving)`;
            } else {
                document.getElementById('xPosition').textContent = (data.xPos || 0) + ' steps';
                document.getElementById('zPosition').textContent = (data.zPos || 0) + ' steps';
            }
            
            document.getElementById('servoPosition').textContent = (data.servoPos || 0) + '°';
            document.getElementById('vacuumStatus').textContent = data.vacuum ? 'ON' : 'OFF';
            document.getElementById('homeSwitches').textContent = 
                `X:${data.xHome ? 'ON' : 'OFF'} Z:${data.zHome ? 'ON' : 'OFF'}`;
            
            const statusElement = document.getElementById('systemStatus');
            if (data.motorsMoving) {
                statusElement.textContent = 'Moving';
                statusElement.className = 'status-badge status-running';
            } else if (data.state === 'WAITING') {
                statusElement.textContent = 'Ready';
                statusElement.className = 'status-badge status-waiting';
            } else {
                statusElement.textContent = 'Running';
                statusElement.className = 'status-badge status-running';
            }
        }
        
        function updateConfigUI(config) {
            document.getElementById('xPickupPos').value = config.xPickupPosInches;
            document.getElementById('xDropoffPos').value = config.xDropoffPosInches;
            document.getElementById('zPickupLower').value = config.zPickupLowerInches;
            document.getElementById('zDropoffLower').value = config.zDropoffLowerInches;
            
            document.getElementById('xMaxSpeed').value = config.xMaxSpeed;
            document.getElementById('xAcceleration').value = config.xAcceleration;
            document.getElementById('zMaxSpeed').value = config.zMaxSpeed;
            document.getElementById('zAcceleration').value = config.zAcceleration;
            
            document.getElementById('servoPickup').value = config.servoPickupPos;
            document.getElementById('servoTravel').value = config.servoTravelPos;
            document.getElementById('servoDropoff').value = config.servoDropoffPos;
            
            document.getElementById('pickupHoldTime').value = config.pickupHoldTime;
            document.getElementById('dropoffHoldTime').value = config.dropoffHoldTime;
            document.getElementById('servoRotationWait').value = config.servoRotationWaitTime;
            
            // Update motor status
            const xMotorEnabled = config.xMotorEnabled !== undefined ? config.xMotorEnabled : true;
            document.getElementById('xMotorStatus').textContent = xMotorEnabled ? 'Enabled' : 'Disabled';
            document.getElementById('xMotorToggle').textContent = xMotorEnabled ? '🔌 Disable X Motor' : '🔌 Enable X Motor';
            document.getElementById('xMotorToggle').className = xMotorEnabled ? 'btn btn-warning' : 'btn btn-success';
        }
        
        function sendCommand(command, data = {}) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({command, ...data}));
            }
        }
        
        function requestStatus() { sendCommand('getStatus'); }
        function requestConfig() { sendCommand('getConfig'); }
        
        function emergencyStop() {
            sendCommand('emergencyStop');
            log('EMERGENCY STOP ACTIVATED');
        }
        
        function triggerHoming() {
            sendCommand('manualControl', {action: 'home'});
            log('Homing sequence triggered');
        }
        
        function triggerPickCycle() {
            sendCommand('manualControl', {action: 'pickCycle'});
            log('Pick cycle triggered');
        }
        
        function toggleVacuum() {
            vacuumState = !vacuumState;
            sendCommand('manualControl', {action: 'vacuum', state: vacuumState});
            log(`Vacuum ${vacuumState ? 'activated' : 'deactivated'}`);
        }
        
        function moveXAxis() {
            const target = parseFloat(document.getElementById('xTarget').value);
            if (!isNaN(target)) {
                sendCommand('manualControl', {action: 'moveX', target});
                log(`Moving X axis to ${target} inches`);
            }
        }
        
        function moveZAxis() {
            const target = parseFloat(document.getElementById('zTarget').value);
            if (!isNaN(target)) {
                sendCommand('manualControl', {action: 'moveZ', target});
                log(`Moving Z axis to ${target} inches`);
            }
        }
        
        function moveServo() {
            const target = parseInt(document.getElementById('servoTarget').value);
            if (!isNaN(target)) {
                sendCommand('manualControl', {action: 'servo', angle: target});
                log(`Moving servo to ${target} degrees`);
            }
        }
        
        function savePositions() {
            const config = {
                xPickupPosInches: parseFloat(document.getElementById('xPickupPos').value),
                xDropoffPosInches: parseFloat(document.getElementById('xDropoffPos').value),
                zPickupLowerInches: parseFloat(document.getElementById('zPickupLower').value),
                zDropoffLowerInches: parseFloat(document.getElementById('zDropoffLower').value)
            };
            sendCommand('setConfig', {config});
            log('Position settings saved');
        }
        
        function saveSpeeds() {
            const config = {
                xMaxSpeed: parseInt(document.getElementById('xMaxSpeed').value),
                xAcceleration: parseInt(document.getElementById('xAcceleration').value),
                zMaxSpeed: parseInt(document.getElementById('zMaxSpeed').value),
                zAcceleration: parseInt(document.getElementById('zAcceleration').value)
            };
            sendCommand('setConfig', {config});
            log('Speed settings saved');
        }
        
        function saveServoSettings() {
            const config = {
                servoPickupPos: parseInt(document.getElementById('servoPickup').value),
                servoTravelPos: parseInt(document.getElementById('servoTravel').value),
                servoDropoffPos: parseInt(document.getElementById('servoDropoff').value)
            };
            sendCommand('setConfig', {config});
            log('Servo settings saved');
        }
        
        function saveTimingSettings() {
            const config = {
                pickupHoldTime: parseInt(document.getElementById('pickupHoldTime').value),
                dropoffHoldTime: parseInt(document.getElementById('dropoffHoldTime').value),
                servoRotationWaitTime: parseInt(document.getElementById('servoRotationWait').value)
            };
            sendCommand('setConfig', {config});
            log('Timing settings saved');
        }
        
        function resetAllSettings() {
            if (confirm('Are you sure you want to reset ALL settings to factory defaults? This cannot be undone.')) {
                sendCommand('manualControl', {action: 'resetToDefaults'});
                log('All settings reset to factory defaults');
            }
        }
        
        function toggleXMotor() {
            sendCommand('manualControl', {action: 'toggleXMotor'});
            log('X-axis motor toggle requested');
        }
        
        function log(message) {
            const logContainer = document.getElementById('systemLog');
            const timestamp = new Date().toLocaleTimeString();
            logContainer.innerHTML += `<div>[${timestamp}] ${message}</div>`;
            logContainer.scrollTop = logContainer.scrollHeight;
        }
        
        function showMotorActivityIndicator(active) {
            let indicator = document.getElementById('motorActivityIndicator');
            if (!indicator) {
                // Create indicator if it doesn't exist
                indicator = document.createElement('div');
                indicator.id = 'motorActivityIndicator';
                indicator.style.cssText = `
                    position: fixed;
                    top: 50%;
                    left: 50%;
                    transform: translate(-50%, -50%);
                    background: rgba(239, 68, 68, 0.95);
                    color: white;
                    padding: 2rem;
                    border-radius: 1rem;
                    font-size: 1.25rem;
                    font-weight: bold;
                    text-align: center;
                    z-index: 10000;
                    box-shadow: 0 10px 25px rgba(0, 0, 0, 0.3);
                    display: none;
                `;
                indicator.innerHTML = `
                    <div>⚡ MOTORS ACTIVE ⚡</div>
                    <div style="font-size: 0.875rem; margin-top: 0.5rem; opacity: 0.9;">
                        WebSocket operations disabled
                    </div>
                `;
                document.body.appendChild(indicator);
            }
            
            indicator.style.display = active ? 'block' : 'none';
        }
        
        // Initialize
        connectWebSocket();
        // No more polling - status updates are now event-driven
    </script>
</body>
</html>
)rawliteral");
}

void TransferArmWebServer::handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "File not found");
}

void TransferArmWebServer::handleAPI(AsyncWebServerRequest *request) {
  request->send(200, "application/json", "{\"status\":\"ok\"}");
}

// Message Handlers
void TransferArmWebServer::handleGetStatus() {
  JsonDocument doc;
  doc["type"] = "status";
  doc["state"] = getStateString(getCurrentState());

  // Only send position data if motors are not moving
  bool motorsMoving = isMovementInProgress();
  doc["motorsMoving"] = motorsMoving;

  if (!motorsMoving) {
    doc["xPos"] = transferArm.getXStepper().currentPosition();
    doc["zPos"] = transferArm.getZStepper().currentPosition();
  } else {
    // Send target positions instead during movement
    doc["xTarget"] = transferArm.getXStepper().targetPosition();
    doc["zTarget"] = transferArm.getZStepper().targetPosition();
  }

  doc["servoPos"] = transferArm.getCurrentServoPosition();
  doc["vacuum"] = digitalRead(SOLENOID_RELAY_PIN);
  doc["xHome"] = transferArm.getXHomeSwitch().read();
  doc["zHome"] = transferArm.getZHomeSwitch().read();

  String message;
  serializeJson(doc, message);
  webSocket.textAll(message);
}

void TransferArmWebServer::handleGetConfig() { sendConfigToClient(); }

void TransferArmWebServer::handleSetConfig(JsonDocument &doc) {
  JsonObject configObj = doc["config"];

  if (configObj["xPickupPosInches"].is<float>()) {
    config.xPickupPosInches = configObj["xPickupPosInches"];
  }
  if (configObj["xDropoffPosInches"].is<float>()) {
    config.xDropoffPosInches = configObj["xDropoffPosInches"];
  }
  if (configObj["zPickupLowerInches"].is<float>()) {
    config.zPickupLowerInches = configObj["zPickupLowerInches"];
  }
  if (configObj["zDropoffLowerInches"].is<float>()) {
    config.zDropoffLowerInches = configObj["zDropoffLowerInches"];
  }
  if (configObj["servoPickupPos"].is<int>()) {
    config.servoPickupPos = configObj["servoPickupPos"];
  }
  if (configObj["servoTravelPos"].is<int>()) {
    config.servoTravelPos = configObj["servoTravelPos"];
  }
  if (configObj["servoDropoffPos"].is<int>()) {
    config.servoDropoffPos = configObj["servoDropoffPos"];
  }
  if (configObj["pickupHoldTime"].is<unsigned long>()) {
    config.pickupHoldTime = configObj["pickupHoldTime"];
  }
  if (configObj["dropoffHoldTime"].is<unsigned long>()) {
    config.dropoffHoldTime = configObj["dropoffHoldTime"];
  }
  if (configObj["servoRotationWaitTime"].is<unsigned long>()) {
    config.servoRotationWaitTime = configObj["servoRotationWaitTime"];
  }
  if (configObj["xMaxSpeed"].is<int>()) {
    config.xMaxSpeed = configObj["xMaxSpeed"];
    transferArm.getXStepper().setMaxSpeed(config.xMaxSpeed);
  }
  if (configObj["xAcceleration"].is<int>()) {
    config.xAcceleration = configObj["xAcceleration"];
    transferArm.getXStepper().setAcceleration(config.xAcceleration);
  }
  if (configObj["zMaxSpeed"].is<int>()) {
    config.zMaxSpeed = configObj["zMaxSpeed"];
    transferArm.getZStepper().setMaxSpeed(config.zMaxSpeed);
  }
  if (configObj["zAcceleration"].is<int>()) {
    config.zAcceleration = configObj["zAcceleration"];
    transferArm.getZStepper().setAcceleration(config.zAcceleration);
  }
  if (configObj["xMotorEnabled"].is<bool>()) {
    config.xMotorEnabled = configObj["xMotorEnabled"];
    digitalWrite(X_ENABLE_PIN, config.xMotorEnabled ? LOW : HIGH);
  }

  saveConfig();
  sendConfigToClient();

  // Send log message
  JsonDocument logDoc;
  logDoc["type"] = "log";
  logDoc["message"] = "Configuration updated";
  String logMessage;
  serializeJson(logDoc, logMessage);
  webSocket.textAll(logMessage);
}

void TransferArmWebServer::handleManualControl(JsonDocument &doc) {
  String action = doc["action"];

  // Check if motors are currently moving (except for emergency actions)
  if (isMovementInProgress() && action != "emergencyStop") {
    JsonDocument logDoc;
    logDoc["type"] = "log";
    logDoc["message"] = "Command ignored - motors are currently moving";
    String logMessage;
    serializeJson(logDoc, logMessage);
    webSocket.textAll(logMessage);
    return;
  }

  if (action == "home") {
    triggerHoming();
  } else if (action == "pickCycle") {
    triggerPickCycle();
  } else if (action == "vacuum") {
    bool state = doc["state"];
    activateVacuum(state);
    broadcastVacuumChange(state);
  } else if (action == "moveX") {
    float target = doc["target"];
    long steps = target * STEPS_PER_INCH;
    // Disable WebSocket during manual movement
    setMotorsActive(true);
    moveToPosition('X', steps);
  } else if (action == "moveZ") {
    float target = doc["target"];
    long steps = target * STEPS_PER_INCH;
    // Disable WebSocket during manual movement
    setMotorsActive(true);
    moveToPosition('Z', steps);
  } else if (action == "servo") {
    int angle = doc["angle"];
    setServoPosition(angle);
    broadcastServoChange(angle);
  } else if (action == "resetToDefaults") {
    resetToDefaults();
    saveConfig();
    sendConfigToClient();

    // Apply the reset values to the motors immediately
    transferArm.getXStepper().setMaxSpeed(config.xMaxSpeed);
    transferArm.getXStepper().setAcceleration(config.xAcceleration);
    transferArm.getZStepper().setMaxSpeed(config.zMaxSpeed);
    transferArm.getZStepper().setAcceleration(config.zAcceleration);

    // Send log message
    JsonDocument logDoc;
    logDoc["type"] = "log";
    logDoc["message"] = "All settings reset to factory defaults and applied";
    String logMessage;
    serializeJson(logDoc, logMessage);
    webSocket.textAll(logMessage);
  } else if (action == "toggleXMotor") {
    toggleXMotorEnable();
  }
}

void TransferArmWebServer::handleEmergencyStop() {
  // Stop all motors immediately
  transferArm.getXStepper().stop();
  transferArm.getZStepper().stop();

  // Turn off vacuum
  digitalWrite(SOLENOID_RELAY_PIN, LOW);

  // Force state to WAITING
  forceState(WAITING);

  // Send log message
  JsonDocument logDoc;
  logDoc["type"] = "log";
  logDoc["message"] = "EMERGENCY STOP ACTIVATED - All systems halted";
  String logMessage;
  serializeJson(logDoc, logMessage);
  webSocket.textAll(logMessage);
}

// Utility Methods
void TransferArmWebServer::broadcastStatus() {
  // Only broadcast if motors are not active
  if (!motorsActive) {
    handleGetStatus();
  }
}

void TransferArmWebServer::broadcastStateChange(PickCycleState newState) {
  // Only broadcast if we have connected clients AND motors are not active
  if (!hasConnectedClients() || motorsActive) return;

  JsonDocument doc;
  doc["type"] = "stateChange";
  doc["state"] = getStateString(newState);
  doc["timestamp"] = millis();

  String message;
  serializeJson(doc, message);
  webSocket.textAll(message);

  // Also send a full status update
  broadcastStatus();
}

void TransferArmWebServer::broadcastVacuumChange(bool vacuumState) {
  // Only broadcast if we have connected clients AND motors are not active
  if (!hasConnectedClients() || motorsActive) return;

  JsonDocument doc;
  doc["type"] = "vacuumChange";
  doc["vacuum"] = vacuumState;
  doc["timestamp"] = millis();

  String message;
  serializeJson(doc, message);
  webSocket.textAll(message);
}

void TransferArmWebServer::broadcastServoChange(int servoPosition) {
  // Only broadcast if we have connected clients AND motors are not active
  if (!hasConnectedClients() || motorsActive) return;

  JsonDocument doc;
  doc["type"] = "servoChange";
  doc["servoPos"] = servoPosition;
  doc["timestamp"] = millis();

  String message;
  serializeJson(doc, message);
  webSocket.textAll(message);
}

void TransferArmWebServer::sendConfigToClient(uint32_t clientId) {
  JsonDocument doc;
  doc["type"] = "config";
  JsonObject configObj = doc["config"].to<JsonObject>();

  configObj["xPickupPosInches"] = config.xPickupPosInches;
  configObj["xDropoffPosInches"] = config.xDropoffPosInches;
  configObj["zPickupLowerInches"] = config.zPickupLowerInches;
  configObj["zDropoffLowerInches"] = config.zDropoffLowerInches;
  configObj["zSuctionStartInches"] = config.zSuctionStartInches;

  configObj["servoPickupPos"] = config.servoPickupPos;
  configObj["servoTravelPos"] = config.servoTravelPos;
  configObj["servoDropoffPos"] = config.servoDropoffPos;

  configObj["pickupHoldTime"] = config.pickupHoldTime;
  configObj["dropoffHoldTime"] = config.dropoffHoldTime;
  configObj["servoRotationWaitTime"] = config.servoRotationWaitTime;

  configObj["xMaxSpeed"] = config.xMaxSpeed;
  configObj["xAcceleration"] = config.xAcceleration;
  configObj["zMaxSpeed"] = config.zMaxSpeed;
  configObj["zAcceleration"] = config.zAcceleration;
  configObj["zDropoffMaxSpeed"] = config.zDropoffMaxSpeed;
  configObj["zDropoffAcceleration"] = config.zDropoffAcceleration;
  configObj["xHomeSpeed"] = config.xHomeSpeed;
  configObj["zHomeSpeed"] = config.zHomeSpeed;
  configObj["xMotorEnabled"] = config.xMotorEnabled;

  String message;
  serializeJson(doc, message);

  if (clientId == 0) {
    webSocket.textAll(message);
  } else {
    webSocket.text(clientId, message);
  }
}

// Configuration Management
void TransferArmWebServer::loadConfig() {
  preferences.begin("transferarm", false);

  config.xPickupPosInches =
      preferences.getFloat("xPickupPos", X_PICKUP_POS_INCHES);
  config.xDropoffPosInches =
      preferences.getFloat("xDropoffPos", X_DROPOFF_POS_INCHES);
  config.zPickupLowerInches =
      preferences.getFloat("zPickupLower", Z_PICKUP_LOWER_INCHES);
  config.zDropoffLowerInches =
      preferences.getFloat("zDropoffLower", Z_DROPOFF_LOWER_INCHES);
  config.zSuctionStartInches =
      preferences.getFloat("zSuctionStart", Z_SUCTION_START_INCHES);

  config.servoPickupPos = preferences.getInt("servoPickup", SERVO_PICKUP_POS);
  config.servoTravelPos = preferences.getInt("servoTravel", SERVO_TRAVEL_POS);
  config.servoDropoffPos =
      preferences.getInt("servoDropoff", SERVO_DROPOFF_POS);

  config.pickupHoldTime = preferences.getULong("pickupHold", PICKUP_HOLD_TIME);
  config.dropoffHoldTime =
      preferences.getULong("dropoffHold", DROPOFF_HOLD_TIME);
  config.servoRotationWaitTime =
      preferences.getULong("servoWait", SERVO_ROTATION_WAIT_TIME);

  config.xMaxSpeed = preferences.getInt("xMaxSpeed", X_MAX_SPEED);
  config.xAcceleration = preferences.getInt("xAccel", X_ACCELERATION);
  config.zMaxSpeed = preferences.getInt("zMaxSpeed", Z_MAX_SPEED);
  config.zAcceleration = preferences.getInt("zAccel", Z_ACCELERATION);
  config.zDropoffMaxSpeed =
      preferences.getInt("zDropoffSpeed", Z_DROPOFF_MAX_SPEED);
  config.zDropoffAcceleration =
      preferences.getInt("zDropoffAccel", Z_DROPOFF_ACCELERATION);
  config.xHomeSpeed = preferences.getInt("xHomeSpeed", X_HOME_SPEED);
  config.zHomeSpeed = preferences.getInt("zHomeSpeed", Z_HOME_SPEED);
  config.xMotorEnabled = preferences.getBool("xMotorEnabled", true);

  preferences.getString("ssid", config.ssid, sizeof(config.ssid));
  preferences.getString("password", config.password, sizeof(config.password));
  config.apMode = preferences.getBool("apMode", true);

  preferences.end();
}

void TransferArmWebServer::saveConfig() {
  preferences.begin("transferarm", false);

  preferences.putFloat("xPickupPos", config.xPickupPosInches);
  preferences.putFloat("xDropoffPos", config.xDropoffPosInches);
  preferences.putFloat("zPickupLower", config.zPickupLowerInches);
  preferences.putFloat("zDropoffLower", config.zDropoffLowerInches);
  preferences.putFloat("zSuctionStart", config.zSuctionStartInches);

  preferences.putInt("servoPickup", config.servoPickupPos);
  preferences.putInt("servoTravel", config.servoTravelPos);
  preferences.putInt("servoDropoff", config.servoDropoffPos);

  preferences.putULong("pickupHold", config.pickupHoldTime);
  preferences.putULong("dropoffHold", config.dropoffHoldTime);
  preferences.putULong("servoWait", config.servoRotationWaitTime);

  preferences.putInt("xMaxSpeed", config.xMaxSpeed);
  preferences.putInt("xAccel", config.xAcceleration);
  preferences.putInt("zMaxSpeed", config.zMaxSpeed);
  preferences.putInt("zAccel", config.zAcceleration);
  preferences.putInt("zDropoffSpeed", config.zDropoffMaxSpeed);
  preferences.putInt("zDropoffAccel", config.zDropoffAcceleration);
  preferences.putInt("xHomeSpeed", config.xHomeSpeed);
  preferences.putInt("zHomeSpeed", config.zHomeSpeed);
  preferences.putBool("xMotorEnabled", config.xMotorEnabled);

  preferences.putString("ssid", config.ssid);
  preferences.putString("password", config.password);
  preferences.putBool("apMode", config.apMode);

  preferences.end();
}

void TransferArmWebServer::resetToDefaults() {
  config.xPickupPosInches = X_PICKUP_POS_INCHES;
  config.xDropoffPosInches = X_DROPOFF_POS_INCHES;
  config.zPickupLowerInches = Z_PICKUP_LOWER_INCHES;
  config.zDropoffLowerInches = Z_DROPOFF_LOWER_INCHES;
  config.zSuctionStartInches = Z_SUCTION_START_INCHES;

  config.servoPickupPos = SERVO_PICKUP_POS;
  config.servoTravelPos = SERVO_TRAVEL_POS;
  config.servoDropoffPos = SERVO_DROPOFF_POS;

  config.pickupHoldTime = PICKUP_HOLD_TIME;
  config.dropoffHoldTime = DROPOFF_HOLD_TIME;
  config.servoRotationWaitTime = SERVO_ROTATION_WAIT_TIME;

  config.xMaxSpeed = X_MAX_SPEED;
  config.xAcceleration = X_ACCELERATION;
  config.zMaxSpeed = Z_MAX_SPEED;
  config.zAcceleration = Z_ACCELERATION;
  config.zDropoffMaxSpeed = Z_DROPOFF_MAX_SPEED;
  config.zDropoffAcceleration = Z_DROPOFF_ACCELERATION;
  config.xHomeSpeed = X_HOME_SPEED;
  config.zHomeSpeed = Z_HOME_SPEED;
  config.xMotorEnabled = true;

  strcpy(config.ssid, "Everwood");
  strcpy(config.password, "Everwood-Staff");
  config.apMode = false;
}

// Manual Control Methods
void TransferArmWebServer::triggerHoming() { homeSystem(); }

void TransferArmWebServer::triggerPickCycle() { triggerPickCycleFromWeb(); }

void TransferArmWebServer::moveToPosition(char axis, long position) {
  if (axis == 'X') {
    transferArm.getXStepper().moveTo(position);
  } else if (axis == 'Z') {
    transferArm.getZStepper().moveTo(position);
  }
}

void TransferArmWebServer::setServoPosition(int angle) {
  transferArm.setServoPosition(angle);
}

void TransferArmWebServer::activateVacuum(bool state) {
  digitalWrite(SOLENOID_RELAY_PIN, state ? HIGH : LOW);
}

void TransferArmWebServer::forceState(PickCycleState newState) {
  setCurrentState(newState);
}

void TransferArmWebServer::toggleXMotorEnable() {
  config.xMotorEnabled = !config.xMotorEnabled;

  // Apply the enable/disable to the hardware
  digitalWrite(X_ENABLE_PIN,
               config.xMotorEnabled ? LOW : HIGH);  // Enable pin is active low

  saveConfig();
  sendConfigToClient();

  // Send log message
  JsonDocument logDoc;
  logDoc["type"] = "log";
  logDoc["message"] =
      config.xMotorEnabled ? "X-axis motor enabled" : "X-axis motor disabled";
  String logMessage;
  serializeJson(logDoc, logMessage);
  webSocket.textAll(logMessage);
}

// Movement tracking methods
bool TransferArmWebServer::isMovementInProgress() {
  return transferArm.isAnyMotorMoving();
}

void TransferArmWebServer::onMovementComplete() {
  // Log movement completion
  JsonDocument logDoc;
  logDoc["type"] = "log";
  logDoc["message"] = "Movement completed - Position updated";
  String logMessage;
  serializeJson(logDoc, logMessage);
  webSocket.textAll(logMessage);
}

// Logging Methods
void TransferArmWebServer::sendLogMessage(const String &message) {
  JsonDocument logDoc;
  logDoc["type"] = "log";
  logDoc["message"] = message;
  String logMessage;
  serializeJson(logDoc, logMessage);
  webSocket.textAll(logMessage);
}

bool TransferArmWebServer::hasConnectedClients() {
  return webSocket.count() > 0;
}