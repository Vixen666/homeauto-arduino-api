#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#ifndef STASSID
#define STASSID "wifi"
#define STAPSK "pwd"
#endif

const int nbr_of_IO = 7;
int pinMapping[] = {16, 5, 4, 0, 2, 14, 12, 13, 15};

String MAC = "aa:bb:cc";
String IP = "";
String server_ip = "192.168.1.3";

int nbr_of_inputs = 1;
int nbr_of_outputs = 2;
String esp_version = "8266_nodemcu";
int sleepmode = 0;

int ioArray[nbr_of_IO][3];

const char *ssid = STASSID;
const char *password = STAPSK;

unsigned long previousMillis = 0;
unsigned long previousMillisRead = 0;
unsigned long previousMillisInput = 0;

unsigned long intervallRead = 1000;
unsigned long pingIntervall = 60000;
unsigned long intervallInput = 100;

ESP8266WebServer server(80);

const int led = 13;

void configRequestRecieved()
{
  if (server.method() == HTTP_POST)
  {
    for (uint8_t i = 0; i < server.args(); i++)
    {
      if (server.argName(i) == "MAC")
      {
        MAC = server.arg(i);
      }
      else if (server.argName(i) == "serverIp")
      {
        server_ip = server.arg(i);
      }
      else if (server.argName(i) == "sleepMode")
      {
        sleepmode = server.arg(i).toInt();
      }
      else if (server.argName(i) == "pingIntervall")
      {
        pingIntervall = server.arg(i).toInt() * 1000;
      }
      else if (server.argName(i) == "readIntervall")
      {
        intervallRead = server.arg(i).toInt();
      }
    }
    sendFullJson();
  }
  else
  {
    server.send(405, "application/json", "{\"errorCode\":\"Only POST allowed\"}");
  }
}

void inputRequestRecieved()
{
  if (server.method() == HTTP_POST)
  {
    int pinId = NULL;
    int input = NULL;
    int value = NULL;
    for (uint8_t i = 0; i < server.args(); i++)
    {
      if (server.argName(i) == "pinId")
      {
        pinId = server.arg(i).toInt();
      }
      else if (server.argName(i) == "input")
      {
        input = server.arg(i).toInt();
      }
      else if (server.argName(i) == "value")
      {
        value = server.arg(i).toInt();
      }
    }
    if (pinId != NULL)
    {
      if (input != NULL)
      {
        ioArray[pinId][1] = input;
      }
      ioArray[pinId][2] = value;
    }
    else
    {
      server.send(405, "application/json", "{\"errorCode\":\"Missing pinId\"}");
    }

    sendFullJson();
  }
  else
  {
    server.send(405, "application/json", "{\"errorCode\":\"Only POST allowed\"}");
  }
}

void sendFullJson()
{
  Serial.println("sendFullJson");
  String api = "{\"MAC\":\"";
  api += MAC;
  api += "\",\"IP\":\"";
  api += IP;

  api += "\",\"serverIp\":\"";
  api += server_ip;

  api += "\",\"espVersion\":\"";
  api += esp_version;

  api += "\",\"sleepMode\":";
  api += sleepmode;

  api += ",\"pingIntervall\":";
  api += pingIntervall;

  api += ",\"IO\":[";

  for (int i = 1; i < nbr_of_IO; i++)
  {
    Serial.println(api);
    if (i != 1)
    {
      api += ",";
    }
    api += "{";
    api += "\"pinId\":";
    api += i;

    api += ",\"gpio\":";
    api += pinMapping[i];

    api += ",\"input\":";
    api += ioArray[i][1];

    api += ",\"value\":";
    api += ioArray[i][2];

    api += "}";
  }
  api += "]";
  api += "}";

  Serial.println(api);
  server.send(200, "application/json", api);
}

void handleNotFound()
{
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void pingServer()
{
  WiFiClient client;

  HTTPClient http;
  if (http.begin(client, "http://jigsaw.w3.org/HTTP/connection.html"))
  {
    Serial.print("[HTTP] GET...\n");
    int httpCode = http.GET();
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        String payload = http.getString();
        Serial.println(payload);
      }
    }
    else
    {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }
}

boolean inputChanged()
{
  for (int i = 1; i < nbr_of_IO; i++)
  {
    if (ioArray[i][1] == 1)
    {
      if (ioArray[i][2] != digitalRead(pinMapping[i]))
      {
        return true;
      }
    }
  }
  return false;
}

void setup(void)
{

  for (int i = 1; i < nbr_of_IO; i++)
  {
    ioArray[i][0] = i;
    if (i < 5)
    {
      pinMode(pinMapping[i], INPUT);
      ioArray[i][1] = 1;
    }
    else
    {
      Serial.println("Setting Output:");
      pinMode(pinMapping[i], OUTPUT);
      digitalWrite(pinMapping[i], LOW);
      ioArray[i][1] = 0;
    }
    ioArray[i][2] = digitalRead(pinMapping[i]);
  }

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  IP = WiFi.localIP().toString();
  Serial.println(IP);

  if (MDNS.begin("esp8266"))
  {
    Serial.println("MDNS responder started");
  }

  server.on("/ping", sendFullJson);
  server.on("/config", configRequestRecieved);
  server.on("/input", inputRequestRecieved);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  for (int i = 0; i < nbr_of_IO; i++)
  {
    Serial.println(ioArray[i][0]);
    Serial.println(ioArray[i][1]);
    Serial.println(ioArray[i][2]);
  }
}

void loop(void)
{
  server.handleClient();
  //MDNS.update();
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillisRead >= intervallRead)
  {
    previousMillisRead = currentMillis;
    for (int i = 1; i < nbr_of_IO; i++)
    {
      if (ioArray[i][1] == 1)
      {
        ioArray[i][2] = digitalRead(pinMapping[i]);
      }
      else if (ioArray[i][1] == 0)
      {
        digitalWrite(pinMapping[i], ioArray[i][2]);
      }
    }
  }

  if (currentMillis - previousMillis >= pingIntervall)
  { //Ping Server
    previousMillis = currentMillis;
    Serial.println("Ping timer, pinging Server!");
    pingServer();
  }

  if (currentMillis - previousMillisInput >= intervallInput)
  { 
    if (inputChanged())
    {
      Serial.println("Values changed, pinging Server!");
      pingServer();
    }
  }
}
