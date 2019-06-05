#include "wifi.h"

#include <ESP8266WiFi.h>

#include "Arduino.h"

const char* kWiFiFile = "localLan.txt";
const char* kAPssid = "sprinKleR";
const char* kAPpass = "GreenerGrass";

void WiFiConfig::Setup() {
  WiFi.disconnect();
  if (Read()) {
    mode_ = WIFI_STA;
    WiFi.mode(mode_);
    if (static_ip_) {
      WiFi.config(ip_, gateway_, subnet_);
    }
    WiFi.begin(ssid_, pass_);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      ESP.restart();
    }
  } else {
    mode_ = WIFI_AP;
    WiFi.mode(mode_);
    WiFi.softAP(kAPssid, kAPpass);
  }
}

void WiFiConfig::SetConfig(const String ssid, const String pass) {
  ssid_ = ssid;
  pass_ = pass;
  static_ip_ = false;
}

void WiFiConfig::SetConfig(const String ssid,
                           const String pass,
                           const IPAddress ip,
                           const IPAddress gateway,
                           const IPAddress subnet) {
  ssid_ = ssid;
  pass_ = pass;
  static_ip_ = true;
  ip_ = ip;
  gateway_ = gateway;
  subnet_ = subnet;
}

void WiFiConfig::Write() {
  SPIFFS.begin();
  File f = SPIFFS.open(kWiFiFile, "w");
  f.print(ssid_ + (String) ";" + pass_ + (String) ";" +
          (static_ip_
               ? ("Manual;" + ip_.toString() + (String) ";" +
                  gateway_.toString() + (String) ";" + subnet_.toString())
               : "Automatico"));
  f.close();
}

WiFiMode WiFiConfig::GetMode() {
  return mode_;
}

IPAddress WiFiConfig::GetIP() {
  switch (mode_) {
    case WIFI_STA:
      return WiFi.localIP();
    case WIFI_AP:
      return WiFi.softAPIP();
  }
}

bool WiFiConfig::Read() {
  SPIFFS.begin();
  File f = SPIFFS.open(kWiFiFile, "r");
  if (!f) {
    return false;
  }

  ssid_ = f.readStringUntil(';');
  pass_ = f.readStringUntil(';');
  if (f.readStringUntil(';') == "Manual") {
    static_ip_ = true;
    ip_ = IPAddress(
        f.readStringUntil('.').toInt(), f.readStringUntil('.').toInt(),
        f.readStringUntil('.').toInt(), f.readStringUntil(';').toInt());
    gateway_ = IPAddress(
        f.readStringUntil('.').toInt(), f.readStringUntil('.').toInt(),
        f.readStringUntil('.').toInt(), f.readStringUntil(';').toInt());
    subnet_ = IPAddress(
        f.readStringUntil('.').toInt(), f.readStringUntil('.').toInt(),
        f.readStringUntil('.').toInt(), f.readStringUntil(';').toInt());
  } else {
    static_ip_ = false;
  }
  f.close();
  return true;
}
