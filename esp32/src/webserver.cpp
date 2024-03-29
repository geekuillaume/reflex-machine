#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#include "./main.hpp"

// Replace with your network credentials
const char* ssid = "ThisIsNotTheWifiYoureLookingFor";
const char* password = "244466666";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

extern const uint8_t index_html_start[] asm("_binary_src_webui_index_html_gz_start");
extern const uint8_t index_html_end[] asm("_binary_src_webui_index_html_gz_end");

extern const uint8_t css_start[] asm("_binary_src_webui_assets_index_css_gz_start");
extern const uint8_t css_end[] asm("_binary_src_webui_assets_index_css_gz_end");

extern const uint8_t js_start[] asm("_binary_src_webui_assets_index_js_gz_start");
extern const uint8_t js_end[] asm("_binary_src_webui_assets_index_js_gz_end");

// Use to debounce game stats broadcast + run it on separate core
bool shouldBroadcastGameState = false;

void broadcastGameState() {
  shouldBroadcastGameState = true;
}

void sendGameStateBroadcastLoop(void *pvParameters) {
  StaticJsonDocument<256> wsMessage;
  String serializedWsMesssage;
  unsigned long startedMsAgo;

  for(;;){
    if (shouldBroadcastGameState) {
      serializedWsMesssage = "";
      wsMessage.clear();
      startedMsAgo = gameLastActionTime - gameStartedAt;
      wsMessage["type"] = "gameState";
      wsMessage["state"] = gameState == IDLE ? "IDLE"
        : gameState == WAITING_FOR_FIRST_PRESS ? "WAITING_FOR_FIRST_PRESS"
        : gameState == RUNNING ? "RUNNING"
        : gameState == FINISHED ? "FINISHED"
        : "UNKNOWN";
      wsMessage["missed"] = gameButtonsMissed;
      wsMessage["pressed"] = gameButtonsPressed;
      wsMessage["startedMsAgo"] = startedMsAgo;
      serializeJson(wsMessage, serializedWsMesssage);
      ws.textAll(serializedWsMesssage);
      shouldBroadcastGameState = false;
      debugA("Sent game broadcast");
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  StaticJsonDocument<256> parsedMessage;

  if (info->final &&
    info->index == 0 &&
    info->len == len &&
    info->opcode == WS_TEXT
  ) {
    DeserializationError error = deserializeJson(parsedMessage, data, len);
    if (error) {
      Serial.printf("Error parsing JSON");
      Serial.println(error.f_str());
      return;
    }

    if (strcmp("stopGame", parsedMessage["type"]) == 0) {
      stopGame();
    }

    if (strcmp("preferences", parsedMessage["type"]) == 0) {
      if (parsedMessage.containsKey("parallel")) {
        preferences.putUInt("parallel", parsedMessage["parallel"]);
        GAME_BUTTONS_ON_IN_PARALLEL = parsedMessage["parallel"];
      }
      if (parsedMessage.containsKey("duration")) {
        preferences.putUInt("duration", parsedMessage["duration"]);
        GAME_BUTTONS_DURATION_PRESSED = parsedMessage["duration"];
      }
      stopGame();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      broadcastGameState();
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

void loopWebserver(void *pvParameters) {
  for (;;) {
    ws.cleanupClients();
    ArduinoOTA.handle();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setupWebserver() {
  WiFi.begin(ssid, password);
  // Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    debugA("Connecting to WiFi..");
  }

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      debugA("Start updating %s\n", type);
    })
    .onEnd([]() {
      debugA("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
     debugA("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      debugA("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  // Print ESP Local IP Address
  printlnA(WiFi.localIP());

  ws.onEvent(onEvent);
  server.addHandler(&ws);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_start, index_html_end - index_html_start);
    response->addHeader("Content-Encoding", "gzip");
    response->addHeader("Content-Type", "text/html");
    request->send(response);
  });

  server.on("/assets/index.css", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", css_start, css_end - css_start);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/assets/index.js", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", js_start, js_end - js_start);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  // Start server
  server.begin();

  xTaskCreatePinnedToCore(
    sendGameStateBroadcastLoop,   /* Task function. */
    "sendGameStateBroadcast",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    1,           /* priority of the task */
    NULL,      /* Task handle to keep track of created task */
    0 // Core 0
  );

  xTaskCreatePinnedToCore(
    loopWebserver,
    "loopWebserver",
    10000,
    NULL,
    2,
    NULL,
    0
  );
}
