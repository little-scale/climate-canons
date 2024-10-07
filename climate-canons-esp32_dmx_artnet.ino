#include <Arduino.h>
#include <esp_dmx.h>
#include "ArtnetWifi.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// #define DHCP_DISABLED

#ifdef DHCP_DISABLED

IPAddress local_IP(192, 168, 1, 154);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 1, 1);  //optional
IPAddress secondaryDNS(1, 1, 1, 1);    //optional

#endif

WiFiUDP UdpSend;
ArtnetWifi artnet;

const char* ssid = "CompactRouter";
const char* password = "jellybean3000";

// const char* ssid = "OliveNet New";
// const char* password = "chummyteapot589";

int transmitPin = 14;
int receivePin = 27;
int enablePin = 21;

dmx_port_t dmxPort = 1;

byte data[DMX_PACKET_SIZE];

//ArtNet settings

const int startUniverse = 3;
const int maxUniverses = 1;
const int numberOfChannels = 512;
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;

void setup() {
  Serial.begin(57600);
  WiFi.mode(WIFI_STA);

#ifdef DHCP_DISABLED
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
#endif

  WiFi.begin(ssid, password);
  delay(1000);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("ESP32-ArtNet-to-DMX-Converter");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else  // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR)
        Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR)
        Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR)
        Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR)
        Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR)
        Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  artnet.setArtDmxCallback(onArtNetFrame);

  artnet.begin("ESP32-ArtNet-to-DMX-Converter");

  /* Set the DMX hardware pins to the pins that we want to use. */
  dmx_config_t config = DMX_CONFIG_DEFAULT;
  dmx_driver_install(dmxPort, &config, DMX_INTR_FLAGS_DEFAULT);
  dmx_set_pin(dmxPort, transmitPin, receivePin, enablePin);
}

void onArtNetFrame(uint16_t universe, uint16_t numberOfChannels, uint8_t sequence, uint8_t* dmxData) {

  sendFrame = 1;

  // Store which universe has got in
  if ((universe - startUniverse) < maxUniverses)
    universesReceived[universe - startUniverse] = 1;

  for (int i = 0; i < maxUniverses; i++) {
    if (universesReceived[i] == 0) {
      //Serial.println("Broke");
      sendFrame = 0;
      break;
    }
  }
  // read universe and put into the right array of data
  for (int i = 0; i < numberOfChannels; i++) {

    if (universe == startUniverse) {
      data[i + 1] = dmxData[i];
    }
  }

  previousDataLength = numberOfChannels;
  memset(universesReceived, 0, maxUniverses);
}

void updateDMX() {
  dmx_write(dmxPort, data, DMX_PACKET_SIZE);
  dmx_send(dmxPort, DMX_PACKET_SIZE);
  dmx_wait_sent(dmxPort, DMX_TIMEOUT_TICK);
}

void loop() {
  if ((WiFi.status() == WL_CONNECTED)) {
    artnet.read();
  }
  updateDMX(); 
}