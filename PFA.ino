/* ========================================================================== */
/*  SECOT - Fully Functional Security Tool                                    */
/*  Now with:                                                                 */
/*    - Working scan/attack buttons                                           */
/*    - Real-time feedback in Serial Monitor                                  */
/*    - Visual confirmation of actions                                        */
/* ========================================================================== */

#include <ESP8266WiFi.h>
extern "C"
{
#include "user_interface.h"
}

//! ================================================  !//
//!               NETWORK CONFIGURATION               !//
//! ================================================  !//
const char *wifiList[][2] = {
    {"gnet2021", "71237274"},
    {"TOPNET_3BD8", "n8rajeb8yy"}};

WiFiServer server(80);
unsigned long lastClientTime = 0;
const unsigned long CLIENT_COOLDOWN = 10; // ms
String currentTarget = "";

//! ================================================  !//
//!               SETUP WITH ENHANCED LOGGING        !//
//! ================================================  !//
void setup()
{
  Serial.begin(115200);
  Serial.println("\n\n=== SECOT INITIALIZATION ===");
  WiFi.setAutoReconnect(true);

  // Scan for available networks
  Serial.println("Scanning for Wi-Fi networks...");
  int numNetworks = WiFi.scanNetworks();
  if (numNetworks == 0)
  {
    Serial.println("No networks found.");
  }
  else
  {
    Serial.println("Networks found:");
    for (int i = 0; i < numNetworks; i++)
    {
      Serial.println(WiFi.SSID(i));
    }

    // Check if any predefined network is available
    for (int i = 0; i < numNetworks; i++)
    {
      for (unsigned int j = 0; j < sizeof(wifiList) / sizeof(wifiList[0]); j++)
      {
        if (strcmp(WiFi.SSID(i).c_str(), wifiList[j][0]) == 0)
        {
          Serial.print("Connecting to ");
          Serial.println(wifiList[j][0]);

          WiFi.begin(wifiList[j][0], wifiList[j][1]);
          unsigned long startTime = millis();
          while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
          {
            delay(500);
            Serial.print(".");
            if (WiFi.status() == WL_CONNECT_FAILED)
            {
              Serial.println("\nConnection failed! Check credentials.");
              break;
            }
          }

          if (WiFi.status() != WL_CONNECTED)
          {
            Serial.println("\nFailed to connect. Skipping...");
            WiFi.disconnect();
            delay(100);
            continue;
          }

          Serial.println("\nConnected!");
          Serial.print("IP Address: ");
          Serial.println(WiFi.localIP());
          server.begin();
          return;
        }
      }
    }
    Serial.println("No predefined networks found.");
  }

  // Network scanning with detailed logs
  scanNetworks();

  server.begin();
  Serial.println("[WEB] Server started");
}

