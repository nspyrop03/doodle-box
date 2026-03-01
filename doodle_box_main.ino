#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <GxEPD2_BW.h>

#include "secrets.h"

const char* ssid = SECRET_WIFI_SSID;
const char* password = SECRET_WIFI_PASS;

const char* mqtt_server = SECRET_MQTT_URL;
const char* mqtt_user = SECRET_MQTT_USER;
const char* mqtt_pass = SECRET_MQTT_PASS;
const int mqtt_port = 8883;

// ==========================================
// E-INK DISPLAY SETUP (WeAct 2.9" 296x128)
// Pins: BUSY=4, RST=16, DC=17, CS=5, CLK(SCL)=18, DIN(SDA)=23
// ==========================================
GxEPD2_BW<GxEPD2_290_BS, GxEPD2_290_BS::HEIGHT> display(GxEPD2_290_BS(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

// ==========================================
// SECURE CERTIFICATE (Let's Encrypt ISRG Root X1)
// ==========================================
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n" \
"TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n" \
"cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n" \
"WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n" \
"ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n" \
"MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJ1yKQzIqiHnncRo7\n" \
"bTsT/DHIgoRmTfl1A74PHEXGZe6XewGLEXcW+PUQz/s/B1Q2FmQfJ+7gH2j4K3uD\n" \
"hH+b2bIItO9zUoX7b+o2r6j7f3q3bQ/E0b0e5mZgO5b6Yx6I1w6yZ9b1q9Y/N7s5\n" \
"m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3\n" \
"m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3\n" \
"q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6\n" \
"v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9\n" \
"l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1\n" \
"k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3\n" \
"p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3\n" \
"r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3\n" \
"s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1\n" \
"s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3\n" \
"m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3\n" \
"q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6\n" \
"v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9\n" \
"l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1\n" \
"k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3\n" \
"p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3\n" \
"r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3s3m6v1k3r1s3q9l3p3\n" \
"-----END CERTIFICATE-----\n";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ==========================================
// Triggered instantly when mail arrives
// ==========================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("New Drawing Received! Bytes: ");
  Serial.println(length);

  if(length > 1000) { 
    Serial.println("Updating Screen...");
    
    display.setFullWindow();
    display.firstPage();
    do {
      // We pass BOTH colors: GxEPD_WHITE for '1' bits, and GxEPD_BLACK for '0' bits
      display.drawBitmap(0, 0, payload, 296, 128, GxEPD_WHITE, GxEPD_BLACK);
    } while (display.nextPage());

    Serial.println("Screen Update Complete! Listening for the next one...");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to HiveMQ...");
    if (client.connect("DoodleBox_Simple", mqtt_user, mqtt_pass)) {
      Serial.println("Connected!");
      client.subscribe("project/drawing");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize E-ink display
  display.init(115200, true, 2, false);
  display.setRotation(1);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected!");

  // Setup MQTT
  //espClient.setCACert(root_ca);
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
  client.setBufferSize(6000); // Crucial for image data size
}

// ==========================================
// MAIN LOOP (Always Awake, Always Listening)
// ==========================================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  // This function keeps the ESP32 listening to the cloud continuously
  client.loop(); 
}