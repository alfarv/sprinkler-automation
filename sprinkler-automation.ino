#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "clock.h"
#include "index.h"  // HTML //
#include "wifi.h"

#define SPRINKLER_DEBUG

#define EVERYDAY 1
#define ALTERNATE 2
#define PROGRAM 3

#define NUM_ZONES 4
#define SPRINKLER_OFF NUM_ZONES

#define SECONDS_PRENDIDO 300
#define SECONDS_APAGADO 15
#define PROG_HORA 2
#define PROG_MINUTO 10
boolean progDia[7] = {false, true, true, true, false, true, false};
int modo = EVERYDAY;

int zona = SPRINKLER_OFF;
boolean prendido, manual_on = false;

#define MOTOR 15

Clock sprinkler_clock;
ESP8266WebServer server(80);
WiFiConfig config;

/***********************************************/

void setup() {
#ifdef SPRINKLER_DEBUG
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Booting");
#endif

  config.Setup();
#ifdef SPRINKLER_DEBUG
  Serial.println("IP address: " + config.GetIP().toString());
#endif

  if (config.GetMode() == WIFI_AP) {
    setupAPWebServer();
    return;
  }
  setupWebServer();
  setupDNS();
  setupOTA();
  pinMode(MOTOR, OUTPUT);
}

/*******************************************/

void setupAPWebServer() {
  server.on("/", handleAPRoot);
  server.onNotFound(handleNotFound);
  server.begin();
}

/*******************************************/

void handleAPRoot() {
  server.send(200, "text/html", MAIN_page);
  if (server.arg("lanId") == "") {
    return;
  } else if (server.arg("isAuto") == "Automatic") {
    config.SetConfig(server.arg("lanId"), server.arg("pass"));
  } else if (server.arg("isAuto") == "Manual") {
    IPAddress ip(
        server.arg("ipEntry11").toInt(), server.arg("ipEntry12").toInt(),
        server.arg("ipEntry13").toInt(), server.arg("ipEntry14").toInt());
    IPAddress gateway(
        server.arg("ipEntry21").toInt(), server.arg("ipEntry22").toInt(),
        server.arg("ipEntry23").toInt(), server.arg("ipEntry24").toInt());
    IPAddress subnet(
        server.arg("ipEntry31").toInt(), server.arg("ipEntry32").toInt(),
        server.arg("ipEntry33").toInt(), server.arg("ipEntry34").toInt());
    config.SetConfig(server.arg("lanId"), server.arg("pass"), ip, gateway,
                     subnet);
  }
  config.Write();
  ESP.restart();
}

/***********************************************/

int setupWebServer() {
  server.on("/", handleRoot);
  server.on("/on", []() {
    manual_on=true;
    server.send(200, "text/plain", "Cycle Initiated");
  });
  server.on("/off", []() {
    abortCiclo();
    server.send(200, "text/plain", "Turned Off");
  });
  server.on("/format", []() {
    int ret = SPIFFS.format();
    server.send(200, "text/plain",
                (String) "Format " + (ret ? "Success" : "Failed"));
  });
  server.onNotFound(handleNotFound);
  server.begin();
#ifdef SPRINKLER_DEBUG
  Serial.println("HTTP server started");
#endif
}

/*******************************************/

void handleRoot() {
  server.send(
      200, "text/plain",
      sprinkler_clock.GetTime() + "\nModo " + (String)modo +
          (hoyToca() ? "\nHoy Toca - " + PROG_HORA + (String) ":" + PROG_MINUTO
                     : "\nHoy No Toca") +
          (prendido ? "\nPrendido Zona " + (String)(zona + 1) : "\nApagado"));
}

/*******************************************/

void handleNotFound() {
  String mensaje =
      "File Not Found\n\nURI: " + (String)server.uri() +
      "\nMethod: " + (String)((server.method() == HTTP_GET) ? "GET" : "POST") +
      (String) "\nArguments: " + (String)server.args() + "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    mensaje +=
        " " + (String)server.argName(i) + ": " + (String)server.arg(i) + "\n";
  }
  server.send(404, "text/plain", mensaje);
}

/***********************************************/

int setupDNS() {
  int ret = MDNS.begin("esp8266");
#ifdef SPRINKLER_DEBUG
  if (ret) {
    Serial.println("MDNS responder started");
  }
#endif
  return ret;
}

/*******************************************/

void setupOTA() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // No authentication by default
  // ArduinoOTA.setPassword("mypass");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");
  ArduinoOTA.onStart([]() { Serial.println("Start updating"); });
  ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
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
}

/***********************************************/

void loop() {
  server.handleClient();
  if (config.GetMode() == WIFI_AP) {
    return;
  }
  sprinkler_clock.HandleTime();
  ciclo();
  if (zona == SPRINKLER_OFF) {
    ArduinoOTA.handle();
  }
}

/**********************************************/

boolean hoyToca() {
  switch (modo) {
    case EVERYDAY:
      return true;
      break;
    case ALTERNATE:
      return !(sprinkler_clock.GetDay() % 2 == 1);
      break;
    case PROGRAM:
      return progDia[sprinkler_clock.GetDay()];
      break;
    default:;  // Error
  }
  return false;
}

/*********************************************/

void ciclo() {
  if ((hoyToca() && sprinkler_clock.GetHour() == PROG_HORA &&
      sprinkler_clock.GetMinute() == PROG_MINUTO && zona == SPRINKLER_OFF)or manual_on) {
    zona = 0;
    prendido = false;
    sprinkler_clock.SetTimer(0);
  }
  if (zona < NUM_ZONES) {
    if (sprinkler_clock.IsTimerDone()) {
      prendido = !prendido;
      manual_on=false;
      if (prendido) {
#ifdef SPRINKLER_DEBUG
        Serial.println("Prendido zona " + (String)(zona + 1));
#endif
        digitalWrite(MOTOR, HIGH);
        sprinkler_clock.SetTimer(SECONDS_PRENDIDO);
      } else {
#ifdef SPRINKLER_DEBUG
        Serial.println("Apagado zona " + (String)(zona + 1));
#endif
        digitalWrite(MOTOR, LOW);
        sprinkler_clock.SetTimer(SECONDS_APAGADO);
        zona++;
      }
    }
  }
}

/*********************************************/

void abortCiclo() {
  digitalWrite(MOTOR, LOW);
  zona = SPRINKLER_OFF;
}