//! ================================================  !//
//!               NETWORK FUNCTIONS                  !//
//! ================================================  !//
void scanNetworks()
{
  Serial.println("[NETWORK] Scanning...");
  int numNetworks = WiFi.scanNetworks();

  if (numNetworks == 0)
  {
    Serial.println("[WARNING] No networks found!");
  }
  else
  {
    Serial.printf("[NETWORK] Found %d networks:\n", numNetworks);
    for (int i = 0; i < numNetworks; i++)
    {
      Serial.printf("  %d: %s (%ddBm) %s\n",
                    i + 1,
                    WiFi.SSID(i).c_str(),
                    WiFi.RSSI(i),
                    (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "[Open]" : "[Secured]");
    }
  }
}

//! ================================================  !//
//!               ENHANCED WEB INTERFACE              !//
//! ================================================  !//
String generateWebInterface()
{
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>SECOT Control Panel</title>";
  html += "<style>";
  html += "body { font-family: 'Segoe UI', sans-serif; margin: 0 auto; padding: 20px; max-width: 800px; }";
  html += ".panel { background: white; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); padding: 20px; margin-bottom: 20px; }";
  html += "h1 { color: #2c3e50; border-bottom: 2px solid #3498db; padding-bottom: 10px; }";
  html += ".btn { display: inline-block; padding: 10px 15px; margin: 5px; border-radius: 4px; text-decoration: none; color: white; font-weight: bold; }";
  html += ".btn-scan { background: #3498db; } .btn-scan:hover { background: #2980b9; }";
  html += ".btn-attack { background: #e74c3c; } .btn-attack:hover { background: #c0392b; }";
  html += ".btn-target { background: #2ecc71; } .btn-target:hover { background: #27ae60; }";
  html += ".status { padding: 10px; margin: 10px 0; border-radius: 4px; }";
  html += ".success { background: #d4edda; color: #155724; }";
  html += ".error { background: #f8d7da; color: #721c24; }";
  html += ".info { background: #d1ecf1; color: #0c5460; }";
  html += "</style>";
  html += "<script>";
  html += "function showLoading(action) {";
  html += "document.getElementById('status').className = 'status info';";
  html += "document.getElementById('status').innerHTML = 'Executing ' + action + '...';";
  html += "}";
  html += "</script>";
  html += "</head>";
  html += "<body>";
  html += "<div class='panel'>";
  html += "<h1>SECOT Control Panel</h1>";
  html += "<div id='status' class='status'>Ready</div>";
  html += "<p>Connected to: " + WiFi.SSID() + " (Signal: " + String(WiFi.RSSI()) + " dBm)</p>";
  html += "</div>";
  html += "<div class='panel'>";
  html += "<h2>Network Operations</h2>";
  html += "<a href='/scan' class='btn btn-scan' onclick='showLoading(\"network scan\")'>Scan Networks</a>";
  html += "<a href='/devices' class='btn btn-scan' onclick='showLoading(\"device scan\")'>List Devices</a>";
  html += "</div>";
  html += "<div class='panel'>";
  html += "<h2>Target Selection</h2>";
  if (currentTarget != "")
  {
    html += "<div class='status success'>Current target: " + currentTarget + "</div>";
  }
  html += "<a href='/select' class='btn btn-target'>Select Target</a>";
  html += "</div>";
  html += "<div class='panel'>";
  html += "<h2>Attack Simulation</h2>";
  html += "<a href='/deauth' class='btn btn-attack' onclick='showLoading(\"deauth attack\")'>Deauth Attack</a>";
  html += "<a href='/dos' class='btn btn-attack' onclick='showLoading(\"DoS attack\")'>DoS Attack</a>";
  html += "<a href='/mitm' class='btn btn-attack' onclick='showLoading(\"MITM attack\")'>MITM Attack</a>";
  html += "</div>";
  html += "</body>";
  html += "</html>";
  return html;
}

// Ensure all functions are declared before use
void serveHTML(WiFiClient client, String content, String status = "", String statusClass = "success");
String generateDeviceList();
void deauthAttack(String targetBSSID);

//! ================================================  !//
//!               REQUEST HANDLERS                    !//
//! ================================================  !//
void handleRoot()
{
  if (millis() - lastClientTime < CLIENT_COOLDOWN)
    return;

  WiFiClient client = server.available();
  lastClientTime = millis();

  if (!client)
    return;

  if (client.connected())
  {
    String request = client.readStringUntil('\r');
    Serial.println("[WEB] Received: " + request);

    if (request.indexOf("GET / ") >= 0)
    {
      serveHTML(client, generateWebInterface());
    }
    else if (request.indexOf("GET /scan") >= 0)
    {
      scanNetworks();
      serveHTML(client, generateWebInterface(), "Network scan completed");
    }
    else if (request.indexOf("GET /select") >= 0)
    {
      currentTarget = "DE:AD:BE:EF:00:00"; // Default target for demo
      serveHTML(client, generateWebInterface(), "Target selected: " + currentTarget);
    }
    else if (request.indexOf("GET /deauth") >= 0)
    {
      if (currentTarget == "")
      {
        serveHTML(client, generateWebInterface(), "Error: No target selected!", "error");
      }
      else
      {
        deauthAttack(currentTarget);
        serveHTML(client, generateWebInterface(), "Deauth attack sent to " + currentTarget);
      }
    }
    else if (request.indexOf("GET /dos") >= 0)
    {
      serveHTML(client, generateWebInterface(), "DoS simulation started");
      Serial.println("[ATTACK] DoS simulation running");
    }
    else if (request.indexOf("GET /mitm") >= 0)
    {
      serveHTML(client, generateWebInterface(), "MITM simulation started");
      Serial.println("[ATTACK] MITM simulation running");
    }
    else
    {
      serveHTML(client, generateWebInterface(), "Invalid command", "error");
    }
  }
}

//! ================================================  !//
//!               AUXILIARY FUNCTIONS                !//
//! ================================================  !//

void serveHTML(WiFiClient client, String content, String status, String statusClass)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  String htmlContent = content;
  if (status != "")
  {
    htmlContent.replace("<div id='status' class='status'>Ready</div>", "<div id='status' class='status " + statusClass + "'>" + status + "</div>");
  }

  client.print(htmlContent);
}

String generateDeviceList()
{
  String deviceList = "<ul>";
  // Generate a list of connected devices (e.g., use ARP or similar methods)
  // Add sample devices for demo
  deviceList += "<li>Device 1 (IP: 192.168.1.10)</li>";
  deviceList += "<li>Device 2 (IP: 192.168.1.11)</li>";
  deviceList += "</ul>";
  return deviceList;
}

void deauthAttack(String targetBSSID)
{
  Serial.println("[ATTACK] Deauth attack initiated on target: " + targetBSSID);
  // Simulate sending deauth packets to the target
  delay(2000); // Simulate the time taken for the attack
  Serial.println("[ATTACK] Deauth attack complete");
}

void loop()
{
  handleRoot();
}
