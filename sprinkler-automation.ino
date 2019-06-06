#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "index.h"  // HTML //
#include "wifi.h"

#define SPRINKLER_DEBUG

const String week[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

struct tiempo {
  int dia;
  int hora;
  int minuto;
  int segundo;
  String weekday;
};
typedef struct tiempo tiempo_t;

tiempo_t now;

#define EVERYDAY 1
#define ALTERNATE 2
#define PROGRAM 3

#define NUM_ZONES 4
#define SPRINKLER_OFF NUM_ZONES

#define SECONDS_PRENDIDO 300
#define SECONDS_APAGADO 15
#define PROG_HORA 12
#define PROG_MINUTO 10
boolean progDia[7] = {false, true, true, true, false, true, false};
int modo = PROGRAM;

unsigned long nextMillis;
int zona = SPRINKLER_OFF;
int secondsInStage;
boolean prendido = false;
boolean RelojActualizado = false;

#define MOTOR LED_BUILTIN

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
  server.on("/off", []() {
    abortCiclo();
    server.send(200, "text/plain", "Apagado");
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
  String mensaje = week[now.dia] + " " + now.hora + ":" + now.minuto;
  mensaje +=
      "\nModo " + (String)modo + " - Hoy " + (hoyToca() ? "" : "No") + "Toca";
  mensaje +=
      (hoyToca() ? (String) " - " + PROG_HORA + (String) ":" + PROG_MINUTO
                 : "") +
      "\n";
  mensaje += (prendido ? "Prendido Zona " + (String)(zona + 1) : "Apagado");
  server.send(200, "text/plain", mensaje);
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
  reloj();
  ciclo();
  if (zona == SPRINKLER_OFF) {
    ArduinoOTA.handle();
  }
}

/***********************************************/

boolean reloj() {
  if (int(millis() - nextMillis) >= 0) {
    actReloj();
    now.segundo = (now.segundo + 1) % 60;
    if (now.segundo == 0) {
      now.minuto = (now.minuto + 1) % 60;
      if (now.minuto == 0) {
        now.hora = (now.hora + 1) % 24;
        if (now.hora == 0) {
          now.dia = (now.dia + 1) % 7;
          RelojActualizado = false;
        }
      }
    }
    nextMillis += 1000;
    if (secondsInStage > 0) {
      secondsInStage--;
    }
#ifdef SPRINKLER_DEBUG
    Serial.printf("%s %02d:%02d:%02d\n", week[now.dia].c_str(), now.hora,
                  now.minuto, now.segundo);
#endif
  }
}

/***********************************************/

void actReloj() {
  if (RelojActualizado) {
    return;
  }
  HTTPClient http;
  http.begin("http://free.timeanddate.com/clock/i6s3ue10/n156/tt0/tm1/th1/tb4");
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    int x = int(response.length());
    while (response.substring(x - 3, x) != "br>") {
      x = x - 1;
    }
    now.hora = response.substring(x, x + 2).toInt();
    now.minuto = response.substring(x + 3, x + 5).toInt();
    now.segundo = response.substring(x + 6, x + 8).toInt();
    // And day???
    while (response.substring(x - 3, x) != "t1>") {
      x = x - 1;
    }
    now.weekday = response.substring(x, x + 3);
    for (int i = 0; i < 7; i++) {
      if (now.weekday.equals(week[i])) {
        now.dia = i;
      }
    }
    nextMillis = millis() + 1000;
    RelojActualizado = true;
  } else {
#ifdef SPRINKLER_DEBUG
    Serial.println("Error in HTTP request to update clock");
#endif
    ESP.restart();
  }
  http.end();
}

/**********************************************/

boolean hoyToca() {
  switch (modo) {
    case EVERYDAY:
      return true;
      break;
    case ALTERNATE:
      return !(now.dia % 2 == 1);
      break;
    case PROGRAM:
      return progDia[now.dia];
      break;
    default:;  // Error
  }
  return false;
}

/*********************************************/

void ciclo() {
  if (hoyToca() && now.hora == PROG_HORA && now.minuto == PROG_MINUTO &&
      zona == SPRINKLER_OFF) {
    zona = 0;
    prendido = false;
    secondsInStage = 0;
  }
  if (zona < NUM_ZONES) {
    if (secondsInStage == 0) {
      prendido = !prendido;
      if (prendido) {
#ifdef SPRINKLER_DEBUG
        Serial.println("Prendido zona " + (String)(zona + 1));
#endif
        digitalWrite(MOTOR, HIGH);
        secondsInStage = SECONDS_PRENDIDO;
      } else {
#ifdef SPRINKLER_DEBUG
        Serial.println("Apagado zona " + (String)(zona + 1));
#endif
        digitalWrite(MOTOR, LOW);
        secondsInStage = SECONDS_APAGADO;
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
