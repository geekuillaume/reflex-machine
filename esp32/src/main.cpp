#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "Button2.h"
#include <arduino-timer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>

uint GAME_BUTTONS_ON_IN_PARALLEL = 3;
uint GAME_BUTTONS_DURATION_PRESSED = 100; // Game stop after 100 presses

Preferences preferences;

// Replace with your network credentials
const char* ssid = "ThisIsNotTheWifiYoureLookingFor";
const char* password = "244466666";

typedef struct {
  int buttonPin;
  int ledPin;
  unsigned long ledTurnedOnAt;
  Button2 button;
} reflex_button;

#define BUTTONS_COUNT 10
reflex_button buttons[] = {
  { .buttonPin = 13, .ledPin = 15 },
  { .buttonPin = 12, .ledPin = 2 },
  { .buttonPin = 14, .ledPin = 0 },
  { .buttonPin = 27, .ledPin = 4 },
  { .buttonPin = 26, .ledPin = 16 },
  { .buttonPin = 25, .ledPin = 17 },
  { .buttonPin = 33, .ledPin = 5 },
  { .buttonPin = 32, .ledPin = 18 },
  { .buttonPin = 22, .ledPin = 19 },
  { .buttonPin = 23, .ledPin = 21 },
};

typedef enum  {
  IDLE,
  WAITING_FOR_FIRST_PRESS,
  RUNNING,
  FINISHED
} GAME_STATE;

GAME_STATE gameState = IDLE;
unsigned int gameButtonsPressed = 0;
unsigned int gameButtonsMissed = 0;
unsigned long gameStartedAt = 0;
unsigned long gameLastActionTime = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

auto beforeIdleTimer = timer_create_default(); // timer to re-start idle state after finished
auto afterFinishedLightFlashTimer = timer_create_default(); // timer to flash lights after finished

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html></html>
)rawliteral";

void turnOffButton(int buttonIndex) {
  buttons[buttonIndex].ledTurnedOnAt = 0;
  digitalWrite(buttons[buttonIndex].ledPin, LOW);
}

void turnOffAllButtons() {
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    turnOffButton(i);
  }
}

void turnOnButton(int buttonIndex) {
  buttons[buttonIndex].ledTurnedOnAt = millis();
  digitalWrite(buttons[buttonIndex].ledPin, HIGH);
}

void turnOnAllButtons() {
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    turnOnButton(i);
  }
}

void broadcastGameState() {
  StaticJsonDocument<256> wsMessage;
  String serializedWsMesssage;

  unsigned long startedMsAgo = gameLastActionTime - gameStartedAt;

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
  Serial.println(serializedWsMesssage);
  ws.textAll(serializedWsMesssage);
}


void stopGame() {
  gameState = FINISHED;
  turnOffAllButtons();
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

int countOnButtons() {
  int count = 0;
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    if (buttons[i].ledTurnedOnAt != 0) {
      count++;
    }
  }

  return count;
}

bool goToIdle(void *) {
  Serial.println("Flashing buttons");

  gameState = IDLE;
  afterFinishedLightFlashTimer.cancel();
  turnOffAllButtons();
  broadcastGameState();

  return true;
}

bool flashAllButtons(void *) {
  bool wasOn = buttons[0].ledTurnedOnAt != 0;

  Serial.println("Flashing buttons");
  if (wasOn) {
    turnOffAllButtons();
  } else {
    turnOnAllButtons();
  }

  return true;
}

void onButtonPressed(Button2& btn) {
  if (gameState == FINISHED) {
    return;
  }

  if (gameState == IDLE) {
    turnOnAllButtons(); // reset ledTurnOnAt for every button to reset delay of other buttons
    gameState = WAITING_FOR_FIRST_PRESS;
    broadcastGameState();
    return;
  }

  if (gameState == WAITING_FOR_FIRST_PRESS) {
    turnOnAllButtons(); // reset ledTurnOnAt for every button to reset delay of other buttons
    gameState = RUNNING;
    gameButtonsPressed = 0;
    gameButtonsMissed = 0;
    gameStartedAt = millis();
  }

  if (buttons[btn.getID()].ledTurnedOnAt == 0) {
    gameButtonsMissed++;
  } else {
    gameButtonsPressed++;
    turnOffButton(btn.getID());
  }
  gameLastActionTime = millis();

  int onButtonsCount = countOnButtons();

  if (gameButtonsPressed >= GAME_BUTTONS_DURATION_PRESSED) {
    gameState = FINISHED;
    beforeIdleTimer.in(5000, goToIdle);
    afterFinishedLightFlashTimer.every(250, flashAllButtons);
  } else if (
    onButtonsCount < GAME_BUTTONS_ON_IN_PARALLEL &&
    gameButtonsPressed + onButtonsCount < GAME_BUTTONS_DURATION_PRESSED
  ) {
    // turn on new random button which is not one already on and not the one just pressed
    int newButtonId;
    do {
      newButtonId = floor(random(0, BUTTONS_COUNT));
    } while (newButtonId == btn.getID() || buttons[newButtonId].ledTurnedOnAt != 0);

    turnOnButton(newButtonId);
  }

  broadcastGameState();
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);

  Serial.println("Starting");

  preferences.begin("reflex-machine", false);
  GAME_BUTTONS_DURATION_PRESSED = preferences.getUInt("duration", 100);
  GAME_BUTTONS_ON_IN_PARALLEL = preferences.getUInt("parallel", 2);

  for (int i = 0; i < BUTTONS_COUNT; i++) {
    buttons[i].button.begin(buttons[i].buttonPin);
    buttons[i].button.setID(i);
    buttons[i].button.setPressedHandler(onButtonPressed);

    pinMode(buttons[i].ledPin, OUTPUT);
    digitalWrite(buttons[i].ledPin, LOW);

    buttons[i].ledTurnedOnAt = 0;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // Start server
  server.begin();


}

void loop() {
  afterFinishedLightFlashTimer.tick();
  beforeIdleTimer.tick();
  for (int i = 0; i < BUTTONS_COUNT; i++) {
    buttons[i].button.loop();
  }
  ws.cleanupClients();
  ArduinoOTA.handle();
}
