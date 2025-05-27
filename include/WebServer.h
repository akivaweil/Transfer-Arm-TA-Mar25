#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ArduinoJson.h>
#include <AsyncWebSocket.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "Settings.h"

//* ************************************************************************
//* ************************** WEB SERVER CLASS *************************
//* ************************************************************************
// This class handles the web interface, WebSocket communication, and
// configuration management for remote control of the Transfer Arm.

class TransferArmWebServer {
 private:
  AsyncWebServer server;
  AsyncWebSocket webSocket;
  Preferences preferences;

  // Configuration structure
  struct Config {
    // Position settings (in inches, converted to steps when used)
    float xPickupPosInches;
    float xDropoffPosInches;
    float zPickupLowerInches;
    float zDropoffLowerInches;
    float zSuctionStartInches;

    // Servo positions
    int servoPickupPos;
    int servoTravelPos;
    int servoDropoffPos;

    // Timing settings
    unsigned long pickupHoldTime;
    unsigned long dropoffHoldTime;
    unsigned long servoRotationWaitTime;

    // Speed and acceleration settings
    int xMaxSpeed;
    int xAcceleration;
    int zMaxSpeed;
    int zAcceleration;
    int zDropoffMaxSpeed;
    int zDropoffAcceleration;
    int xHomeSpeed;
    int zHomeSpeed;

    // WiFi settings
    char ssid[32];
    char password[64];
    bool apMode;
  } config;

  // WebSocket event handlers
  void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                        AwsEventType type, void *arg, uint8_t *data,
                        size_t len);
  static void onWebSocketEventWrapper(AsyncWebSocket *server,
                                      AsyncWebSocketClient *client,
                                      AwsEventType type, void *arg,
                                      uint8_t *data, size_t len);

  // HTTP handlers
  void handleRoot(AsyncWebServerRequest *request);
  void handleNotFound(AsyncWebServerRequest *request);
  void handleAPI(AsyncWebServerRequest *request);

  // Configuration management
  void loadConfig();
  void saveConfig();
  void resetToDefaults();

  // Message handlers
  void handleGetStatus();
  void handleGetConfig();
  void handleSetConfig(JsonDocument &doc);
  void handleManualControl(JsonDocument &doc);
  void handleEmergencyStop();

  // Utility methods
  void broadcastStatus();
  void sendConfigToClient(uint32_t clientId = 0);

  // Movement tracking
  bool isMovementInProgress();

 public:
  // Constructor
  TransferArmWebServer();

  // Main interface methods
  void begin();
  void update();

  // Configuration getters (for use by other modules)
  Config &getConfig() { return config; }

  // Manual control methods
  void triggerHoming();
  void triggerPickCycle();
  void moveToPosition(char axis, long position);
  void setServoPosition(int angle);
  void activateVacuum(bool state);
  void forceState(PickCycleState newState);

  void onMovementComplete();

  // Logging methods
  void sendLogMessage(const String &message);
  bool hasConnectedClients();
};

// Global instance declaration
extern TransferArmWebServer webServer;

#endif  // WEB_SERVER_H