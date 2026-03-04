#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <GxEPD2_BW.h>
#include <driver/rtc_io.h>

#include "secrets.h"

#define WAKE_BUTTON GPIO_NUM_33
#define IMAGE_BYTES 4736

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

WiFiClientSecure espClient;
PubSubClient client(espClient);

bool newDrawingReceived = false;
RTC_DATA_ATTR byte lastDrawing[IMAGE_BYTES];
RTC_DATA_ATTR bool isFirstBoot = true;

bool isDrawingBlank(byte* payload, unsigned int length) {
  byte firstByte = payload[0];

  for (unsigned int i = 1; i < length; i++) {
    if (payload[i] != firstByte) {
      return false;
    }
  }
  return true;
}

// ==========================================
// Triggered instantly when mail arrives
// ==========================================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("New Drawing Received! Bytes: ");
  Serial.println(length);

  if(length >= IMAGE_BYTES) { 
    if (isDrawingBlank(payload, IMAGE_BYTES)) {
      Serial.println("Received an empty canvas! Ignoring to save previous drawing.");
      newDrawingReceived = true; 
      return;
    }

    bool drawingChanged = false;
    
    if(isFirstBoot) {
      Serial.println("First boot detected. Forcing screen update...");
      drawingChanged = true;
      isFirstBoot = false;
    } else if(memcmp(lastDrawing, payload, IMAGE_BYTES) != 0) {
      Serial.println("New doodle detected!");
      drawingChanged = true;
    } else {
      Serial.println("Drawing is exactly the same! Skipping refresh...");
    }

    if(drawingChanged) {
      Serial.println("Updating screen...");
      memcpy(lastDrawing, payload, IMAGE_BYTES);

      display.setFullWindow();
      display.firstPage();
      do {
        // We pass BOTH colors: GxEPD_WHITE for '1' bits, and GxEPD_BLACK for '0' bits
        display.drawBitmap(0, 0, payload, 296, 128, GxEPD_WHITE, GxEPD_BLACK);
      } while (display.nextPage());
    }
    newDrawingReceived = true;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nWAKING UP\n\n");
  
  // Initialize E-ink display
  display.init(115200, true, 2, false);
  display.setRotation(1);

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);

  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 20) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }

  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi Connected!");

    // Setup MQTT
    espClient.setInsecure();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(mqttCallback);
    client.setBufferSize(6000); // Crucial for image data size

    if(client.connect("DoodleBox_Device", mqtt_user, mqtt_pass)) {
      Serial.println("Connected to HiveMQ Cloud!");
      client.subscribe("project/drawing");

      Serial.println("Checking mailbox...");
      unsigned long startTime = millis();
      while(millis() - startTime < 5000) {
        client.loop();
        delay(10);
      }
      
      if(!newDrawingReceived) {
        Serial.println("Mailbox is empty.");
      }
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
    }
  } else {
    Serial.println("Failed to connect to the WiFi!");
  }

  Serial.println("Powering down screen and going to Deep Sleep...");
  
  // Cut power to the screen to save battery and prevent burn-in
  display.hibernate(); 

  // Configure the Wake Button
  // Tell it to wake up when Pin 33 goes LOW (0)
  esp_sleep_enable_ext0_wakeup(WAKE_BUTTON, 0); 
  // Turn on the internal pull-up resistor
  rtc_gpio_pullup_en(WAKE_BUTTON);
  rtc_gpio_pulldown_dis(WAKE_BUTTON);

  uint64_t time_in_sec = 60;
  esp_sleep_enable_timer_wakeup(time_in_sec * 1000000ULL); // convert to ms

  esp_deep_sleep_start();
}

void loop() { }
