#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>

#define PIN_PWRLED_MB 5
#define PIN_HDDLED_MB 4
#define PIN_PWRBTN_MB 6
#define PIN_RSTBTN_MB 7

#define PIN_PWRLED_CASE 1
#define PIN_HDDLED_CASE 10
#define PIN_PWRBTN_CASE 0
#define PIN_RSTBTN_CASE 8

TaskHandle_t taskHandle[7];

static bool pwrLedState = false;
static bool hddLedState = false;
static bool powerctl = false;

enum class PowerState
{
  Off,
  On,
  Sleep
};
static PowerState powerState = PowerState::Off;
static Preferences Config;

void httpServer(void *pvParameters);
void PwrLedControl(void *pvParameters);
void PwrControl(void *pvParameters);
void LedPwm(void *pvParameters);
void PowerButton(void *pvParameters);
void SerialInput(void *pvParameters);

void setup()
{
  Serial.begin(9600);
  Serial.println("hello");

  pinMode(PIN_PWRLED_MB, OUTPUT);
  pinMode(PIN_HDDLED_MB, OUTPUT);
  pinMode(PIN_PWRBTN_MB, INPUT_PULLUP);
  pinMode(PIN_RSTBTN_MB, INPUT_PULLUP);

  pinMode(PIN_PWRLED_MB, INPUT_PULLUP);
  pinMode(PIN_HDDLED_MB, INPUT_PULLUP);
  pinMode(PIN_PWRBTN_MB, OUTPUT);
  pinMode(PIN_RSTBTN_MB, OUTPUT);

  Config.begin("config");

  xTaskCreatePinnedToCore(httpServer, "web", 8192, NULL, 20, &taskHandle[0], 1);
  xTaskCreatePinnedToCore(PwrLedControl, "led", 1024, NULL, 10, &taskHandle[1], 0);
  xTaskCreatePinnedToCore(PwrControl, "pwr", 1024, NULL, 15, &taskHandle[2], 0);
  xTaskCreatePinnedToCore(LedPwm, "pwm", 1024, NULL, 0, &taskHandle[3], 0);
  xTaskCreatePinnedToCore(PowerButton, "btn", 1024, NULL, 10, &taskHandle[4], 0);
  xTaskCreatePinnedToCore(SerialInput, "ser", 4096, NULL, 0, &taskHandle[5], 0);

  Serial.println("nya");
}

void SerialInput(void *pvParameters)
{
  while (1)
  {
    if (Serial.available())
    {
      const String raw = Serial.readStringUntil('\n');
      const int splitter = raw.indexOf(':');

      if (splitter != -1)
      {
        String name = raw.substring(0, splitter);
        String data = raw.substring(splitter + 1);
        if (name == "WIFI.SSID" || name == "WIFI.PASSWORD" || name == "WEB.PASSWORD" || name == "WEB.HOSTNAME")
        {
          Config.putString(name.c_str(), data);
          Serial.println(name + ":" + data + ", saved");
          if (name == "WIFI.SSID" || name == "WIFI.PASSWORD")
          {
            WiFi.disconnect();
            WiFi.begin(Config.getString("WIFI.SSID").c_str(), Config.getString("WIFI.PASSWORD").c_str());
          }
          else if (name == "WEB.HOSTNAME")
          {
            MDNS.end();
            delay(100);
            MDNS.begin(Config.getString("WEB.HOSTNAME", "RemotePowerButton").c_str());
            MDNS.addService("http", "tcp", 80);
          }
        }
        else
        {
          Serial.println(name + " is not in list");
        }
      }
      else if (raw == "WIFI.SSID" || raw == "WIFI.PASSWORD" || raw == "WEB.PASSWORD" || raw == "WEB.HOSTNAME")
      {
        Serial.println(raw + ":" + Config.getString(raw.c_str()));
      }
      else
      {
        Serial.println("> KEY:VALUE");
      }
      delay(100);
    }
  }
}

