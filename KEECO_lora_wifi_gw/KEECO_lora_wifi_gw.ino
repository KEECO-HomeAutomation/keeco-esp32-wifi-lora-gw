/*
   KEECO HW Node application software
   Version 3.9
   Developed by https://github.com/litechniks

   Only Manage_IO needs to be modified to implement your application
   The usecase in this code is a gate lock functionality with and RFID reader that is operated over MQTT


*/

#include "Arduino.h"
#include "heltec.h"
#include <WiFi.h>
#include <EEPROM.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <arduino-timer.h>             //https://github.com/contrem/arduino-timer
#include <FS.h>
#include "SPIFFS.h"
#include <PubSubClient.h>      //https://pubsubclient.knolleary.net/api.html
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include "configFileHandler.h"
#include "KEECO_OLED_display.h"
#include "lorahandler.h"
#include <stdio.h>


#define DEBUG     //to enable debug purpose serial output 
#define OTA       //to enable OTA updates
#define CSS       //makes the web interface nicer but also slower and less reliable
#define TIMERVALUE 5000

#define BAND    868E6

const char* update_path = "/firmware";
const char* update_username = "admin";
const char* update_password = "admin";

WebServer webserver(80);

IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

ConfigurationHandler espConfig;
displayHandler dh;
LoraHandler lh;


//timer for various tasks - for future scalability
auto timer = timer_create_default();

//https://bit.ly/2WPt42i


/*
  Initializing the KEECO HW Node
  Serial Timeout is needed for the serial command terminal
  LittleFS - is used to store the configuration data - SSID, Password, mqtt_server, UUID
  espConfig - global object storing the configuration variables
  initWifiOnBoot - try to connect to Infrastructure WiFi (60 sec timeout). If not successful start AP mode. Later if disconnected from STA then AP reactivates. Also starts mDNS.
  initWebserver - webserver to set configuration parameters
  initMqtt - connect to the set mqtt server. Name is resolved with mDNS. Set subscriptions.
  initIO - place your custom init code in this function
  InitOTA - initializing OTA
  timer - setup a timer that calls publishIO() - place your periodically called code there

*/
void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Serial.setTimeout(10);
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Starting KEECO Node...");
  dh.addLine("Starting KEECO Node...");
  SPIFFS.begin(true);                 //added true so that it will format if needed
  Serial.println("[=_______]");
  dh.addLine("Init Config");
  espConfig.initConfiguration();
  Serial.println("[==______]");
  dh.addLine("Init WiFi");
  initWifiOnBoot();
  Serial.println("[===_____]");
  dh.addLine("Init WebServer");
  initWebserver();
  Serial.println("[====____]");
  dh.addLine("Init I/O");
  initIO();
  Serial.println("[=====___]");
  dh.addLine("Init MQTT");
  initMqtt();
  Serial.println("[======__]");
  dh.addLine("Init OTA");
  InitOTA();
  Serial.println("[=======_]");
  dh.addLine("Init Timer");
  timer.every(TIMERVALUE, timerCallback);
  Serial.println("[========]");
  dh.addLine("Init Done");
  Serial.println("Welcome - ver 3.2!");
  dh.addLine("Welcome - ver 3.2!");
  Serial.println("Help: {\"command\":\"help\"}");
  dh.addLine("Help: {\"command\":\"help\"}");
  Serial.println("https://bit.ly/2WPt42i");
  dh.addLine(toStringIp(WiFi.localIP()));
  dh.updateInternalStat();
  dh.displayStatuses(dh.display_stat);
}

void loop() {
  timer.tick();
  webserverInLoop();
  mqttInLoop();
  IOprocessInLoop();
  OTAInLoop();
  espConfig.serialCmdCheckInLoop();
  dh.displayInLoop();
  lh.loraInLoop();
}
