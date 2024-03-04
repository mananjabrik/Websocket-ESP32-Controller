// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char *ssid = "@wifiRoot";
const char *password = "test225610!";

bool ledState = 0;
const int ledPin = 2;
String message = "";
String status = "";
String value = "";

int outMotorCW = 27;
int outMotorCCW = 14;
int pwmCW = 0;
int pwmCCW = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RC Controller</title>
</head>
<body>
  <h1>RC Controller</h1>
    <div class="controller">
      <h1 id="connect"></h1> 
    </div>

  <script>
    const h1 = document.getElementById("connect");
    h1.innerText = `connnect ws to this link  ws://${window.location.hostname}/ws`
  </script>
  
</body>
</html>
)rawliteral";

void notifyClients()
{
  ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    message = (char *)data;

    status = message.substring(0, message.indexOf("&"));
    value = message.substring(message.indexOf("&") + 1, message.length());

    Serial.print("Status: ");
    Serial.println(status);

    if (status == "speed")
    {

      int intValue = value.toInt();
      int pwmMax = 255;
      pwmCW = pwmMax * intValue / 100;
      Serial.print("Speed Increase ");
      Serial.println(pwmCW);
    }

    if (status == "steer")
    {
      Serial.print("change dir ");
      Serial.println(value);
    }

    if (status == "toggle")
    {
      ledState = !ledState;
      notifyClients();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
    break;
  }
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

String processor(const String &var)
{
  Serial.println(var);
  if (var == "STATE")
  {
    if (ledState)
    {
      return "ON";
    }
    else
    {
      return "OFF";
    }
  }
  return String();
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(outMotorCW, OUTPUT);
  pinMode(outMotorCCW, OUTPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  // Start server
  server.begin();
}

void loop()
{
  ws.cleanupClients();
  digitalWrite(ledPin, ledState);
  digitalWrite(outMotorCW, pwmCW);
  digitalWrite(outMotorCCW, LOW);
}