const uint16_t sinTable[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 71, 79, 87, 94, 102, 109, 116, 123, 130, 137, 144, 150, 157, 163, 169, 175, 181, 187, 192, 197, 202, 207, 212, 216, 220, 224, 228, 232, 235, 238, 241, 243, 246, 248, 250, 251, 253, 254, 255, 255, 256, 256};

void LedPwm(void *pvParameters)
{
  ledcSetup(0, 12800, 8);
  ledcAttachPin(PIN_PWRLED_CASE, 0);
  ledcSetup(1, 12800, 8);
  ledcAttachPin(PIN_HDDLED_CASE, 1);

  int pwrlevel = 0;
  int hddlevel = 0;
  while (1)
  {
    if (pwrLedState && pwrlevel < 49)
    {
      pwrlevel++;
      ledcWrite(0, sinTable[pwrlevel]);
    }
    else if (!pwrLedState && pwrlevel >= 2)
    {
      pwrlevel -= 2;
      ledcWrite(0, sinTable[pwrlevel]);
    }
    else if (!pwrLedState && pwrlevel == 1)
    {
      pwrlevel = 0;
      ledcWrite(0, sinTable[pwrlevel]);
    }

    int state = !digitalRead(PIN_HDDLED_MB);
    if (state && hddlevel < 49)
    {
      hddlevel++;
      ledcWrite(1, sinTable[hddlevel]);
    }
    else if (state && hddlevel >= 2)
    {
      hddlevel -= 2;
      ledcWrite(1, sinTable[hddlevel]);
    }
    else if (state && hddlevel == 1)
    {
      hddlevel = 0;
      ledcWrite(1, sinTable[hddlevel]);
    }

    delay(7);
  }
}

void PwrLedControl(void *pvParameters)
{
  Serial.println("ledControl");
  while (1)
  {
    switch (powerState)
    {
    case PowerState::On:
      pwrLedState = true;
      delay(2900);
      break;

    case PowerState::Sleep:
      pwrLedState = true;
      delay(1000);
      pwrLedState = false;
      delay(1000);
      break;

    case PowerState::Off:
      pwrLedState = true;
      delay(100);
      pwrLedState = false;
      delay(2900);
      break;
    }
    if (!WiFi.isConnected())
    {
      delay(200);
      for (int i = 0; i < 3; i++)
      {
        pwrLedState = true;
        delay(100);
        pwrLedState = false;
        delay(200);
      }
    }
  }
}

void PowerButton(void *pvParameters)
{
  while (1)
  {
    if (!powerctl)
    {
      digitalWrite(PIN_PWRBTN_MB, !digitalRead(PIN_PWRBTN_CASE));
    }
    delay(10);
  }
}

void PwrControl(void *pvParameters)
{
  Serial.println("PwrControl");
  int counter = 0;

  while (1)
  {
    if (!digitalRead(PIN_PWRLED_MB))
      counter++;
    else
      counter--;

    if (counter <= 0)
    {
      powerState = PowerState::Off;
      counter = 1;
    }
    else if (counter >= 15)
    {
      powerState = PowerState::On;
      counter = 14;
    }
    else
    {
      powerState = PowerState::Sleep;
    }

    delay(100);
  }
}

static WebServer server(80);

