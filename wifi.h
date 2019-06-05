#ifndef SPRINKLER_AUTOMATION_WIFI_H_
#define SPRINKLER_AUTOMATION_WIFI_H_

#include <ESP8266WiFi.h>
#include <FS.h>

#include "Arduino.h"

class WiFiConfig {
 public:
  WiFiConfig() = default;
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

  void Setup();
  void SetConfig(const String ssid, const String pass);
  void SetConfig(const String ssid,
                 const String pass,
                 const IPAddress ip,
                 const IPAddress gateway,
                 const IPAddress subnet);
  void Write();
  WiFiMode GetMode();
  IPAddress GetIP();

 private:
  WiFiMode mode_;
  String ssid_;
  String pass_;
  bool static_ip_;
  IPAddress ip_;
  IPAddress gateway_;
  IPAddress subnet_;

  bool Read();
};

#endif  // SPRINKLER_AUTOMATION_WIFI_H_
