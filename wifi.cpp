#include "wifi.h"

#include <ESP8266WiFi.h>

#include "Arduino.h"

void WiFiConfig::setup() {
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  if (static_ip_) {
    WiFi.config(ip_, gateway_, subnet_);
  }
  WiFi.begin(ssid_, pass_);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    ESP.restart();
  }
}
