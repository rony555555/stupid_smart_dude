#include <Arduino.h>
#include <WiFi.h>

const char* SSID     = "Yadin";
const char* PASSWORD = "54005400";

IPAddress local_IP(10, 100, 102, 101);   // static IP you want for the ESP32
IPAddress gateway(10, 100, 102, 1);      // usually your router's IP
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(10, 100, 102, 22);       // optional
IPAddress secondaryDNS(10, 100, 102, 1);

const uint16_t PORT = 8000;
WiFiServer server(PORT);

void connectWifi() {
  Serial.println("Configuring static IP...");
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("⚠️  Failed to configure static IP");
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ WiFi connected!");
  Serial.print("ESP32 IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Listening on TCP port ");
  Serial.println(PORT);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  connectWifi();
  server.begin();
}

void loop() {
  static WiFiClient client;

  if (!client || !client.connected()) {
    WiFiClient newClient = server.available();
    if (newClient) {
      client.stop();
      client = newClient;
      Serial.print("Client connected: ");
      Serial.println(client.remoteIP());
      client.println("Hello from ESP32 (static IP 192.168.1.200)");
    }
  }

  if (client && client.connected() && client.available()) {
    String line = client.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      Serial.print("RX: ");
      Serial.println(line);
      client.print("ESP32 echo: ");
      client.println(line);
    }
  }
}
