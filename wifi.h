#ifndef SPRINKLER_AUTOMATION_WIFI_H_
#define SPRINKLER_AUTOMATION_WIFI_H_

#include <ESP8266WiFi.h>

#include "Arduino.h"

class WiFiConfig {
 public:
  WiFiConfig(String ssid, String pass)
      : ssid_(ssid), pass_(pass), static_ip_(false){};
  WiFiConfig(String ssid,
             String pass,
             IPAddress ip,
             IPAddress gateway,
             IPAddress subnet)
      : ssid_(ssid),
        pass_(pass),
        ip_(ip),
        gateway_(gateway),
        subnet_(subnet),
        static_ip_(true){};
  ~WiFiConfig() = default;

  void setup();

 private:
  String ssid_;
  String pass_;
  bool static_ip_;
  IPAddress ip_;
  IPAddress gateway_;
  IPAddress subnet_;
};

#endif  // SPRINKLER_AUTOMATION_WIFI_H_
