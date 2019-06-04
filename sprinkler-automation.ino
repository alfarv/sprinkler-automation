#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#include "index.h"  // HTML //

#define SPRINKLER_DEBUG

const char* FS_FILE = "localLan.txt";
const char* SERVER_WIFI_SSID = "sprinKleR";
const char* SERVER_WIFI_PASS = "GreenerGrass";

struct wifi_config {
  String ssid;
  String pass;
  boolean valid_ip;
  byte ip[4];
  byte gateway[4];
  byte subnet[4];
};
typedef struct wifi_config wifi_config_t;

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

/***********************************************/

void setup() {
  wifi_config_t wifiConfig;
#ifdef SPRINKLER_DEBUG
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Booting");
#endif

  SPIFFS.begin();
  File f = SPIFFS.open(FS_FILE, "r");
  if (!f) {
    PARAM_write();
  }
  PARAM_read(f, &wifiConfig);
  f.close();
  setupWiFi(&wifiConfig);

  setupDNS();
  setupWebServer();
  setupOTA();
  pinMode(MOTOR, OUTPUT);
}

/*******************************************/

void PARAM_write() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(SERVER_WIFI_SSID, SERVER_WIFI_PASS);
  server.on("/", PARAM_input);
  server.onNotFound(handleNotFound);
  server.begin();
#ifdef SPRINKLER_DEBUG
  Serial.println(WiFi.softAPIP());
#endif
  while (true) {  // revisit this :(
    server.handleClient();
  }
}

/*******************************************/

void PARAM_input() {
  String html = MAIN_page;
  server.send(200, "text/html", html);
  if ((server.arg("lanId") != "")) {
    String parameters = "";
    for (uint8_t i = 0; i < server.args() - 1; i++) {
      parameters += server.arg(i) + ";";
    }
    File f = SPIFFS.open(FS_FILE, "w");
    f.print(parameters);
    f.close();
#ifdef SPRINKLER_DEBUG
    Serial.println("File writen with: " + parameters);
#endif
  }
}

/***********************************************/

void PARAM_read(File f, wifi_config_t* wifiConfig) {
  wifiConfig->ssid = f.readStringUntil(';');
  wifiConfig->pass = f.readStringUntil(';');
  if (f.readStringUntil(';') == "Manual") {
    (*wifiConfig).valid_ip = (true);
    readIp(f, (*wifiConfig).ip);
    readIp(f, (*wifiConfig).gateway);
    readIp(f, (*wifiConfig).subnet);
  } else {
    (*wifiConfig).valid_ip = (false);
  }
}

/*******************************************/

void readIp(File f, byte ip[4]) {
  ip[0] = f.readStringUntil(';').toInt();
  ip[1] = f.readStringUntil(';').toInt();
  ip[2] = f.readStringUntil(';').toInt();
  ip[3] = f.readStringUntil(';').toInt();
}

/***********************************************/

void setupWiFi(wifi_config_t* wifiConfig) {
  WiFi.disconnect();

  if (WiFi.status() != WL_CONNECTED) {
#ifdef SPRINKLER_DEBUG
    Serial.println("Connecting to WiFi ");
#endif
    WiFi.mode(WIFI_STA);
    if (wifiConfig->valid_ip) {
#ifdef SPRINKLER_DEBUG
      Serial.println("Using static IP configuration");
#endif
      WiFi.config(wifiConfig->ip, wifiConfig->gateway, wifiConfig->subnet);
    }
    WiFi.begin(wifiConfig->ssid, wifiConfig->pass);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
#ifdef SPRINKLER_DEBUG
      Serial.println("Connection Failed! Rebooting...");
#endif
      delay(5000);
      ESP.restart();
    }
#ifdef SPRINKLER_DEBUG
    Serial.println("Connected to " + (String)wifiConfig->ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
#endif
  }
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

void loop() {
  reloj();
  ciclo();
  server.handleClient();
  if (zona == SPRINKLER_OFF) {
    ArduinoOTA.handle();
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
