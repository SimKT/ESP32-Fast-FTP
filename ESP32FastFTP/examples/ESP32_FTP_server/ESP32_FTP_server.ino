
#include <WiFi.h>

// set #define FTP_DEBUG in ESP32FastFTP.h to see ftp commands in serial
#include "ESP32FastFTP.h"

const char *ssid = "Your AP SSID";
const char *password = "Your AP pass";
const char *ftpuser = "user";
const char *ftppass = "pass";
//#define SOFTAP   /// uncomment if you want a stand alone software access point server
//#define STATICIP /// uncomment if you want static ip

#ifdef STATICIP
IPAddress ipa(192, 168, 1, 75); /// conf for static address
IPAddress gate(192, 168, 1, 1);
IPAddress sub(255, 255, 255, 0);
#endif

FtpServer ftp;

void setup(void) {
  delay(2200);
  Serial.begin(115200);
  WiFi.persistent(0);
  WiFi.mode(WIFI_STA);
  Serial.println("");

#ifndef SOFTAP
///if connected to WiFi router
  #ifdef STATICIP
    WiFi.config(ipa, gate, sub);
  #endif
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
#else
///if ACCESS POINT
  #ifdef STATICIP
    Serial.println(WiFi.softAPConfig(ipa, gate, sub) ? "Ready" : "Failed!");
  #endif
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP("ESP32 FTP server", "12345678") ? "Ready" : "Failed!");
#endif
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //////////////////////////////
  // don't use SD.begin() for starting up SD interface, use this instead
  ftp.initSD(); // uncomment for SDI interface to sd card
  // ftp.initSD_MMC(); // uncomment
  //////////////////////////////
  // ftp.configPassiveIP(IPAddress(192, 168, 100, 2)); // to set passive IP if neccessary
  ///////////////////
  // begin (user , pass)
  ftp.beginFTP(ftpuser, ftppass);
}

void loop(void) {
  /// Handle request
  ftp.handleFTP();
}
