#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#define SPRINKLER_DEBUG

const char *SERVER_WIFI_SSID = "your_ssid";
const char *SERVER_WIFI_PASS = "your_pass";

ESP8266WebServer server(80);

const String week[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

struct tiempo {
  int dia;
  int hora;
  int minuto;
  int segundo;
  String weekday;
};
typedef struct tiempo tiempo_t;

#define EVERYDAY 1
#define ALTERNATE 2
#define PROGRAM 3

#define SECONDS_PRENDIDO 300
#define SECONDS_APAGADO 50
#define PROG_HORA 10
#define PROG_MINUTO 45
boolean progDia[7] = {false, true, true, true, false, true, false};
int modo = PROGRAM;

unsigned long nextMillis;
tiempo_t now;
int zona;
int secondsInStage;
boolean prendido = false;
boolean enCiclo = false;
boolean RelojActualizado = false;


/***********************************************/

void setup() {
#ifdef SPRINKLER_DEBUG
  Serial.begin(115200);
  while (!Serial);
#endif
  setupWiFi();
  setupDNS();
  setupWebServer();
}

/***********************************************/

void setupWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
#ifdef SPRINKLER_DEBUG
    Serial.println("Connecting to WiFi ");
#endif
    WiFi.begin(SERVER_WIFI_SSID, SERVER_WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
#ifdef SPRINKLER_DEBUG
      Serial.print(".");
#endif
    }
#ifdef SPRINKLER_DEBUG
    Serial.print("Connected to ");
    Serial.println(SERVER_WIFI_SSID);
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
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
#ifdef SPRINKLER_DEBUG
  Serial.println("HTTP server started");
#endif
}

/*******************************************/

void handleRoot() {
  String mensaje = " MODO ";
  mensaje += modo;
  mensaje += " Dia ";
  mensaje += now.dia;
  mensaje += " Hora: ";
  mensaje += now.hora;
  mensaje += " : ";
  mensaje += now.minuto;
  if (hoyToca()) {
    mensaje += "  Hoy Toca \n\n";
  }
  else {
    mensaje += "  Hoy No Toca \n\n";
  }
  if (prendido) {
    mensaje += " Prendido ";
  }
  else {
    mensaje += " Apagado ";
  }
  mensaje += "  zona ";
  mensaje += zona;
  server.send(200, "text/plain", mensaje);
}

/*******************************************/

void handleNotFound() {
  String mensaje = "File Not Found\n\n";
  mensaje += "URI: ";
  mensaje += server.uri();
  mensaje += "\nMethod: ";
  mensaje += (server.method() == HTTP_GET) ? "GET" : "POST";
  mensaje += "\nArguments: ";
  mensaje += server.args();
  mensaje += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    mensaje += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", mensaje);
}

/***********************************************/

void loop() {
  actReloj();
  reloj();
  ciclo();
  server.handleClient();
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
#ifdef SPRINKLER_DEBUG
    Serial.print("HTTP response code ");
    Serial.println(httpCode);
#endif
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
  }
#ifdef SPRINKLER_DEBUG
  else {
    Serial.println("Error in HTTP request");
  }
#endif
  http.end();
}

/***********************************************/

boolean reloj() {
  if (int(millis() - nextMillis) >= 0) {
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
    String mensaje = " Dia ";
    mensaje += now.dia;
    mensaje += " Hora ";
    mensaje += now.hora;
    mensaje += " Minuto ";
    mensaje += now.minuto;
    mensaje += " Segundo ";
    mensaje += now.segundo;
    mensaje += " Weekday ";
    mensaje += now.weekday;
    if (hoyToca()) {
      mensaje += " HoyToca ";
    }
    Serial.println(mensaje);
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
    default:
      ; // Error
  }
  return false;
}

/*********************************************/

void ciclo() {
  if (hoyToca()
      && now.hora == PROG_HORA
      && now.minuto == PROG_MINUTO
      && !enCiclo) {
    enCiclo = true;
    zona = 1;
    prendido = false;
  }
  if (enCiclo) {
    if (zona <= 4) {
      if (secondsInStage == 0) {
        prendido = !prendido;
        if (prendido) {
#ifdef SPRINKLER_DEBUG
          Serial.print(" prendido ");
          Serial.print(" zona ");
          Serial.println(zona);
#endif
          secondsInStage = SECONDS_PRENDIDO;
        }
        else {
#ifdef SPRINKLER_DEBUG
          Serial.print(" apagado ");
          Serial.print(" zona ");
          Serial.println(zona);
#endif
          secondsInStage = SECONDS_APAGADO;
          zona++;
        }
      }
    }
    else {
      enCiclo = false;
      secondsInStage = 0;
    }
  }
}