void httpServer(void *pvParameters)
{
  Serial.println("httpServer");

  WiFi.mode(WIFI_MODE_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(Config.getString("WIFI.SSID").c_str(), Config.getString("WIFI.PASSWORD").c_str());

  server.on("/", []()
            {
    if (server.method() != HTTPMethod::HTTP_GET) {
      server.send(405, "text/plain", "GET Only");
      return;
    }
#include "index.html.cpp"
    server.send(200, "text/html",INDEX_HTML); });

  server.on("/state", []()
            {
    if (server.method() != HTTPMethod::HTTP_GET) {
      server.send(405, "text/plain", "GET Only");
      return;
    }
    String result = "{\"state\":\"";
    switch (powerState) {
      case PowerState::On:
        result += "On";
        break;
      case PowerState::Sleep:
        result += "Sleep";
        break;
      case PowerState::Off:
        result += "Off";
        break;
    }
    result += "\"}";

    server.send(200, "application/json", result); });

  server.on("/name", []()
            {
    if (server.method() != HTTPMethod::HTTP_GET) {
      server.send(405, "text/plain", "GET or POST");
    }
    String result = "{\"name\":\"";
    result += Config.getString("WEB.HOSTNAME");
    result += "\"}";
    server.send(200, "application/json", result);
    return; });

  server.on("/on", []()
            {
    if (server.method() != HTTPMethod::HTTP_POST) {
      server.send(405, "text/plain", "POST Only");
      return;
    }
    if (!server.authenticate(Config.getString("WEB.HOSTNAME").c_str(), Config.getString("WEB.PASSWORD").c_str())) {
      server.send(401, "text/plain", "not authorized");
      return;
    }
    if (powerState != PowerState::On) {
      server.send(202, "text/plain", "triggered");
      powerctl = true;
      digitalWrite(PIN_PWRBTN_MB, true);
      delay(100);
      digitalWrite(PIN_PWRBTN_MB, false);
      powerctl = false;
    } else {
      server.send(409, "text/plain", "already on");
    } });

  server.on("/off", []()
            {
    if (server.method() != HTTPMethod::HTTP_POST) {
      server.send(405, "text/plain", "POST Only");
      return;
    }
    if (!server.authenticate(Config.getString("WEB.HOSTNAME").c_str(), Config.getString("WEB.PASSWORD").c_str())) {
      server.send(401, "text/plain", "not authorized");
      return;
    }
    if (powerState == PowerState::On) {
      server.send(202, "text/plain", "triggered");
      powerctl = true;
      digitalWrite(PIN_PWRBTN_MB, true);
      delay(100);
      digitalWrite(PIN_PWRBTN_MB, false);
      powerctl = false;
    } else {
      server.send(409, "text/plain", "already off");
    } });

  server.on("/force-off", []()
            {
    if (server.method() != HTTPMethod::HTTP_POST) {
      server.send(405, "text/plain", "POST Only");
      return;
    }
    if (!server.authenticate(Config.getString("WEB.HOSTNAME").c_str(), Config.getString("WEB.PASSWORD").c_str())) {
      server.send(401, "text/plain", "not authorized");
      return;
    }
    if (powerState != PowerState::Off) {
      server.send(202, "text/plain", "force power off");
      powerctl = true;
      digitalWrite(PIN_PWRBTN_MB, true);
      delay(5000);
      digitalWrite(PIN_PWRBTN_MB, false);
      powerctl = false;
    } else {
      server.send(409, "text/plain", "already off");
    } });

  server.on("/update", HTTP_GET, []()
            {
#include "update.html.cpp"
        server.send(200, "text/html", UPDATE_HTML); });
  // OTA Update
  server.on(
      "/firmware", HTTP_POST, []()
      {
    server.sendHeader("Connection", "close");
    if (Update.hasError())
    {
      server.send(500, "text/plain", Update.errorString());
    }
    else
    {
      server.send(200, "text/plain", "update done");
      ESP.restart();
    } },
      []()
      {
        HTTPUpload &upload = server.upload();

        switch (upload.status)
        {
        case UPLOAD_FILE_START:
          Update.begin();
          break;

        case UPLOAD_FILE_WRITE:
          Update.write(upload.buf, upload.currentSize);
          break;

        case UPLOAD_FILE_END:
          if (Update.end(true))
            Serial.printf("written %u / Restart", upload.totalSize);
          else
            Update.printError(Serial);
          break;

        case UPLOAD_FILE_ABORTED:
          Update.abort();
          break;
        }
      });
  server.on("/firmware", HTTP_GET, []()
            { server.send(405, "Method Not Allowed"); });

  bool active = false;

  while (1)
  {
    if (WiFi.isConnected())
    {
      if (active)
      {
        server.handleClient();
      }
      else
      {
        server.begin();
        MDNS.begin(Config.getString("WEB.HOSTNAME", "RemotePowerButton").c_str());
        MDNS.addService("http", "tcp", 80);
        active = true;
      }
    }
    else if (active)
    {
      server.close();
      MDNS.end();
      active = false;
    }

    delay(10);
  }
}

void loop()
{
}
